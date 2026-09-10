#include "on_num_export_end.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <cmath>

num_export_end::num_export_end(QWidget* parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	savePath = QDir::homePath() + "/Desktop";

	initUI();
	setWindowTitle(QString::fromUtf8("批量输出"));
	resize(560, 420);
}

num_export_end::~num_export_end()
{
}

void num_export_end::initUI()
{
	pathLabel = new QLabel(QString::fromUtf8("路径: ") + savePath, this);
	browseBtn = new QPushButton(QString::fromUtf8("浏览"), this);
	exportBtn = new QPushButton(QString::fromUtf8("开始导出"), this);
	logEdit = new QPlainTextEdit(this);
	logEdit->setReadOnly(true);

	QHBoxLayout* pathLayout = new QHBoxLayout;
	pathLayout->addWidget(pathLabel, 1);
	pathLayout->addWidget(browseBtn);

	QHBoxLayout* btnLayout = new QHBoxLayout;
	btnLayout->addStretch(1);
	btnLayout->addWidget(exportBtn);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(pathLayout);
	mainLayout->addLayout(btnLayout);
	mainLayout->addWidget(logEdit);

	connect(browseBtn, &QPushButton::clicked, this, &num_export_end::onSelectSavePath);
	connect(exportBtn, &QPushButton::clicked, this, &num_export_end::onExportAll);
}

void num_export_end::onSelectSavePath()
{
	QString selectedPath = QFileDialog::getExistingDirectory(this,
		QString::fromUtf8("选择保存路径"), savePath);
	if (!selectedPath.isEmpty()) {
		savePath = selectedPath;
		pathLabel->setText(QString::fromUtf8("路径: ") + savePath);
	}
}

bool num_export_end::collectPathPoints(ULONG pathID, std::vector<AptPoint>& points)
{
	points.clear();

	int nPointsCount = 0;
	ULONG* ulPointsIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nPointsCount, &ulPointsIDs);
	if (FAILED(hr) || nPointsCount <= 0 || ulPointsIDs == nullptr) {
		if (ulPointsIDs) {
			m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);
		}
		return false;
	}

	// 逐点读取笛卡尔位姿：QUATERNION时 dPointPosture = [X, Y, Z, qw, qx, qy, qz]
	for (int i = 0; i < nPointsCount; i++) {
		ULONG pointID = ulPointsIDs[i];

		PQPostureType nPostureType = QUATERNION;
		INT nPostureCount = 0;
		double* dPointPosture = nullptr;
		double dVelocity = 0.0;
		double dSpeedPercent = 0.0;
		PQPointInstruction nInstruct = PQ_LINE;
		INT nApproach = 0;

		HRESULT hrPoint = m_ptrKit->PQAPIGetPointInfo(pointID, nPostureType, &nPostureCount, &dPointPosture,
			&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

		if (SUCCEEDED(hrPoint) && dPointPosture != nullptr && nPostureCount >= 3) {
			AptPoint apt;
			apt.x = dPointPosture[0];
			apt.y = dPointPosture[1];
			apt.z = dPointPosture[2];
			apt.velocity = dVelocity;

			// 由姿态四元数求刀轴方向：刀轴 = R(q) * (0,0,1)
			if (nPostureCount >= 7) {
				const double qw = dPointPosture[3];
				const double qx = dPointPosture[4];
				const double qy = dPointPosture[5];
				const double qz = dPointPosture[6];
				apt.i = 2.0 * (qx * qz + qw * qy);
				apt.j = 2.0 * (qy * qz - qw * qx);
				apt.k = 1.0 - 2.0 * (qx * qx + qy * qy);
			}
			else {
				apt.i = 0.0;
				apt.j = 0.0;
				apt.k = -1.0;
			}

			points.push_back(apt);
		}

		if (dPointPosture) {
			m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
		}
	}

	m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);

	// 相邻点距离检查：超过5mm时按5mm步长插补
	interpolatePoints(points);

	return !points.empty();
}

// 相邻点距离检查：超过5mm时按5mm步长线性插补新点（位置与刀轴矢量同步插值）
void num_export_end::interpolatePoints(std::vector<AptPoint>& points)
{
	const double kMaxSpacing = 5.0; // 允许的最大点间距(mm)

	std::vector<AptPoint> result;
	result.reserve(points.size() * 2);

	for (size_t idx = 0; idx < points.size(); idx++) {
		result.push_back(points[idx]);
		if (idx + 1 >= points.size()) {
			break;
		}

		const AptPoint& cur = points[idx];
		const AptPoint& next = points[idx + 1];

		const double dx = next.x - cur.x;
		const double dy = next.y - cur.y;
		const double dz = next.z - cur.z;
		const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (dist <= kMaxSpacing) {
			continue;
		}

		// 按5mm步长插入补点，末段为不足5mm的剩余距离
		const int insertCount = static_cast<int>(std::floor(dist / kMaxSpacing));
		for (int k = 1; k <= insertCount; k++) {
			const double t = (kMaxSpacing * k) / dist;
			AptPoint pt;
			pt.x = cur.x + dx * t;
			pt.y = cur.y + dy * t;
			pt.z = cur.z + dz * t;

			// 刀轴矢量线性插值后归一化为单位矢量
			pt.i = cur.i + (next.i - cur.i) * t;
			pt.j = cur.j + (next.j - cur.j) * t;
			pt.k = cur.k + (next.k - cur.k) * t;
			const double norm = std::sqrt(pt.i * pt.i + pt.j * pt.j + pt.k * pt.k);
			if (norm > 1e-12) {
				pt.i /= norm;
				pt.j /= norm;
				pt.k /= norm;
			}
			else {
				pt.i = cur.i;
				pt.j = cur.j;
				pt.k = cur.k;
			}

			pt.velocity = cur.velocity;
			result.push_back(pt);
		}
	}

	points.swap(result);
}

void num_export_end::onExportAll()
{
	logEdit->clear();

	if (m_ptrKit == nullptr) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("三维内核未初始化！"));
		return;
	}
	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("请先选择保存路径！"));
		return;
	}

	QMap<ULONG, QString> robotMap = getObjectsByType(PQ_ROBOT);
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, robotMap);
	if (robotNames.isEmpty()) {
		QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("当前没有可用的喷涂机器人！"));
		return;
	}

	// 所有轨迹点合并输出到同一个APT文件：每条路径一个工序块，GOTO点列连续排列
	const QString baseName = QString("%1_%2")
		.arg(QString::fromUtf8("批量输出"))
		.arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

	QStringList lines = buildAptHeader(baseName);

	int pathCount = 0;
	int skipCount = 0;
	int totalPoints = 0;

	for (int r = 0; r < robotNames.size(); r++) {
		const QString robotName = robotNames[r];
		ULONG robotID = robotMap.key(robotName, 0);

		QStringList groups = getPathGroupNames(robotID);
		for (int g = 0; g < groups.size(); g++) {
			QStringList paths = getPathNames(robotID, groups[g]);

			for (int p = 0; p < paths.size(); p++) {
				const QString& pathName = paths[p];

				ULONG pathID = 0;
				GetObjIDByName(PQ_PATH, pathName.toStdWString(), pathID);
				if (pathID == 0) {
					logEdit->appendPlainText(QString::fromUtf8("跳过(无法获取路径ID): %1 / %2")
						.arg(robotName, pathName));
					skipCount++;
					continue;
				}

				std::vector<AptPoint> points;
				if (!collectPathPoints(pathID, points)) {
					logEdit->appendPlainText(QString::fromUtf8("跳过(无轨迹点): %1 / %2")
						.arg(robotName, pathName));
					skipCount++;
					continue;
				}

				appendAptOperation(lines, points,
					QString("%1_%2_%3").arg(robotName, groups[g], pathName));

				pathCount++;
				totalPoints += static_cast<int>(points.size());
				logEdit->appendPlainText(QString::fromUtf8("已合并: %1 / %2 / %3 (%4个点)")
					.arg(robotName, groups[g], pathName).arg(points.size()));
			}
		}
	}

	if (pathCount == 0) {
		QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("没有可输出的轨迹点！"));
		return;
	}

	lines << "FINI";

	QString filePath = savePath + "/" + baseName + ".txt";
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromUtf8("错误"),
			QString::fromUtf8("无法创建文件: %1").arg(filePath));
		return;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream << lines.join("\n");
	file.close();

	QMessageBox::information(this, QString::fromUtf8("完成"),
		QString::fromUtf8("批量输出完成！共合并 %1 条路径、%2 个轨迹点，跳过 %3 条。\n输出文件: %4")
			.arg(pathCount).arg(totalPoints).arg(skipCount).arg(filePath));
}

QStringList num_export_end::buildAptHeader(const QString& partName)
{
	// CATIA APT文件头，结构照抄 JB_001.txt 样例：$$注释头 + PARTNO + MULTAX + 换刀工序
	QStringList lines;

	const QString dateStr = QDateTime::currentDateTime().toString("yyyy年M月d日 hh:mm:ss");
	lines << "$$ -----------------------------------------------------------------";
	lines << QString("$$     Generated on %1").arg(dateStr);
	lines << "$$     CATIA APT VERSION 1.0";
	lines << "$$ -----------------------------------------------------------------";
	lines << "$$ 001";
	lines << QString("$$  %1").arg(partName);
	lines << "$$*CATIA0";
	lines << "$$ 001";
	lines << "$$     0.00000     1.00000     0.00000     0.00000";
	lines << "$$     0.00000     0.00000     1.00000     0.00000";
	lines << "$$     1.00000     0.00000     0.00000     0.00000";
	lines << QString("PARTNO %1").arg(partName);

	// 换刀工序（固定模板）
	lines << "$$ OPERATION NAME : Tool Change.1";
	lines << "$$  Start generation of : Tool Change.1";
	lines << "MULTAX";
	lines << "$$ TOOLCHANGEBEGINNING";
	lines << "CUTTER/  0.000000,  0.000000,  0.000000,  0.000000,  0.000000,$";
	lines << "        10.000000, 40.000000";
	lines << "TOOLNO/1,MILL,1,0,   14.106000,  100.000000,$";
	lines << "  100.000000,  100.000000,   20.000000,   15.000000,,   40.000000,$";
	lines << " 1000.000000,MMPM,   70.000000,RPM,CLW,$";
	lines << "ON,,NOTE";
	lines << "TPRINT/T1 Conical Mill D 25,T1 Conical Mill D 25,T1 Conical Mill D 25";
	lines << "LOADTL/1,1,1";
	lines << "$$ TOOLCHANGEEND";
	lines << "$$  End of generation of : Tool Change.1";

	return lines;
}

void num_export_end::appendAptOperation(QStringList& lines, const std::vector<AptPoint>& points,
	const QString& operationName)
{
	// 轨迹工序：GOTO / X,Y,Z,I,J,K（XYZ保留5位小数，IJK保留6位小数并右对齐9列）
	lines << QString("$$ OPERATION NAME : %1").arg(operationName);
	lines << QString("$$  Start generation of : %1").arg(operationName);
	lines << "LOADTL/1,1";

	double feed = points.front().velocity;
	if (feed <= 0.0) {
		feed = 1000.0;
	}
	lines << QString("FEDRAT/ %1,MMPM").arg(feed, 9, 'f', 4);
	lines << "SPINDL/   70.0000,RPM,CLW";

	for (size_t i = 0; i < points.size(); i++) {
		const AptPoint& p = points[i];
		lines << QString("GOTO  / %1,%2,%3,%4,%5,%6")
			.arg(p.x, 0, 'f', 5)
			.arg(p.y, 0, 'f', 5)
			.arg(p.z, 0, 'f', 5)
			.arg(p.i, 9, 'f', 6)
			.arg(p.j, 9, 'f', 6)
			.arg(p.k, 9, 'f', 6);
	}

	lines << QString("$$  End of generation of : %1").arg(operationName);
}

//******************//
// 枚举辅助函数     //
//******************//

QMap<ULONG, QString> num_export_end::getObjectsByType(PQDataType objType)
{
	QMap<ULONG, QString> objectMap;

	VARIANT namesVariant;
	VariantInit(&namesVariant);
	namesVariant.parray = NULL;

	VARIANT idsVariant;
	VariantInit(&idsVariant);
	idsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Doc_get_obj_bytype(objType, &namesVariant, &idsVariant);
	if (FAILED(hr)) {
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap;
	}

	QStringList names = extractStringArrayFromVariant(namesVariant);
	QList<long> ids = extractLongArrayFromVariant(idsVariant);

	int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; i++) {
		objectMap[ids[i]] = names[i];
	}

	VariantClear(&namesVariant);
	VariantClear(&idsVariant);

	return objectMap;
}

QStringList num_export_end::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
{
	QStringList robotNames;

	for (auto it = robotMap.constBegin(); it != robotMap.constEnd(); ++it) {
		long id = it.key();
		PQRobotType robotType = PQ_MECHANISM_ROBOT;
		HRESULT hr = m_ptrKit->Robot_get_type(id, &robotType);

		if (SUCCEEDED(hr) && robotType == mechanismType) {
			robotNames.append(it.value());
		}
	}

	return robotNames;
}

QStringList num_export_end::getPathGroupNames(ULONG robotID)
{
	QStringList groupNames;

	VARIANT varGroupName;
	VariantInit(&varGroupName);
	varGroupName.parray = NULL;

	HRESULT hr = m_ptrKit->Doc_get_pathgroup_name(robotID, &varGroupName);
	if (SUCCEEDED(hr)) {
		groupNames = extractStringArrayFromVariant(varGroupName);
	}

	VariantClear(&varGroupName);
	return groupNames;
}

QStringList num_export_end::getPathNames(ULONG robotID, const QString& groupName)
{
	QStringList pathNames;

	VARIANT sNames;
	VARIANT sIDs;
	VariantInit(&sNames);
	VariantInit(&sIDs);
	sNames.parray = NULL;
	sIDs.parray = NULL;

	std::wstring wstrGroupName = groupName.toStdWString();
	BSTR bstrGroupName = SysAllocString(wstrGroupName.c_str());

	HRESULT hr = m_ptrKit->Path_get_group_path(robotID, bstrGroupName, &sNames, &sIDs);

	SysFreeString(bstrGroupName);

	if (SUCCEEDED(hr)) {
		pathNames = extractStringArrayFromVariant(sNames);
	}

	VariantClear(&sNames);
	VariantClear(&sIDs);

	return pathNames;
}

QStringList num_export_end::extractStringArrayFromVariant(const VARIANT& variant)
{
	QStringList result;

	if ((variant.vt & VT_ARRAY) == 0 || variant.vt != (VT_ARRAY | VT_BSTR)) {
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		return result;
	}

	long lowerBound = 0, upperBound = 0;
	if (FAILED(SafeArrayGetLBound(array, 1, &lowerBound)) ||
		FAILED(SafeArrayGetUBound(array, 1, &upperBound))) {
		return result;
	}

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		return result;
	}

	BSTR* data = nullptr;
	if (FAILED(SafeArrayAccessData(array, (void**)&data)) || data == nullptr) {
		return result;
	}

	for (long i = 0; i < elementCount; i++) {
		if (data[i] != nullptr) {
			result.append(QString::fromWCharArray(data[i]));
		}
		else {
			result.append(QString());
		}
	}

	SafeArrayUnaccessData(array);

	return result;
}

QList<long> num_export_end::extractLongArrayFromVariant(const VARIANT& variant)
{
	QList<long> result;

	if ((variant.vt & VT_ARRAY) == 0) {
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		return result;
	}

	long lowerBound = 0, upperBound = 0;
	SafeArrayGetLBound(array, 1, &lowerBound);
	SafeArrayGetUBound(array, 1, &upperBound);

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		return result;
	}

	result.reserve(elementCount);

	VARTYPE vt = VT_EMPTY;
	SafeArrayGetVartype(array, &vt);

	void* data = nullptr;
	if (FAILED(SafeArrayAccessData(array, &data)) || data == nullptr) {
		return result;
	}

	for (long i = 0; i < elementCount; i++) {
		long value = 0;
		switch (vt) {
		case VT_I4:  value = static_cast<LONG*>(data)[i]; break;
		case VT_I2:  value = static_cast<SHORT*>(data)[i]; break;
		case VT_UI4: value = static_cast<ULONG*>(data)[i]; break;
		default:     value = 0; break;
		}
		result.append(value);
	}

	SafeArrayUnaccessData(array);

	return result;
}

void num_export_end::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG& o_uID)
{
	o_uID = 0;

	VARIANT vNamePara;
	VariantInit(&vNamePara);
	vNamePara.parray = NULL;

	VARIANT vIDPara;
	VariantInit(&vIDPara);
	vIDPara.parray = NULL;

	m_ptrKit->Doc_get_obj_bytype(i_nType, &vNamePara, &vIDPara);
	if (NULL == vNamePara.parray || NULL == vIDPara.parray) {
		VariantClear(&vNamePara);
		VariantClear(&vIDPara);
		return;
	}

	QStringList names = extractStringArrayFromVariant(vNamePara);
	QList<long> ids = extractLongArrayFromVariant(vIDPara);

	int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; i++) {
		if (names[i].toStdWString() == i_wsName) {
			o_uID = (ULONG)ids[i];
			break;
		}
	}

	VariantClear(&vNamePara);
	VariantClear(&vIDPara);
}
