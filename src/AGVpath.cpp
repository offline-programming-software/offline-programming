#include "AGVpath.h"

AGVpath::AGVpath(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::AGVpathClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);
	init();

	connect(ui->pushButton_1, &QPushButton::clicked, this, &AGVpath::onCalculate);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &AGVpath::onConfirm);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &AGVpath::onCancel);
}

AGVpath::~AGVpath()
{
	delete ui;
}

QStringList AGVpath::getPathGroupNames(ULONG robotID)
{
	QStringList groupNames;

	VARIANT varGroupName;
	VariantInit(&varGroupName);
	varGroupName.parray = NULL;

	// 调用接口获取轨迹组名称
	HRESULT hr = m_ptrKit->Doc_get_pathgroup_name(robotID, &varGroupName);
	if (SUCCEEDED(hr)) {
		groupNames = extractStringArrayFromVariant(varGroupName);
		qDebug() << "成功获取轨迹组名称，数量:" << groupNames.size() << "机器人ID:" << robotID;
	}
	else {
		qDebug() << "获取轨迹组名称失败，机器人ID:" << robotID << "错误码:" << hr;
		QMessageBox::warning(this, "警告", "获取轨迹组名称失败！");
	}

	VariantClear(&varGroupName);
	return groupNames;
}

QStringList AGVpath::getPathNames(ULONG robotID, const QString& groupName)
{
	QStringList pathNames;

	VARIANT sNames;
	VARIANT sIDs;
	VariantInit(&sNames);
	VariantInit(&sIDs);
	sNames.parray = NULL;
	sIDs.parray = NULL;

	// 将QString转换为std::wstring，再获取C风格宽字符串指针
	std::wstring wstrGroupName = groupName.toStdWString();
	BSTR bstrGroupName = SysAllocString(wstrGroupName.c_str());

	// 或者更简洁的一行写法：
	// BSTR bstrGroupName = SysAllocString(groupName.toStdWString().c_str());

	HRESULT hr = m_ptrKit->Path_get_group_path(robotID, bstrGroupName, &sNames, &sIDs);

	SysFreeString(bstrGroupName);

	if (SUCCEEDED(hr)) {
		pathNames = extractStringArrayFromVariant(sNames);
		qDebug() << "成功获取轨迹名称，数量:" << pathNames.size()
			<< "机器人ID:" << robotID << "组名:" << groupName;
	}
	else {
		qDebug() << "获取轨迹名称失败，机器人ID:" << robotID
			<< "组名:" << groupName << "错误码:" << hr;
	}

	VariantClear(&sNames);
	VariantClear(&sIDs);

	return pathNames;
}

void AGVpath::init()
{
	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_WORKINGPART;
	m_AGVMap = getObjectsByType(robotType);

	QList<QString> AGVNames;

	if (m_AGVMap.isEmpty()) {
		QMessageBox::information(this, "提示", "当前无AGV！");
		return;
	}

	for (auto it = m_AGVMap.begin(); it != m_AGVMap.end(); it++) {
		AGVNames.push_back(it.value());
	}

	ui->comboBox_1->addItems(AGVNames);
	ui->comboBox_5->addItems(AGVNames);

	QString AGVName = ui->comboBox_1->currentText();

	ULONG AGVID = m_AGVMap.key(AGVName);


	connect(ui->comboBox_1, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onAGVSelectionChanged);

	if (ui->comboBox_1->count() > 0) { // 确保有选项后再触发
		onAGVSelectionChanged(ui->comboBox_1->currentIndex());
	}

}

void AGVpath::onCalculate(){

	// 获取AGV名称
	QString AGVname = ui->comboBox_1->currentText();
	ULONG AGVID = m_AGVMap.key(AGVname);

	// 获取起点和终点名称
	QString startPointName = ui->comboBox_2->currentText();
	QString endPointName = ui->comboBox_3->currentText();

	//获取驱动点
	INT nCount = 0;
	WCHAR* whPathNames = nullptr;
	ULONG* uPathIDs = nullptr;
	m_ptrKit->Part_get_path(AGVID, &nCount, &whPathNames, &uPathIDs);
	QMap<ULONG, QString> AGVpoints;

	QString pathName = QString::fromWCharArray(whPathNames);
	QStringList pointList = pathName.split('#', QString::SkipEmptyParts);

	for (int i = 0; i < nCount; i++) {
		AGVpoints.insert(uPathIDs[i], pointList[i]);
	}

	m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
	m_ptrKit->PQAPIFree((LONG_PTR*)uPathIDs);

	ULONG startPathID = AGVpoints.key(startPointName);
	ULONG endPathID = AGVpoints.key(endPointName);

	// 1、获取起点和终点的位置信息
	PQPostureType nPostureType = QUATERNION;
	INT nPostureCount = 0;
	double* dStartPosture = nullptr;
	double* dEndPosture = nullptr;
	double dVelocity = 0;
	double dSpeedPercent = 0;
	PQPointInstruction nInstruct = PQ_LINE;
	INT nApproach = 0;

	int Count = 0;
	ULONG* startPointIDs = nullptr;
	m_ptrKit->Path_get_point_id(startPathID, &Count, &startPointIDs);

	ULONG* endPointIDs = nullptr;
	m_ptrKit->Path_get_point_id(endPathID, &Count, &endPointIDs);

	// 获取起点位姿
	m_ptrKit->PQAPIGetPointInfo(startPointIDs[0], nPostureType, &nPostureCount, &dStartPosture,
		&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

	// 获取终点位姿
	m_ptrKit->PQAPIGetPointInfo(endPointIDs[0], nPostureType, &nPostureCount, &dEndPosture,
		&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);



	// 2、指定方向（用户指定避让方向）
	QString dirName = ui->comboBox_4->currentText();
	QVector3D dir = getDirectionVector(dirName);

	// 3、确认距离（通过计算出避让对象的包围盒，计算出避让的距离）
	ULONG avoidanceObjectID = m_AGVMap.key(AGVname);
	AABB avoidObj = creatAABB(avoidanceObjectID, 0);

	// 4、计算过渡点的边界值
	double boundaryValue = calculateBoundaryValue(avoidObj, dir);

	//// 5、创建过渡点
	std::vector<QVector3D> pos = calculateTransitionPoints(startPointIDs[0],
		endPointIDs[0], dStartPosture, dEndPosture, dir, boundaryValue);
	
	// 释放内存
	m_ptrKit->PQAPIFree((LONG_PTR*)startPointIDs);
	m_ptrKit->PQAPIFree((LONG_PTR*)endPointIDs);
	m_ptrKit->PQAPIFree((LONG_PTR*)dStartPosture);
	m_ptrKit->PQAPIFree((LONG_PTR*)dEndPosture);

	//通过移动轨迹点20、21来实现效果演示

	ULONG middlePathID1 = AGVpoints.key("驱动点20");
	ULONG middlePathID2 = AGVpoints.key("驱动点21");

	// 1、获取起点和终点的位置信息

	ULONG* middlePointIDs1 = nullptr;
	m_ptrKit->Path_get_point_id(middlePathID1, &Count, &middlePointIDs1);

	ULONG* middlePointIDs2 = nullptr;
	m_ptrKit->Path_get_point_id(middlePathID2, &Count, &middlePointIDs2);

	double* middlePos1 = nullptr;
	double* middlePos2 = nullptr;

	// 获取起点位姿
	m_ptrKit->PQAPIGetPointInfo(middlePointIDs1[0], nPostureType, &nPostureCount, &middlePos1,
		&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

	// 获取终点位姿
	m_ptrKit->PQAPIGetPointInfo(middlePointIDs2[0], nPostureType, &nPostureCount, &middlePos2,
		&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

	
	QVector3D middlePoint1 (middlePos1[0], middlePos1[1], middlePos1[2]);
	QVector3D middlePoint2 (middlePos2[0], middlePos2[1], middlePos2[2]);

	QVector3D middleMove1 = middlePoint1 - pos[0];
	QVector3D middleMove2 = middlePoint2 - pos[1];

	ULONG uPathId1[1] = { middlePointIDs1[0] };
	ULONG uPathId2[1] = { middlePointIDs2[0] };
	
	
	CComBSTR bsName;
	m_ptrKit->Doc_get_obj_name(middlePathID1, &bsName);

	INT i_PathCount = 1;
	m_ptrKit->Path_Translation(uPathId1, i_PathCount, middleMove1.x(), middleMove1.y(), middleMove1.z());
	m_ptrKit->Path_Translation(uPathId2, i_PathCount, middleMove2.x(), middleMove2.y(), middleMove2.z());

	m_ptrKit->PQAPIFree((LONG_PTR*)middlePointIDs1);
	m_ptrKit->PQAPIFree((LONG_PTR*)middlePointIDs2);

}

void AGVpath::onConfirm()
{
	reject();
}

void AGVpath::onCancel()
{
	reject();
}

QVector3D AGVpath::getDirectionVector(const QString& dirName) {
	// 将输入转换为小写并去除首尾空格，提高匹配稳定性
	QString direction = dirName.toLower().trimmed();

	if (direction == "x轴正方向" || direction == "x+")
		return QVector3D(1, 0, 0);
	else if (direction == "x轴负方向" || direction == "x-")
		return QVector3D(-1, 0, 0);
	else if (direction == "y轴正方向" || direction == "y+")
		return QVector3D(0, 1, 0);
	else if (direction == "y轴负方向" || direction == "y-")
		return QVector3D(0, -1, 0);
	else if (direction == "z轴正方向" || direction == "z+")
		return QVector3D(0, 0, 1);
	else if (direction == "z轴负方向" || direction == "z-")
		return QVector3D(0, 0, -1);
	else {
		// 默认情况或错误处理，返回零向量
		qWarning() << "未知的方向名称:" << dirName << "，返回零向量";
		return QVector3D(0, 0, 0);
	}
}

AABB AGVpath::creatAABB(ULONG uID, ULONG uCoordinate)
{
	AABB bbox;
	// 首先获取顶点数量
	long lCount = 0;
	double dSrcPosition[6] = {};
	m_ptrKit->PQAPIGetWorkPartVertexCount(uID, &lCount);

	//// 第一次调用获取顶点数量
	//m_ptrKit->PQAPIGetWorkPartVertex(uID, uCoordinate, lCount, dSrcPosition);

	//if (lCount <= 0) {
	//	return bbox; // 返回空的包围盒
	//}

	// 准备存储所有顶点数据
	std::vector<double> allVertexData(lCount * 3);

	// 重新设置lCount为实际要获取的数量
	int actualCount = lCount;
	m_ptrKit->PQAPIGetWorkPartVertex(uID, uCoordinate, actualCount, allVertexData.data());

	// 提取顶点坐标（假设每个顶点数据包含6个double，前3个是位置坐标）
	std::vector<Point3D> vertices;
	vertices.reserve(actualCount);

	for (int i = 0; i < actualCount; ++i) {
		double x = allVertexData[i * 3];
		double y = allVertexData[i * 3 + 1];
		double z = allVertexData[i * 3 + 2];
		vertices.emplace_back(x, y, z);
	}

	bbox = calculateAABB(vertices);

	return bbox;
}

double AGVpath::calculateBoundaryValue(const AABB& bbox, const QVector3D& dir)
{
	// 计算包围盒8个顶点在方向向量上的投影，找到最大值[1,4](@ref)
	std::vector<QVector3D> vertices = {
		QVector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z),
		QVector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.minPoint.z),
		QVector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.minPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.minPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z)
	};

	double maxProjection = std::numeric_limits<double>::lowest();

	for (const auto& vertex : vertices) {
		double projection = QVector3D::dotProduct(vertex, dir);
		if (projection > maxProjection) {
			maxProjection = projection;
		}
	}

	return maxProjection;
}

std::vector<QVector3D> AGVpath::calculateTransitionPoints(
	ULONG startPointID, ULONG endPointID,
	double* startPosture, double* endPosture,
	const QVector3D& dir, double boundaryValue)
{
	std::vector<QVector3D> transitionPoints;
	transitionPoints.reserve(2); // 预分配空间，提高效率

	// 计算起点和终点在方向向量上的投影
	QVector3D startPos(startPosture[0], startPosture[1], startPosture[2]);
	QVector3D endPos(endPosture[0], endPosture[1], endPosture[2]);

	double startProjection = QVector3D::dotProduct(startPos, dir);
	double endProjection = QVector3D::dotProduct(endPos, dir);

	// 计算过渡点1（基于起点）：沿方向向量移动到边界
	QVector3D transition1Pos = startPos + (boundaryValue - startProjection) * dir;

	// 计算过渡点2（基于终点）：沿方向向量移动到边界
	QVector3D transition2Pos = endPos + (boundaryValue - endProjection) * dir;

	// 将两个过渡点位置添加到结果向量中
	transitionPoints.push_back(transition1Pos);
	transitionPoints.push_back(transition2Pos);

	return transitionPoints;
}

void AGVpath::onAGVSelectionChanged(int index)
{
	// 忽略无效索引
	if (index < 0) {
		return;
	}
	// 调用更新函数
	QString AGVName = ui->comboBox_1->currentText();
	ULONG AGVID = m_AGVMap.key(AGVName);

	INT nCount = 0;
	WCHAR* whPathNames = nullptr;
	ULONG* uPathIDs = nullptr;
	m_ptrKit->Part_get_path(AGVID, &nCount, &whPathNames, &uPathIDs);

	QString pathName = QString::fromWCharArray(whPathNames);
	QStringList pointList = pathName.split('#', QString::SkipEmptyParts);

	// ！！！重要：在添加新项目前，清空 comboBox_2 和 comboBox_3 的旧内容 [4](@ref)
	ui->comboBox_2->clear();
	ui->comboBox_3->clear();

	// 添加新的路径点到下拉框
	ui->comboBox_2->addItems(pointList);
	ui->comboBox_3->addItems(pointList);

	// 释放内存
	m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
	m_ptrKit->PQAPIFree((LONG_PTR*)uPathIDs);
}

QMap<ULONG, QString> AGVpath::getObjectsByType(PQDataType objType)
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
		qDebug() << "获取对象列表失败，类型:" << objType << "错误码:" << hr;
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap;
	}

	// 提取名称数组
	QStringList names = extractStringArrayFromVariant(namesVariant);
	// 提取ID数组
	QList<long> ids = extractLongArrayFromVariant(idsVariant);

	// 构建映射关系
	int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; i++) {
		objectMap[ids[i]] = names[i];
	}

	VariantClear(&namesVariant);
	VariantClear(&idsVariant);

	qDebug() << "成功获取对象列表，类型:" << objType << "数量:" << objectMap.size();
	return objectMap;
}

QStringList AGVpath::extractStringArrayFromVariant(const VARIANT& variant)
{
	QStringList result;

	if ((variant.vt & VT_ARRAY) == 0 || variant.vt != (VT_ARRAY | VT_BSTR)) {
		qDebug() << "VARIANT 类型错误，期望VT_ARRAY|VT_BSTR，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		qDebug() << "SAFEARRAY 无效或维度不正确";
		return result;
	}

	// 获取数组边界
	long lowerBound, upperBound;
	HRESULT hr = SafeArrayGetLBound(array, 1, &lowerBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组下边界失败，错误码:" << hr;
		return result;
	}

	hr = SafeArrayGetUBound(array, 1, &upperBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组上边界失败，错误码:" << hr;
		return result;
	}

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		qDebug() << "数组元素数量为0或负数:" << elementCount;
		return result;
	}

	// 访问数组数据
	BSTR* data = nullptr;
	hr = SafeArrayAccessData(array, (void**)&data);
	if (FAILED(hr)) {
		qDebug() << "SafeArrayAccessData 失败，错误码:" << hr;
		return result;
	}

	// 提取所有字符串元素
	for (long i = 0; i < elementCount; i++) {
		if (data[i] != nullptr) {
			QString str = QString::fromWCharArray(data[i]);
			result.append(str);
		}
		else {
			result.append(QString()); // 空字符串处理
		}
	}

	// 取消数据访问
	SafeArrayUnaccessData(array);

	return result;
}

QList<long> AGVpath::extractLongArrayFromVariant(const VARIANT& variant)
{
	QList<long> result;

	if ((variant.vt & VT_ARRAY) == 0) {
		qDebug() << "VARIANT 类型错误，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		qDebug() << "SAFEARRAY 无效或维度不正确";
		return result;
	}

	long lowerBound, upperBound;
	SafeArrayGetLBound(array, 1, &lowerBound);
	SafeArrayGetUBound(array, 1, &upperBound);

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		return result;
	}

	result.reserve(elementCount);

	VARTYPE vt;
	SafeArrayGetVartype(array, &vt);

	HRESULT hr = SafeArrayLock(array);
	if (FAILED(hr)) {
		qDebug() << "锁定数组失败，错误码:" << hr;
		return result;
	}

	void* data = array->pvData;
	for (long i = 0; i < elementCount; i++) {
		long value = 0;
		switch (vt) {
		case VT_I4:
			value = static_cast<LONG*>(data)[i];
			break;
		case VT_I2:
			value = static_cast<SHORT*>(data)[i];
			break;
		case VT_UI4:
			value = static_cast<ULONG*>(data)[i];
			break;
		default:
			qDebug() << "不支持的数组元素类型:" << vt;
			break;
		}
		result.append(value);
	}

	SafeArrayUnlock(array);
	return result;
}
QStringList AGVpath::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
{
	QStringList robotNames;

	if (robotMap.isEmpty()) {
		return robotNames; // 返回空列表
	}

	// 遍历机器人映射表，筛选指定类型的机器人
	for (auto it = robotMap.constBegin(); it != robotMap.constEnd(); ++it) {
		long id = it.key();    // 获取机器人ID
		QString name = it.value(); // 获取机器人名称

		PQRobotType robotType = PQ_MECHANISM_ROBOT;
		HRESULT hr = m_ptrKit->Robot_get_type(id, &robotType);

		if (SUCCEEDED(hr) && robotType == mechanismType) {
			robotNames.append(name);
		}
	}

	return robotNames;
}

void AGVpath::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
{
	VARIANT vNamePara;
	vNamePara.parray = NULL;
	VARIANT vIDPara;
	vIDPara.parray = NULL;
	m_ptrKit->Doc_get_obj_bytype(i_nType, &vNamePara, &vIDPara);
	if (NULL == vNamePara.parray || NULL == vIDPara.parray)
	{
		return;
	}
	//缓存指定对象名称
	BSTR* bufName;
	long lenName = vNamePara.parray->rgsabound[0].cElements;
	SafeArrayAccessData(vNamePara.parray, (void**)&bufName);
	int nTarIndex = 0;
	if (!i_wsName.empty())
	{
		for (int i = 0; i < lenName; i++)
		{
			if (0 == i_wsName.compare(bufName[i]))
			{
				nTarIndex = i;
			}
		}
	}
	SafeArrayUnaccessData(vNamePara.parray);


	//缓存指定对象ID
	ULONG* bufID;
	long lenID = vIDPara.parray->rgsabound[0].cElements;
	SafeArrayAccessData(vIDPara.parray, (void**)&bufID);
	o_uID = bufID[nTarIndex];
	SafeArrayUnaccessData(vIDPara.parray);
}
