#include "ui\PositionCorrectWidget.h"
#include <iostream>
#include <QDebug>

PositionCorrectWidget::PositionCorrectWidget(CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback* ptrKitCallback, QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::PositionCorrectWidgetClass()),
	m_ptrKit(ptrKit),
	m_ptrKitCallback(ptrKitCallback)

{
	ui->setupUi(this);
	setWindowTitle(QString::fromLocal8Bit("对象偏移修正"));
	this->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	pickObj = new pickWidget(this);
	ui->loutObj->addWidget(pickObj);
	getCoordID();
	getOBJID();
	initCmbBox();
	//连接槽函数

	connect(ui->cmbObj, SIGNAL(currentIndexChanged(int)), this, SLOT(cmbObj_currentIndexChanged(int)));
	connect(ui->cmbCoord, SIGNAL(currentIndexChanged(int)), this, SLOT(cmbCoord_currentIndexChanged(int)));
}

PositionCorrectWidget::~PositionCorrectWidget()
{
	delete ui;
}

void PositionCorrectWidget::getOBJID()
{
	//函数要求：获得机器人和工件的ID和名字，存入对应的成员变量中
	//接口需求：HRESULT pq_GetAllDataObjectsByType (PQDataType i_eObjType,BSTR* o_sNames,BSTR* o_sIDs)
	
	if (!m_ptrKit)
		return;

	m_objIDs.clear();
	m_objNames.clear();

	// 首先获取机器人对象
	BSTR bsRobotNames = nullptr;
	BSTR bsRobotIDs = nullptr;

	HRESULT hr = m_ptrKit->pq_GetAllDataObjectsByType(PQ_ROBOT, &bsRobotNames, &bsRobotIDs);
	if (SUCCEEDED(hr) && bsRobotNames && bsRobotIDs)
	{
		// 将BSTR转换为QString
		QString strRobotNames = QString::fromWCharArray(bsRobotNames);
		QString strRobotIDs = QString::fromWCharArray(bsRobotIDs);

		// 按"#"分割名称和ID
		QStringList robotNameList = strRobotNames.split(QChar('#'), QString::SkipEmptyParts);
		QStringList robotIdList = strRobotIDs.split(QChar('#'), QString::SkipEmptyParts);

		// 将机器人的名称和ID分别存储到成员变量
		for (int i = 0; i < robotNameList.size() && i < robotIdList.size(); ++i)
		{
			m_objNames.push_back(robotNameList[i]);
			m_objIDs.push_back(robotIdList[i].toULong());
		}

		// 释放BSTR内存
		SysFreeString(bsRobotNames);
		SysFreeString(bsRobotIDs);
	}

	// 然后获取工件对象
	BSTR bsObjNames = nullptr;
	BSTR bsObjIDs = nullptr;

	hr = m_ptrKit->pq_GetAllDataObjectsByType(PQ_WORKINGPART, &bsObjNames, &bsObjIDs);
	if (SUCCEEDED(hr) && bsObjNames && bsObjIDs)
	{
		// 将BSTR转换为QString
		QString strObjNames = QString::fromWCharArray(bsObjNames);
		QString strObjIDs = QString::fromWCharArray(bsObjIDs);

		// 按"#"分割名称和ID
		QStringList nameList = strObjNames.split(QChar('#'), QString::SkipEmptyParts);
		QStringList idList = strObjIDs.split(QChar('#'), QString::SkipEmptyParts);

		// 将工件的名称和ID分别存储到成员变量
		for (int i = 0; i < nameList.size() && i < idList.size(); ++i)
		{
			m_objNames.push_back(nameList[i]);
			m_objIDs.push_back(idList[i].toULong());
		}

		// 释放BSTR内存
		SysFreeString(bsObjNames);
		SysFreeString(bsObjIDs);
	}
}

void PositionCorrectWidget::getCoordID()
{
	//函数要求：获得坐标系的ID和名字，存入对应的成员变量中
	
	if (!m_ptrKit)
		return;

	m_coordIDs.clear();
	m_coordNames.clear();

	// 获取坐标系对象
	BSTR bsCoordNames = nullptr;
	BSTR bsCoordIDs = nullptr;

	HRESULT hr = m_ptrKit->pq_GetAllDataObjectsByType(PQ_COORD, &bsCoordNames, &bsCoordIDs);
	if (SUCCEEDED(hr) && bsCoordNames && bsCoordIDs)
	{
		// 将BSTR转换为QString
		QString strCoordNames = QString::fromWCharArray(bsCoordNames);
		QString strCoordIDs = QString::fromWCharArray(bsCoordIDs);

		// 按"#"分割名称和ID
		QStringList nameList = strCoordNames.split(QChar('#'), QString::SkipEmptyParts);
		QStringList idList = strCoordIDs.split(QChar('#'), QString::SkipEmptyParts);

		// 将名称和ID分别存储到成员变量
		for (int i = 0; i < nameList.size() && i < idList.size(); ++i)
		{
			m_coordNames.push_back(nameList[i]);
			m_coordIDs.push_back(idList[i].toULong());
		}

		// 释放BSTR内存
		SysFreeString(bsCoordNames);
		SysFreeString(bsCoordIDs);
	}
}

void PositionCorrectWidget::initCmbBox()
{
	//要求，将m_objNames和m_coordNames同步更新到对应下拉框
	// 清空下拉框
	ui->cmbObj->clear();
	ui->cmbCoord->clear();

	// 同步对象名称到对象下拉框
	for (const QString& objName : m_objNames)
	{
		ui->cmbObj->addItem(objName);
	}

	// 同步坐标系名称到坐标系下拉框
	for (const QString& coordName : m_coordNames)
	{
		ui->cmbCoord->addItem(coordName);
	}
}

void PositionCorrectWidget::setCurrentID()
{
	m_currentObjID = m_objIDs[ui->cmbObj->currentIndex()];
	m_currentCoordID = m_coordIDs[ui->cmbCoord->currentIndex()];
}

void PositionCorrectWidget::setObjPos(const ULONG& objID,
									const ULONG& coordID,
									std::array<double, 6> correctionValues)
{
	//参考API
	//HRESULT Doc_set_obj_posture(ULONG i_ulID, DOUBLE *i_dPosture, INT i_nPostureArraySize, PQPostureType i_ePostureType,ULONG i_ulCoorID)
	//SDK:9.2.0.6420(函数本体最低支持版本) 11.1.0.7310(PQPostureType 最低支持版本)
	//Parameters:
	//  i_ulID 欲设置位姿的对象 ID
	//  i_dPosture 位姿数据
	//  i_nPostureArraySize 位姿数据数组大小
	//  i_ePostureType 指定姿态表示方式:欧拉角、四元素,详见 9.2 位姿描述定义
	//  i_ulCoorID 位姿参考坐标系,若为世界坐标系传 0
	//函数要求：应用位置修正

	if (!m_ptrKit)
		return;

	// 将std::array转换为double指针
	double dPosture[6];
	for (int i = 0; i < 6; ++i)
	{
		dPosture[i] = correctionValues[i];
	}

	// 调用API设置对象位姿
	PQPostureType ePostureType = EULERANGLEZYX;
	int nPostureCount = 6;

	HRESULT hr = m_ptrKit->Doc_set_obj_posture(objID, dPosture, nPostureCount, ePostureType, coordID);

	if (SUCCEEDED(hr))
	{
		qDebug() << "Position correction applied successfully for Object ID:" << objID;
	}
	else
	{
		qDebug() << "Failed to apply position correction. Error code:" << hr;
	}
}

void PositionCorrectWidget::cmbObj_currentIndexChanged(int index)
{
	setCurrentID();
	//打印当前选择的对象名称ID、坐标系名称ID
// 打印当前选择的对象名称ID、坐标系名称ID
	qDebug() << "Current Object ID:" << m_currentObjID<<"\n";

	
}

void PositionCorrectWidget::cmbCoord_currentIndexChanged(int index)
{
	setCurrentID();
	if (index >= 0 && index < m_objNames.size() && index < m_objIDs.size())
	{
		qDebug() << "Current Object Name:" << m_objNames[index]
			<< "ID:" << m_objIDs[index];
	}
	if (ui->cmbCoord->currentIndex() >= 0 && ui->cmbCoord->currentIndex() < m_coordNames.size() && ui->cmbCoord->currentIndex() < m_coordIDs.size())
	{
		int coordIndex = ui->cmbCoord->currentIndex();
		qDebug() << "Current Coord Name:" << m_coordNames[coordIndex]
			<< "ID:" << m_coordIDs[coordIndex];
	}
}

void PositionCorrectWidget::on_btnApply_clicked()
{
	// 实现函数，应用位置修正
	setCurrentID();

	// 从UI控件获取修正值（需要根据实际UI控件名称调整）
	// 这里假设UI中有对应的输入框：leX、leY、leZ、leRX、leRY、leRZ
	std::array<double, 6> correctionValues = {
		ui->spnX->text().toDouble(),
		ui->spnY->text().toDouble(),
		ui->spnZ->text().toDouble(),
		ui->spnRoll->text().toDouble(),
		ui->spnPitch->text().toDouble(),
		ui->spnYaw->text().toDouble()
	};

	// 验证当前选择的对象和坐标系
	if (ui->cmbObj->currentIndex() < 0 || ui->cmbCoord->currentIndex() < 0)
	{
		qDebug() << "Error: Please select an object and coordinate system first.";
		return;
	}

	// 调用setObjPos应用位置修正
	setObjPos(m_currentObjID, m_currentCoordID, correctionValues);

	qDebug() << "Position correction applied for Object ID:" << m_currentObjID
		<< "in Coordinate ID:" << m_currentCoordID;
}

void PositionCorrectWidget::on_btnTest_clicked()
{
	std::cout << "btnTestClicked" << std::endl;
	CComBSTR bsName;
	CComBSTR bsID;
	m_ptrKit->pq_GetAllDataObjectsByType(PQ_ROBOT, &bsName, &bsID);
	ulong railID;
	ulong robotID;
	const QString idText = QString::fromWCharArray(bsID);
	const QStringList idParts = idText.split(QChar('#'), QString::SkipEmptyParts);
	if (idParts.size() >= 2)
	{
		railID = idParts[0].toULong();
		robotID = idParts[1].toULong();
	}
	DOUBLE dRobJoints[18] = { 0.1745,0.1745,0.1745,0.1745,0.1745,0.1745,0,0,0,0,0,0,0,0,0,0,0,0};
	INT nRJointsCount = 18;
	DOUBLE dGuideJoints[9] = { 0,0,0,1000,0,0,2000,0,0 };
	INT nRGuideCount = 9;
	DOUBLE dPositionorJoints[1] = {0};
	INT nPJointsCount = 1;
	DOUBLE dVel[1] = { 50 };
	DOUBLE dSpeedPercent[1] = { 50 };
	INT nApproach[1] = { 50 };
	INT PointCOunt = 3;
	ULONG uiPathID = 0;
	PQPointInstruction eInstruct[1] = { PQ_ABSJOINT };
	INT instruct[1] = {PQ_ABSJOINT};
	ULONG uoPathID = 0;
	CComBSTR pathName = L"testPath";
	std::wstring groupName = L"testGrp";
	ULONG uCorID = 0;
	ULONG uPathID = 0;
	m_ptrKit->Path_insert_from_joint(robotID,dRobJoints,nRJointsCount,
	dGuideJoints,nRGuideCount,dPositionorJoints, 0,PointCOunt,
	eInstruct,dVel, dSpeedPercent, nApproach, pathName, CComBSTR(groupName.c_str()),
	uCorID, &uPathID,true);


	//i_ulRobotID    指定生成的轨迹归属于哪个机器人
	//i_dRobotJoints 各个点位机器人的关节角数据（旋转轴传弧度，平移轴传距离）
	//i_nRobotJointsSize 关节角数据数组大小
	//i_dGuideJoints 关联的导轨关节角数据（旋转轴传弧度，平移轴传距离）
	//i_nGuideJointsSize 导轨数据数组大小
	//i_dPositionerJoints 关联的变位机关节角数据（旋转轴传弧度，平移轴传距离）
	//i_nPositionerJoints
	//Size
	//变位机数据数组大小
	//i_nPointCount 输入的轨迹点个数
	//i_eInstruct 各个点处的轨迹点指令（PQ_JOINT：关节角 PQ_ABSJOINT：AbsJ）
	//i_dVelocity 轨迹点速度，按序排列
	//i_dSpeedPercent 轨迹点速度百分比，按序排列
	//i_nApproach    轨迹点轨迹逼近，按序排列
	//i_PathName    生成的轨迹名称
	//i_GroupName    生成的轨迹所属轨迹组名称
	//i_uCoordinateID 参考坐标系ID
	//o_PathID 返回生成的轨迹ID
	//i_bIsUpdate 插入点后是否更新，如果多次循环调用将i_bIsUpdate设置为False，
	//在完成所有轨迹点插入后调用Doc_update_view更新一次即可

}
