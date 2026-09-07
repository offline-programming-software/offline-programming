#include "on_export_end.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

export_end::export_end(QWidget* parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	initUI();
	setWindowTitle("单个输出");
	resize(440, 380);
	loadRobots();
}

export_end::~export_end()
{
}

void export_end::initUI()
{
	robotCombo = new QComboBox(this);
	groupCombo = new QComboBox(this);
	pathCombo = new QComboBox(this);
	pointSpin = new QSpinBox(this);
	pointSpin->setMinimum(1);
	resultEdit = new QPlainTextEdit(this);
	resultEdit->setReadOnly(true);
	outputBtn = new QPushButton(QString::fromUtf8("输出"), this);
	saveBtn = new QPushButton(QString::fromUtf8("保存到文件"), this);
	saveBtn->setEnabled(false);

	QFormLayout* form = new QFormLayout;
	form->addRow(QString::fromUtf8("机器人:"), robotCombo);
	form->addRow(QString::fromUtf8("路径组:"), groupCombo);
	form->addRow(QString::fromUtf8("路径:"), pathCombo);
	form->addRow(QString::fromUtf8("点序号:"), pointSpin);

	QHBoxLayout* btnLayout = new QHBoxLayout;
	btnLayout->addWidget(outputBtn);
	btnLayout->addWidget(saveBtn);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(form);
	mainLayout->addLayout(btnLayout);
	mainLayout->addWidget(resultEdit);

	connect(robotCombo, &QComboBox::currentTextChanged, this, &export_end::onRobotChanged);
	connect(groupCombo, &QComboBox::currentTextChanged, this, &export_end::onGroupChanged);
	connect(pathCombo, &QComboBox::currentTextChanged, this, &export_end::onPathChanged);
	connect(outputBtn, &QPushButton::clicked, this, &export_end::onOutput);
	connect(saveBtn, &QPushButton::clicked, this, &export_end::onSaveToFile);
}

void export_end::loadRobots()
{
	if (m_ptrKit == nullptr) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("三维内核未初始化！"));
		return;
	}

	m_robotMap = getObjectsByType(PQ_ROBOT);
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("当前没有可用的喷涂机器人！"));
		return;
	}

	robotCombo->blockSignals(true);
	robotCombo->clear();
	robotCombo->addItems(robotNames);
	robotCombo->blockSignals(false);

	onRobotChanged();
}

QString export_end::currentRobotName()
{
	return robotCombo->currentText().trimmed();
}

QString export_end::currentGroupName()
{
	return groupCombo->currentText().trimmed();
}

QString export_end::currentPathName()
{
	return pathCombo->currentText().trimmed();
}

void export_end::onRobotChanged()
{
	QString robotName = currentRobotName();
	if (robotName.isEmpty()) {
		return;
	}

	ULONG robotID = m_robotMap.key(robotName, 0);
	QStringList groups = getPathGroupNames(robotID);

	groupCombo->blockSignals(true);
	groupCombo->clear();
	groupCombo->addItems(groups);
	groupCombo->blockSignals(false);

	onGroupChanged();
}

void export_end::onGroupChanged()
{
	pathCombo->blockSignals(true);
	pathCombo->clear();

	QString robotName = currentRobotName();
	QString groupName = currentGroupName();
	if (!robotName.isEmpty() && !groupName.isEmpty()) {
		ULONG robotID = m_robotMap.key(robotName, 0);
		pathCombo->addItems(getPathNames(robotID, groupName));
	}

	pathCombo->blockSignals(false);

	onPathChanged();
}

void export_end::onPathChanged()
{
	int count = 0;

	QString pathName = currentPathName();
	if (!pathName.isEmpty()) {
		ULONG pathID = 0;
		GetObjIDByName(PQ_PATH, pathName.toStdWString(), pathID);
		if (pathID != 0) {
			int nPointsCount = 0;
			ULONG* ulPointsIDs = nullptr;
			if (SUCCEEDED(m_ptrKit->Path_get_point_id(pathID, &nPointsCount, &ulPointsIDs))) {
				count = nPointsCount;
			}
			if (ulPointsIDs) {
				m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);
			}
		}
	}

	pointSpin->setRange(1, qMax(count, 1));
	pointSpin->setValue(1);
}

void export_end::onOutput()
{
	resultEdit->clear();
	saveBtn->setEnabled(false);

	QString robotName = currentRobotName();
	QString pathName = currentPathName();
	if (robotName.isEmpty() || pathName.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("请先选择机器人和路径！"));
		return;
	}

	// 获取路径ID及其点ID列表
	ULONG pathID = 0;
	GetObjIDByName(PQ_PATH, pathName.toStdWString(), pathID);
	if (pathID == 0) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("无法获取路径ID！"));
		return;
	}

	int nPointsCount = 0;
	ULONG* ulPointsIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nPointsCount, &ulPointsIDs);
	if (FAILED(hr) || nPointsCount <= 0 || ulPointsIDs == nullptr) {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("该路径上没有轨迹点！"));
		if (ulPointsIDs) {
			m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);
		}
		return;
	}

	int nIndex = pointSpin->value() - 1; // 界面序号从1开始
	if (nIndex < 0 || nIndex >= nPointsCount) {
		QMessageBox::warning(this, QString::fromUtf8("警告"),
			QString::fromUtf8("点序号超出范围，当前路径共 %1 个点！").arg(nPointsCount));
		m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);
		return;
	}

	ULONG pointID = ulPointsIDs[nIndex];
	m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);

	// 读取点位姿：EULERANGLEXYZ时 dPointPosture = [X, Y, Z, Rx, Ry, Rz]
	PQPostureType nPostureType = EULERANGLEXYZ;
	INT nPostureCount = 6;
	double* dPointPosture = nullptr;
	double dVelocity = 0;
	double dSpeedPercent = 0;
	PQPointInstruction nInstruct = PQ_LINE;
	INT nApproach = 0;

	hr = m_ptrKit->PQAPIGetPointInfo(pointID, nPostureType, &nPostureCount, &dPointPosture,
		&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

	if (SUCCEEDED(hr) && dPointPosture != nullptr && nPostureCount >= 3) {
		QString content;
		content += QString::fromUtf8("机器人: %1    路径: %2\n").arg(robotName, pathName);
		content += QString::fromUtf8("点序号: %1    点ID: %2\n").arg(nIndex + 1).arg((ulong)pointID);
		content += QString::fromUtf8("坐标: X=%1, Y=%2, Z=%3\n")
			.arg(dPointPosture[0], 0, 'f', 3)
			.arg(dPointPosture[1], 0, 'f', 3)
			.arg(dPointPosture[2], 0, 'f', 3);
		if (nPostureCount >= 6) {
			content += QString::fromUtf8("位姿(欧拉角XYZ): Rx=%1, Ry=%2, Rz=%3\n")
				.arg(dPointPosture[3], 0, 'f', 3)
				.arg(dPointPosture[4], 0, 'f', 3)
				.arg(dPointPosture[5], 0, 'f', 3);
		}
		else {
			content += QString::fromUtf8("位姿: 无欧拉角数据(仅%1个值)\n").arg(nPostureCount);
		}
		content += QString::fromUtf8("速度: %1    速度百分比: %2\n")
			.arg(dVelocity, 0, 'f', 3)
			.arg(dSpeedPercent);

		resultEdit->setPlainText(content);
		saveBtn->setEnabled(true);
	}
	else {
		QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("获取轨迹点位姿失败！"));
	}

	if (dPointPosture) {
		m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
	}
}

void export_end::onSaveToFile()
{
	QString content = resultEdit->toPlainText();
	if (content.isEmpty()) {
		return;
	}

	QString defaultName = QString("%1_%2_P%3.txt")
		.arg(currentRobotName(), currentPathName())
		.arg(pointSpin->value());
	QString fileName = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存轨迹点"),
		defaultName, "Text files(*.txt)");
	if (fileName.isEmpty()) {
		return;
	}

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromUtf8("错误"),
			QString::fromUtf8("无法创建文件: %1").arg(fileName));
		return;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream << content;
	file.close();
}

//******************//
// 枚举辅助函数     //
//******************//

QMap<ULONG, QString> export_end::getObjectsByType(PQDataType objType)
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

QStringList export_end::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

QStringList export_end::getPathGroupNames(ULONG robotID)
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

QStringList export_end::getPathNames(ULONG robotID, const QString& groupName)
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

QStringList export_end::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> export_end::extractLongArrayFromVariant(const VARIANT& variant)
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

void export_end::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG& o_uID)
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
