#include "posCal.h"
#include <comdef.h>
#include <oaidl.h>

posCal::posCal(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::posCalClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);
	init();

	connect(ui->pushButton_1, &QPushButton::clicked, this, &posCal::onCalculate);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &posCal::onShow);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &posCal::onConfirm);
	connect(ui->pushButton_4, &QPushButton::clicked, this, &posCal::onCancel);
}

posCal::~posCal()
{
	delete ui;
}


void posCal::init()
{
	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);
	
	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		return;
	}

	// 添加机器人名称到comboBox_1
	ui->comboBox_1->addItems(robotNames);

	// 初始化时触发第一个机器人的数据加载
	if (!robotNames.isEmpty()) {
		onComboBox1CurrentIndexChanged(0);
	}

	// 连接信号槽，实现联动效果
	connect(ui->comboBox_1, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &posCal::onComboBox1CurrentIndexChanged);
	connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &posCal::onComboBox2CurrentIndexChanged);
	connect(ui->comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &posCal::onComboBox4CurrentIndexChanged);


}

void posCal::onComboBox1CurrentIndexChanged(int index)
{
	if (index < 0) return;

	// 使用信号阻塞器避免递归调用
	QSignalBlocker blocker2(ui->comboBox_2);
	QSignalBlocker blocker3(ui->comboBox_3);

	// 清空现有内容
	ui->comboBox_2->clear();
	ui->comboBox_3->clear();

	// 获取当前选择的机器人名称和ID
	QString robotName = ui->comboBox_1->currentText();
	ULONG robotID = m_robotMap.key(robotName, 0);

	if (robotID == 0) {
		return;
	}

	// 获取轨迹组名称并添加到comboBox_2
	QStringList groupNames = getPathGroupNames(robotID);
	ui->comboBox_2->addItems(groupNames);

	// 如果有轨迹组，触发第一个轨迹组的轨迹加载
	if (!groupNames.isEmpty()) {
		onComboBox2CurrentIndexChanged(0);
	}
}

void posCal::onComboBox2CurrentIndexChanged(int index)
{
	if (index < 0) return;

	QSignalBlocker blocker(ui->comboBox_3);
	ui->comboBox_3->clear();

	// 获取当前选择的机器人和轨迹组
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_2->currentText();
	ULONG robotID = m_robotMap.key(robotName, 0);

	if (robotID == 0 || groupName.isEmpty()) {
		return;
	}

	// 获取轨迹名称并添加到comboBox_3
	QStringList pathNames = getPathNames(robotID, groupName);
	ui->comboBox_3->addItems(pathNames);
}

void posCal::onComboBox4CurrentIndexChanged(int index)
{
	if (index < 0) return;

	QString isAGV = ui->comboBox_4->currentText();

	if (isAGV == "是") {
		ULONG uPartID = 0;
		GetObjIDByName(PQ_WORKINGPART, L"零件2", uPartID);
		INT nCount = 0;
		WCHAR* whPathNames = nullptr; 
		ULONG* uPathIDs = nullptr;
		m_ptrKit->Part_get_path(uPartID, &nCount, &whPathNames, &uPathIDs);

		// 清空comboBox_5
		ui->comboBox_5->clear();


		if (whPathNames != nullptr) {
			std::wstring wstr = whPathNames;
			QString guidePoints = QString::fromStdWString(wstr);
			
			QStringList guideNames = guidePoints.split("#", QString::SkipEmptyParts);

			ui->comboBox_5->addItems(guideNames);

			for (int i = 0; i < nCount; i++) {
				m_AGVMap[uPathIDs[i]] = guideNames[i];
			}

		}

		// 释放内存
		if (whPathNames) m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
		if (uPathIDs) m_ptrKit->PQAPIFree((LONG_PTR*)uPathIDs);
		
	}
	else {
		ui->comboBox_5->clear();
	}

}

void posCal::onCalculate()
{
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_2->currentText();
	QString pathName = ui->comboBox_3->currentText();

	//获取轨迹点id
	ULONG ulPathID = 0;
	GetObjIDByName(PQ_PATH, pathName.toStdWString(), ulPathID);
	int nCount = 0;
	ULONG* ulPtIDs = nullptr;
	m_ptrKit->Path_get_point_id(ulPathID, &nCount, &ulPtIDs);
	

	//获取轨迹点信息
	PQPostureType nPostureType = QUATERNION;
	INT nPostureCount = 0;
	double* dPointPosture = nullptr;
	double dVelocity = 0;
	double dSpeedPercent = 0;
	PQPointInstruction nInstruct = PQ_LINE;
	INT nApproach = 0;
	
	double X_min, X_max, Y_min, Y_max, Z_min, Z_max = 0;

	for (int i = 0; i < nCount; i++) {
		ULONG ulPointID = ulPtIDs[i];
		m_ptrKit->PQAPIGetPointInfo(ulPointID, nPostureType, &nPostureCount, &dPointPosture,
			&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

		double x = dPointPosture[0];
		double y = dPointPosture[1];
		double z = dPointPosture[2];

		if (x > X_max) X_max = x;
		if (x < X_min) X_min = x;
		if (x > Y_max) Y_max = y;
		if (x < Y_min) Y_min = y;
		if (x > Z_max) Z_max = z;
		if (x < Z_min) Z_min = z;
	}

	
	//计算出中心点

	double x_center = (X_max + X_min) / 2;
	double y_center = (Y_max + Y_min) / 2;
	double z_center = (Z_max + Z_min) / 2;

	std::vector<double> centerPoint = { x_center, y_center, z_center};

	m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPtIDs);

	//计算得到平均法矢
	std::vector<double> dir = calculateAverageNormal(ulPathID);
	
	//对于站位来说有两种方式AGV和龙门，对于AGV来说需要确定x、y、theta，对于龙门来说需要确定z、y、z

	ULONG robotID = 0;
	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(),robotID);

	std::vector<double> robotJoints = { 0,0,M_PI/2,0,0,0 };

	std::vector<double>relativePos = calculataRelativePos(robotID, robotJoints);

	ULONG CoordinateID = 0;
	m_ptrKit->Robot_get_base_coordinate(robotID, &CoordinateID);

	//计算得到机器人新基座标位置和原来坐标轴位置
	std::vector<double> newBase = adjustRobotBasePosition(CoordinateID, centerPoint, relativePos);

	if (ui->comboBox_4->currentText() == "是") {
		
		//只移动AGV的xy以及theta
		std::vector<double> move = {newBase[0] - newBase[3],newBase[1] - newBase[4],newBase[2] - newBase[5]};
		QString pointName = ui->comboBox_5->currentText();
		
		ULONG pointID = m_AGVMap.key(pointName);
		ULONG uPathId[1] = { pointID };
		INT i_PathCount = 1;
		m_ptrKit->Path_Translation(uPathId, i_PathCount, move[0], move[1], move[2]);

		double dJoint[6] = { 0,0,0,0,0,0 };
		INT o_nPostureArraySize = 0;
		DOUBLE* o_dPosture = nullptr;
		m_ptrKit->Robot_get_forward_kinematics(robotID, dJoint, 6, QUATERNION, 0, 1, &o_nPostureArraySize,
			&o_dPosture);

		//将四元数转化为方向向量
		double w = o_dPosture[3], x = o_dPosture[4], y = o_dPosture[5], z = o_dPosture[6];
		// 计算中间量，优化性能

		std::vector<double> v = { 0.0, 0.0, 1.0 };
		double tx = 2 * (y * v[2] - z * v[1]);
		double ty = 2 * (z * v[0] - x * v[2]);
		double tz = 2 * (x * v[1] - y * v[0]);

		std::vector<double> robotdir;
		double X = v[0] + w * tx + (y * tz - z * ty);
		double Y = v[1] + w * ty + (z * tx - x * tz);
		double Z = v[2] + w * tz + (x * ty - y * tx);
		m_ptrKit->PQAPIFree((LONG_PTR*)o_dPosture);

		robotdir.push_back(X);
		robotdir.push_back(Y);
		robotdir.push_back(Z);

		//计算按AGV的坐标系旋转theta多少实现
		double theta = calculateAGVJoint(robotdir, dir, CoordinateID);

		m_ptrKit->Path_Rotate(uPathId, i_PathCount, 0, 0, theta);

	}
	else {

		//移动龙门轨道的xyz
		std::vector<double> move = { newBase[0] - newBase[3],newBase[1] - newBase[4],newBase[2] - newBase[5] };
		
		ULONG guideID = 0;
		GetObjIDByName(PQ_ROBOT,L" ",guideID);

		std::vector<std::vector<double>> guideDir = calculateJointMovementDir(guideID);

		std::vector<double> jointDeltas = calculateJointValues(move, guideDir);

		int guideCount = guideDir.size();
		//获得导轨关节值
		int nCount = 0;
		double* dJoints = nullptr;
		m_ptrKit->Doc_get_obj_joints(guideID, &nCount, &dJoints);

		std::vector<double> newGuideJoints(nCount);
		for (int i = 0; i < nCount; ++i) {
			newGuideJoints[i] = dJoints[i] + jointDeltas[i]; // 假设关节值是线性可加的
		}

		m_ptrKit->PQAPIFreeArray((LONG_PTR*)dJoints);

		//获取导轨的关节限位

		double* dLinks = nullptr;
		m_ptrKit->Doc_get_obj_links(guideID, &guideCount, &dLinks);

		for (int i = 0; i < nCount; i++) {
			if(newGuideJoints[i] < dLinks[2*i]){
				newGuideJoints[i] = dLinks[2 * i];
			}
			else if (newGuideJoints[i] > dLinks[2*i+1]) {
				newGuideJoints[i] = dLinks[2 * i + 1];
			}
		}


		m_ptrKit->PQAPIFreeArray((LONG_PTR*)dLinks);

		DOUBLE dRobotJoints[6] = { 0,0,0,0,0,0 };
		INT nRJointsCount = 6;
		DOUBLE dPositionerJoints[1] = { 0 };
		INT nPJointsCount = 1;
		DOUBLE dVelocity[1] = { 50 };
		DOUBLE dSpeedPercent[1] = { 50 };
		INT nApproach[1] = { 50 };
		INT PointCount = 1;

		ULONG uoPathID = 0;
		m_ptrKit->PQAPIAddAbsJointPath(robotID, dRobotJoints, nRJointsCount,
			newGuideJoints.data(), nCount, dPositionerJoints, nPJointsCount, dVelocity,
			dSpeedPercent, nApproach, PointCount, ulPathID, &uoPathID);


	}

}

void posCal::onShow()
{
	QString AGVPointName = ui->comboBox_5->currentText();
	ULONG AGVPoint = m_AGVMap.key(AGVPointName);
	ULONG uPathId[1] = { AGVPoint };
	INT i_PathCount = 1;
	m_ptrKit->Path_Translation(uPathId, i_PathCount, 0, 0, 20);
}

void posCal::onConfirm()
{

	reject();
}

void posCal::onCancel()
{
	reject();
}

QStringList posCal::getPathGroupNames(ULONG robotID)
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

QStringList posCal::getPathNames(ULONG robotID, const QString& groupName)
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

QMap<ULONG, QString> posCal::getObjectsByType(PQDataType objType)
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

QStringList posCal::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> posCal::extractLongArrayFromVariant(const VARIANT& variant)
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
QStringList posCal::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

void posCal::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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

// 在posCal.cpp中实现函数
std::vector<double> posCal::calculateAverageNormal(ULONG pathID)
{
	std::vector<double> averageNormal(3, 0.0); // 初始化为零向量 [x, y, z]

	int nCount = 0;
	ULONG* ulPtIDs = nullptr;

	// 获取轨迹中的所有点ID
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nCount, &ulPtIDs);
	if (FAILED(hr) || nCount == 0) {
		qDebug() << "获取轨迹点ID失败或轨迹点为空";
		return averageNormal;
	}

	int validPointCount = 0;

	for (int i = 0; i < nCount; i++) {
		ULONG ulPointID = ulPtIDs[i];

		// 获取点的姿态信息（四元数表示）
		PQPostureType nPostureType = QUATERNION;
		INT nPostureCount = 0;
		double* dPointPosture = nullptr;
		double dVelocity = 0;
		double dSpeedPercent = 0;
		PQPointInstruction nInstruct = PQ_LINE;
		INT nApproach = 0;

		hr = m_ptrKit->PQAPIGetPointInfo(ulPointID, nPostureType, &nPostureCount,
			&dPointPosture, &dVelocity, &dSpeedPercent,
			&nInstruct, &nApproach);

		if (SUCCEEDED(hr) && nPostureCount >= 4) { // 四元数需要4个分量
			// 提取四元数: [qx, qy, qz, qw]
			double qw = dPointPosture[3];
			double qx = dPointPosture[4];
			double qy = dPointPosture[5];
			double qz = dPointPosture[6];
		

			double z_x = 2.0 * (qx * qz - qw * qy);
			double z_y = 2.0 * (qy * qz + qw * qx);
			double z_z = 1.0 - 2.0 * (qx * qx + qy * qy);

			// 累加到平均法向
			averageNormal[0] += z_x;
			averageNormal[1] += z_y;
			averageNormal[2] += z_z;
			validPointCount++;

			// 释放内存
			m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
		}
	}

	// 释放点ID数组内存
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPtIDs);

	if (validPointCount > 0) {
		// 计算平均值
		averageNormal[0] /= validPointCount;
		averageNormal[1] /= validPointCount;
		averageNormal[2] /= validPointCount;

		// 归一化处理
		double length = sqrt(averageNormal[0] * averageNormal[0] +
			averageNormal[1] * averageNormal[1] +
			averageNormal[2] * averageNormal[2]);

		if (length > 0.0001) { // 避免除零
			averageNormal[0] /= length;
			averageNormal[1] /= length;
			averageNormal[2] /= length;
		}

		qDebug() << "计算平均法向成功，有效点数:" << validPointCount
			<< "法向向量: [" << averageNormal[0] << ", "
			<< averageNormal[1] << ", " << averageNormal[2] << "]";
	}
	else {
		qDebug() << "没有有效的轨迹点数据";
		// 返回默认Z轴方向
		averageNormal[0] = 0.0;
		averageNormal[1] = 0.0;
		averageNormal[2] = 1.0;
	}

	return averageNormal;
}

std::vector<double> posCal::calculataRelativePos(ULONG robotID, std::vector<double> robotJoints)
{
	std::vector<double> relativePos;
	//获取
	int nCount = 0;
	m_ptrKit->Robot_get_joints_count(robotID, &nCount);

	ULONG uCoordinateID = 0;
	m_ptrKit->Robot_get_base_coordinate(robotID, &uCoordinateID);

	INT o_nPostureArraySize = 0;
	DOUBLE* o_dPosture = nullptr;
	m_ptrKit->Robot_get_forward_kinematics(robotID, robotJoints.data(), nCount, QUATERNION,
		uCoordinateID, 0, &o_nPostureArraySize,&o_dPosture);

	for (int i = 0; i < 3; i++) {
	
		relativePos.push_back(o_dPosture[i]);
	}
	m_ptrKit->PQAPIFree((LONG_PTR*)o_dPosture);

	return relativePos;
}

std::vector<double> posCal::adjustRobotBasePosition(ULONG coordinateID, const std::vector<double>& pointWorld,
	const std::vector<double>& pointRobot)
{
	if (pointWorld.size() < 3 || pointRobot.size() < 3) {
		qDebug() << "错误：点坐标需要至少3个元素";
		return {};
	}

	// 1. 获取当前机器人基座标系在世界坐标系中的位姿
	ULONG robotID = m_robotMap.key(ui->comboBox_1->currentText(), 0);
	if (robotID == 0) {
		qDebug() << "错误：无法获取机器人ID";
		return {};
	}

	// 假设有接口获取机器人基座标系位姿
	double currentPosition[3] = { 0, 0, 0 };
	double currentOrientation[9] = { 0 }; // 3x3旋转矩阵

	int nCount = 0;
	double* dPosture = nullptr;
	m_ptrKit->Doc_get_coordinate_posture(coordinateID, QUATERNION, &nCount, &dPosture, 0);
	double qw, qx, qy, qz;
	qw = dPosture[3];
	qx = dPosture[4];
	qy = dPosture[5];
	qz = dPosture[6];

	double x, y, z;
	x = dPosture[0];
	y = dPosture[1];
	z = dPosture[2];
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);

	Eigen::Quaterniond quat(qw, qx, qy, qz);
	quat.normalize(); // 确保四元数是单位四元数

	Eigen::Matrix3d R_world_robot = quat.toRotationMatrix();

	// 3. 计算新的平移向量
	Eigen::Vector3d P_world(pointWorld[0], pointWorld[1], pointWorld[2]);
	Eigen::Vector3d P_robot(pointRobot[0], pointRobot[1], pointRobot[2]);

	Eigen::Vector3d t_world_robot_new = P_world - R_world_robot * P_robot;

	// 4. 应用新的位姿到机器人基座标系
	std::vector<double> newPosition = { t_world_robot_new[0], t_world_robot_new[1], t_world_robot_new[2] ,x ,y ,z};

	return newPosition;
}

double posCal::calculateAGVJoint(std::vector<double> robotDir, std::vector<double> pathDir, ULONG baseCoordinate)
{

	//计算出旋转角度

	//1、获取AGV坐标系位姿
	int nCount = 0;
	double* dPosture = nullptr;
	m_ptrKit->Doc_get_coordinate_posture(baseCoordinate, QUATERNION, &nCount, &dPosture, 0);
	std::vector<double> Dir;
	for (int i = 0; i < nCount; i++) {
		Dir.push_back(dPosture[i]);
	}
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);

	//2、将robotDir和pathDir投影到xy平面
	std::vector<double> normal = { 0.0, 0.0, 1.0 }; // XY平面法向量

	// 计算robotDir在XY平面的投影
	double dotProduct = robotDir[0] * normal[0] + robotDir[1] * normal[1] + robotDir[2] * normal[2];
	std::vector<double> robotProj = {
		robotDir[0] - dotProduct * normal[0],
		robotDir[1] - dotProduct * normal[1],
		robotDir[2] - dotProduct * normal[2]
	};

	// 将Z分量强制设为0，确保在XY平面
	robotProj[2] = 0.0;

	// 对pathDir进行相同操作
	dotProduct = pathDir[0] * normal[0] + pathDir[1] * normal[1] + pathDir[2] * normal[2];
	std::vector<double> pathProj = {
		pathDir[0] - dotProduct * normal[0],
		pathDir[1] - dotProduct * normal[1],
		pathDir[2] - dotProduct * normal[2]
	};
	pathProj[2] = 0.0;


	// 3、提取Dir坐标系的Z轴方向
	std::vector<double> zAxis = extractZAxisFromQuaternion(Dir);

	// 4、计算旋转角度（使robotProj与pathProj反向）
	double rotationRadians = calculateRotationToOpposite(robotProj, pathProj, zAxis);

	// 转换为角度返回
	double rotationDegrees = rotationRadians * 180.0 / M_PI;

	return rotationDegrees;
}



// 从四元数提取Z轴方向（假设四元数为[w, x, y, z]）
std::vector<double> posCal::extractZAxisFromQuaternion(const std::vector<double>& quat) {
	if (quat.size() < 4) return { 0.0, 0.0, 1.0 }; // 默认Z轴

	double w = quat[0], x = quat[1], y = quat[2], z = quat[3];
	// 四元数旋转后的Z轴方向
	std::vector<double> zAxis = {
		2 * x*z + 2 * w*y,
		2 * y*z - 2 * w*x,
		1 - 2 * x*x - 2 * y*y
	};

	// 归一化
	double length = std::sqrt(zAxis[0] * zAxis[0] + zAxis[1] * zAxis[1] + zAxis[2] * zAxis[2]);
	if (length > 1e-10) {
		zAxis[0] /= length;
		zAxis[1] /= length;
		zAxis[2] /= length;
	}
	return zAxis;
}

double posCal::calculateRotationToOpposite(const std::vector<double>& v1,
	const std::vector<double>& v2,
	const std::vector<double>& rotationAxis) {
	// 1. 提取向量在旋转轴垂直平面上的分量
	double v1x = v1[0], v1y = v1[1], v1z = v1[2];
	double v2x = v2[0], v2y = v2[1], v2z = v2[2];
	double axisx = rotationAxis[0], axisy = rotationAxis[1], axisz = rotationAxis[2];

	// 2. 将向量投影到与旋转轴垂直的平面上
	// 投影公式: v_proj = v - (v·axis) * axis
	double dot1 = v1x * axisx + v1y * axisy + v1z * axisz;
	double dot2 = v2x * axisx + v2y * axisy + v2z * axisz;

	std::vector<double> v1_proj = {
		v1x - dot1 * axisx,
		v1y - dot1 * axisy,
		v1z - dot1 * axisz
	};
	std::vector<double> v2_proj = {
		v2x - dot2 * axisx,
		v2y - dot2 * axisy,
		v2z - dot2 * axisz
	};

	// 3. 计算投影向量的夹角[2,3](@ref)
	double dx1 = v1_proj[0], dy1 = v1_proj[1];
	double dx2 = v2_proj[0], dy2 = v2_proj[1];

	// 计算点积和叉积
	double dot = dx1 * dx2 + dy1 * dy2;
	double det = dx1 * dy2 - dy1 * dx2; // 二维叉积

	// 4. 计算当前夹角
	double currentAngle = std::atan2(det, dot);

	// 5. 计算到反向所需的角度（增加180度）
	double targetAngle = currentAngle;

	// 标准化角度到[-π, π]范围
	if (targetAngle > M_PI) targetAngle -= 2 * M_PI;
	else if (targetAngle <= -M_PI) targetAngle += 2 * M_PI;

	return targetAngle;
}

/**
 * @brief 计算导轨上每个关节运动时，末端执行器的运动方向向量（单位向量）
 * @param guideID 导轨ID
 * @return 二维向量，每个子向量代表一个关节运动时末端在基坐标系下的方向单位向量
 */
std::vector<std::vector<double>> posCal::calculateJointMovementDir(ULONG guideID)
{
	std::vector<std::vector<double>> guideDir;

	// 1. 获取关节数量
	int nCount = 0;
	m_ptrKit->Doc_get_obj_joint_count(guideID, &nCount);

	// 安全检查
	if (nCount <= 0) {
		return guideDir; // 返回空向量
	}

	// 2. 获取当前关节角度（使用vector替代变长数组）
	std::vector<double> currentJoints(nCount, 0.0);
	// 这里需要替换为获取当前实际关节值的代码，例如：
	// m_ptrKit->Doc_get_obj_joint_values(guideID, currentJoints.data(), nCount);

	// 3. 计算基准位置（当前姿态下的末端位置）
	INT basePosSize = 0;
	DOUBLE* basePos = nullptr;
	m_ptrKit->Robot_get_forward_kinematics(guideID, currentJoints.data(), nCount,
		QUATERNION, 0, 1, &basePosSize, &basePos);

	if (basePosSize < 3) {
		if (basePos) m_ptrKit->PQAPIFree((LONG_PTR*)basePos);
		return guideDir; // 位置数据不足
	}

	// 4. 对每个关节进行数值微分计算
	const double delta = 0.001; // 微小位移量（弧度）

	for (int i = 0; i < nCount; i++) {
		// 4.1 创建正负偏移的关节数组
		std::vector<double> positiveJoints = currentJoints;
		std::vector<double> negativeJoints = currentJoints;

		positiveJoints[i] += delta; // 正偏移
		negativeJoints[i] -= delta; // 负偏移

		// 4.2 计算正偏移后的末端位置
		INT posPosSize = 0;
		DOUBLE* posPos = nullptr;
		m_ptrKit->Robot_get_forward_kinematics(guideID, positiveJoints.data(), nCount,
			QUATERNION, 0, 1, &posPosSize, &posPos);

		// 4.3 计算负偏移后的末端位置
		INT negPosSize = 0;
		DOUBLE* negPos = nullptr;
		m_ptrKit->Robot_get_forward_kinematics(guideID, negativeJoints.data(), nCount,
			QUATERNION, 0, 1, &negPosSize, &negPos);

		// 4.4 计算方向向量（数值微分）
		std::vector<double> dirVector(3, 0.0); // XYZ方向分量
		double magnitude = 0.0;

		if (posPosSize >= 3 && negPosSize >= 3) {
			for (int j = 0; j < 3; j++) {
				// 中心差分法求导数
				double derivative = (posPos[j] - negPos[j]) / (2 * delta);
				dirVector[j] = derivative;
				magnitude += derivative * derivative;
			}

			// 4.5 单位化方向向量
			magnitude = sqrt(magnitude);
			if (magnitude > 1e-10) { // 避免除零
				for (int j = 0; j < 3; j++) {
					dirVector[j] /= magnitude;
				}
			}

			guideDir.push_back(dirVector);
		}

		// 释放内存
		if (posPos) m_ptrKit->PQAPIFree((LONG_PTR*)posPos);
		if (negPos) m_ptrKit->PQAPIFree((LONG_PTR*)negPos);
	}

	// 5. 释放基准位置内存并返回结果
	if (basePos) m_ptrKit->PQAPIFree((LONG_PTR*)basePos);
	return guideDir;
}


std::vector<double> posCal::calculateJointValues(const std::vector<double>& move,
	const std::vector<std::vector<double>>& guideDir) {
	int numJoints = guideDir.size(); // 关节数量 n
	int spaceDim = move.size();      // 空间维度，通常是 3 (x, y, z)

	// 将数据转换为 Eigen 矩阵
	Eigen::VectorXd targetDisplacement(spaceDim);
	for (int i = 0; i < spaceDim; ++i) {
		targetDisplacement(i) = move[i];
	}

	// 构建雅可比矩阵 (Jacobian Matrix) J，其列向量就是 guideDir
	Eigen::MatrixXd Jacobian(spaceDim, numJoints);
	for (int j = 0; j < numJoints; ++j) {
		for (int i = 0; i < spaceDim; ++i) {
			Jacobian(i, j) = guideDir[j][i]; // 第 j 个关节的方向向量填入第 j 列
		}
	}

	Eigen::VectorXd deltaJoints = Jacobian.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(targetDisplacement);

	// 将结果转换回 std::vector
	std::vector<double> result;
	result.reserve(numJoints);
	for (int i = 0; i < numJoints; ++i) {
		result.push_back(deltaJoints(i));
	}

	return result;
}

