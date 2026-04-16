#include "posCal.h"
#include "parseJSON.h"
#include "robxFileIO.h"
#include <comdef.h>
#include <oaidl.h>
#include <fstream>
#include <QMessageBox>
#include <sstream>
#include <stdexcept>

#include <QFile>
#include <QTextStream>
#include <QDateTime>

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

	QString relations = m_tempDir + "relations.json";
	// 加载机器人关系数据
	relationsMap = loadRobotRelations(relations.toStdString());

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

std::map<std::string, std::pair<std::string, std::string>> posCal::loadRobotRelations(const std::string & filePath)
{
	std::map<std::string, std::pair<std::string, std::string>> relationsMap;
	std::ifstream file(filePath);

	// 1. 检查文件是否打开成功
	if (!file.is_open()) {
		qWarning() << "警告: 无法打开关系文件" << QString::fromStdString(filePath) << "，将使用默认值。";
		return relationsMap; // 返回空地图
	}

	try {
		json data;
		file >> data;
		file.close();

		// 2. 确保根节点是数组
		if (!data.is_array()) {
			qWarning() << "警告: relations.json 根节点不是数组，解析跳过。";
			return relationsMap;
		}

		// 3. 遍历数组
		for (const auto& item : data) {
			// 确保每一项也是数组，且至少有一个元素（机器人名称）
			if (item.is_array() && item.size() >= 1) {
				std::string robotName = item[0].get<std::string>();

				std::string railName = "无";
				std::string agvName = "无";

				// 读取第二个元素 (导轨)，如果存在且不为空
				if (item.size() > 1 && !item[1].get<std::string>().empty()) {
					railName = item[1].get<std::string>();
				}

				// 读取第三个元素 (AGV)，如果存在且不为空
				if (item.size() > 2 && !item[2].get<std::string>().empty()) {
					agvName = item[2].get<std::string>();
				}

				// 存入 Map，方便后续通过机器人名称快速查找
				relationsMap[robotName] = std::make_pair(railName, agvName);
			}
		}
	}
	catch (const std::exception& e) {
		qCritical() << "解析 relations.json 失败:" << e.what();
	}

	return relationsMap;
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

	//将AGV名称添加到控件textBrowser中
	std::string robotStdName = robotName.toStdString();
	auto it = relationsMap.find(robotStdName);
	QString selectedAGV;
	if (it != relationsMap.end()) {
		const auto& [railName, agvName] = it->second;
		selectedAGV = QString::fromStdString(agvName);
	}
	else {
		//// 提示该机器人没有AGV或导轨关系
		//QMessageBox::warning(nullptr, "警告", "没有机器人创建关系");
	}

	ui->textBrowser->setPlainText(selectedAGV);
	const QString agvText = selectedAGV.trimmed();
	const bool hasAgvBinding = !agvText.isEmpty()
		&& agvText.compare(QStringLiteral("无"), Qt::CaseInsensitive) != 0;
	ui->comboBox_4->setEnabled(hasAgvBinding);
	ui->textEdit->setEnabled(hasAgvBinding);


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

	const QString isAGV = ui->comboBox_4->currentText();

	if (isAGV == "是") {
		ui->textEdit->setPlainText("站位点");
		// 原有 AGV 路径加载逻辑保留（如需）
	}
	else {
		ui->textEdit->clear();
	}

}

void posCal::onCalculate()
{
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_2->currentText();
	QString pathName = ui->comboBox_3->currentText();

	if (robotName.isEmpty() || groupName.isEmpty() || pathName.isEmpty()) {
		QMessageBox::warning(this, tr("提示"), tr("请先选择机器人、轨迹组和轨迹。"));
		return;
	}

	ULONG ulPathID = 0;
	GetObjIDByName(PQ_PATH, pathName.toStdWString(), ulPathID);

	int pointCount = 0;
	ULONG* ulPtIDs = nullptr;
	if (FAILED(m_ptrKit->Path_get_point_id(ulPathID, &pointCount, &ulPtIDs)) || pointCount <= 0 || !ulPtIDs) {
		QMessageBox::warning(this, tr("提示"), tr("当前轨迹没有可用的路径点。"));
		return;
	}

	double minX = std::numeric_limits<double>::max();
	double maxX = std::numeric_limits<double>::lowest();
	double minY = std::numeric_limits<double>::max();
	double maxY = std::numeric_limits<double>::lowest();
	double minZ = std::numeric_limits<double>::max();
	double maxZ = std::numeric_limits<double>::lowest();
	bool hasValidPoint = false;

	for (int i = 0; i < pointCount; ++i) {
		PQPostureType nPostureType = QUATERNION;
		INT nPostureCount = 0;
		double* dPointPosture = nullptr;
		double dVelocity = 0.0;
		double dSpeedPercent = 0.0;
		PQPointInstruction nInstruct = PQ_LINE;
		INT nApproach = 0;

		if (FAILED(m_ptrKit->PQAPIGetPointInfo(ulPtIDs[i], nPostureType, &nPostureCount, &dPointPosture,
			&dVelocity, &dSpeedPercent, &nInstruct, &nApproach)) || !dPointPosture) {
			continue;
		}

		const double x = dPointPosture[0];
		const double y = dPointPosture[1];
		const double z = dPointPosture[2];

		minX = std::min(minX, x);
		maxX = std::max(maxX, x);
		minY = std::min(minY, y);
		maxY = std::max(maxY, y);
		minZ = std::min(minZ, z);
		maxZ = std::max(maxZ, z);

		hasValidPoint = true;

		m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
		dPointPosture = nullptr;
	}

	m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPtIDs);

	if (!hasValidPoint) {
		QMessageBox::warning(this, tr("提示"), tr("轨迹中没有有效的姿态点，无法计算站位。"));
		return;
	}

	const double x_center = (maxX + minX) * 0.5;
	const double y_center = (maxY + minY) * 0.5;
	const double z_center = (maxZ + minZ) * 0.5;
	std::vector<double> centerPoint{ x_center, y_center, z_center };

	/*std::vector<double> dir = calculateAverageNormal(ulPathID);*/
	std::vector<double> dir = calculateAveragePathDirection(ulPathID);

	ULONG robotID = 0;
	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);

	std::vector<double> robotJoints = { 0,0,0,0,0,0 };
	std::vector<double> relativePos = calculataRelativePos(robotID, robotJoints);

	ULONG coordinateID = 0;
	m_ptrKit->Robot_get_base_coordinate(robotID, &coordinateID);

	std::vector<double> newBase = adjustRobotBasePosition(coordinateID, centerPoint, relativePos);

	if (ui->comboBox_4->currentText() == "是") {
		auto readOffset = [this](QTextEdit* edit, const QString& name, bool& ok) {
			const QString text = edit->toPlainText().trimmed();
			if (text.isEmpty()) {
				return 0.0;
			}
			const double value = text.toDouble(&ok);
			if (!ok) {
				QMessageBox::warning(this, tr("提示"), tr("%1 偏置值不是有效数字。").arg(name));
			}
			return value;
			};

		bool okX = true;
		bool okY = true;
		bool okZ = true;
		const double offsetX = readOffset(ui->textEdit_x, tr("X"), okX);
		const double offsetY = readOffset(ui->textEdit_y, tr("Y"), okY);
		const double offsetZ = readOffset(ui->textEdit_z, tr("Z"), okZ);

		if (!okX || !okY || !okZ) {
			return;
		}

		newBase[0] += offsetX;
		newBase[1] += offsetY;
		newBase[2] += offsetZ;

		QString stationName = ui->textEdit->toPlainText().trimmed();
		if (stationName.isEmpty()) {
			stationName = tr("站位点");
			ui->textEdit->setPlainText(stationName);
		}

		std::vector<double> robotdir{ 1.0, 0.0, 0.0 };
		double dJoint[6] = { 0, 0, 0, 0, 0, 0 };
		INT postureSize = 0;
		DOUBLE* posture = nullptr;
		if (SUCCEEDED(m_ptrKit->Robot_get_forward_kinematics(
			robotID, dJoint, 6, QUATERNION, 0, 1, &postureSize, &posture))
			&& posture && postureSize >= 7) {
			const double w = posture[3];
			const double x = posture[4];
			const double y = posture[5];
			const double z = posture[6];
			const std::vector<double> v{ 1.0, 0.0, 0.0 };
			const double tx = 2 * (y * v[2] - z * v[1]);
			const double ty = 2 * (z * v[0] - x * v[2]);
			const double tz = 2 * (x * v[1] - y * v[0]);

			robotdir = {
				v[0] + w * tx + (y * tz - z * ty),
				v[1] + w * ty + (z * tx - x * tz),
				v[2] + w * tz + (x * ty - y * tx) };
		}
		if (posture) {
			m_ptrKit->PQAPIFree((LONG_PTR*)posture);
		}

		std::vector<double> negDir{ -dir[0], -dir[1], -dir[2] };
		const double theta = calculateAGVJoint(robotdir, negDir, coordinateID);

		auto toUtf8 = [](const QString& text) {
			return text.toUtf8().toStdString();
			};

		AgvStationInfo station;
		station.robotName = toUtf8(robotName);
		station.groupName = toUtf8(groupName);
		station.pathName = toUtf8(pathName);
		station.stationName = toUtf8(stationName);
		station.x = newBase[0];
		station.y = newBase[1];
		station.z = newBase[2];
		station.theta = theta;

		RobxIO io;
		QVector<AgvStationInfo> stations;
		io.updateData(stations, "AgvStationInfo.json");

		bool replaced = false;
		for (auto& info : stations) {
			if (info.robotName == station.robotName
				&& info.groupName == station.groupName
				&& info.pathName == station.pathName
				&& info.stationName == station.stationName) {
				info = station;
				replaced = true;
				break;
			}
		}
		if (!replaced) {
			stations.append(station);
		}
		io.writeData(stations, "AgvStationInfo.json");

		// 插入pos点（Part）
		QString agvName = ui->textBrowser->toPlainText().trimmed();
		ULONG uPartID = 0;
		if (!agvName.isEmpty() && agvName != QStringLiteral("无")) {
			GetObjIDByName(PQ_WORKINGPART, agvName.toStdWString(), uPartID);
		}

		if (uPartID == 0) {
			QMessageBox::warning(this, tr("提示"), tr("AGV对应Part对象不存在，已写入JSON但未插入pos点。"));
			return;
		}

		double i_dPosition[6] = { station.x, station.y, station.z, 0.0, 0.0, station.theta };
		PQPostureType i_ePostureType = EULERANGLEXYZ;
		DOUBLE i_dVelocity = 30;
		DOUBLE i_dpalvel = 0.1;
		INT i_nApproach[1] = { -1 };
		ULONG uPathID = 0;

		QString i_PathName = QStringLiteral("%1_%2_%3_%4")
			.arg(robotName)
			.arg(groupName)
			.arg(pathName)
			.arg(stationName);
		QString i_GroupName = QStringLiteral("Group");

		std::wstring wsPathName = i_PathName.toStdWString();
		std::wstring wsGroupName = i_GroupName.toStdWString();

		std::vector<wchar_t> pathNameBuf(wsPathName.begin(), wsPathName.end());
		pathNameBuf.push_back(L'\0');

		std::vector<wchar_t> groupNameBuf(wsGroupName.begin(), wsGroupName.end());
		groupNameBuf.push_back(L'\0');

		HRESULT hrInsert = m_ptrKit->Part_insert_point_with_posture(
			uPartID, 1, i_dPosition, i_ePostureType,
			i_dVelocity, i_dpalvel, i_nApproach,
			pathNameBuf.data(), groupNameBuf.data(), 0, 1, &uPathID);

		if (FAILED(hrInsert)) {
			QMessageBox::warning(this, tr("提示"), tr("JSON已写入，但插入pos点失败。"));
			return;
		}

		// 追加插入关节点（机器人关节=0，导轨关节=中间值）
		{
			std::string robotStdName = robotName.toStdString();
			auto itRel = relationsMap.find(robotStdName);
			if (itRel != relationsMap.end()) {
				const std::string railNameStd = itRel->second.first;
				if (!railNameStd.empty() && railNameStd != "无") {
					QString selectedRail = QString::fromStdString(railNameStd);
					ULONG guideID = 0;
					GetObjIDByName(PQ_ROBOT, selectedRail.toStdWString(), guideID);
					if (guideID != 0) {
						// 读取导轨轴限位
						int guideLinkCount = 0;
						double* guideLinks = nullptr;
						if (SUCCEEDED(m_ptrKit->Doc_get_obj_links(guideID, &guideLinkCount, &guideLinks))
							&& guideLinks && guideLinkCount >= 2) {
							const int guideJointCount = guideLinkCount / 2;
							std::vector<double> midGuideJoints(guideJointCount, 0.0);
							for (int i = 0; i < guideJointCount; ++i) {
								const double lower = guideLinks[2 * i];
								const double upper = guideLinks[2 * i + 1];
								midGuideJoints[i] = (lower + upper) * 0.5;
							}
							m_ptrKit->PQAPIFreeArray((LONG_PTR*)guideLinks);

							// 读取机器人轴数（用 links 数量 / 2）
							int robotLinkCount = 0;
							double* robotLinks = nullptr;
							int robotJointCount = 6;
							if (SUCCEEDED(m_ptrKit->Doc_get_obj_links(robotID, &robotLinkCount, &robotLinks))
								&& robotLinks && robotLinkCount >= 2) {
								robotJointCount = robotLinkCount / 2;
								m_ptrKit->PQAPIFreeArray((LONG_PTR*)robotLinks);
							}
							if (robotJointCount <= 0) {
								robotJointCount = 6;
							}

							std::vector<DOUBLE> zeroRobotJoints(robotJointCount, 0.0);
							DOUBLE dVelocity[1] = { 50 };
							DOUBLE dSpeedPercent[1] = { 50 };
							INT nApproach[1] = { 50 };
							INT pointCountOut = 1;
							ULONG uoPathID = 0;

							m_ptrKit->PQAPIAddAbsJointPath(
								robotID,
								zeroRobotJoints.data(), static_cast<INT>(zeroRobotJoints.size()),
								midGuideJoints.data(), static_cast<INT>(midGuideJoints.size()),
								nullptr, 0, dVelocity, dSpeedPercent, nApproach,
								pointCountOut, ulPathID, &uoPathID);
						}
						else if (guideLinks) {
							m_ptrKit->PQAPIFreeArray((LONG_PTR*)guideLinks);
						}
					}
				}
			}
		}

		return;
	}
	else {
		std::vector<double> move = { newBase[0] - newBase[3], newBase[1] - newBase[4], newBase[2] - newBase[5] };

		std::string robotStdName = robotName.toStdString();
		auto it = relationsMap.find(robotStdName);
		QString selectedRail;
		if (it != relationsMap.end()) {
			const auto& relation = it->second;
			selectedRail = QString::fromStdString(relation.first);
		}

		if (selectedRail.isEmpty()) {
			QMessageBox::warning(this, tr("提示"), tr("未找到与机器人绑定的导轨。"));
			return;
		}

		ULONG guideID = 0;
		GetObjIDByName(PQ_ROBOT, selectedRail.toStdWString(), guideID);
		if (guideID == 0) {
			QMessageBox::warning(this, tr("提示"), tr("导轨对象不存在。"));
			return;
		}

		const auto guideDir = calculateJointMovementDir(guideID);
		if (guideDir.empty()) {
			QMessageBox::warning(this, tr("提示"), tr("无法计算导轨的运动方向向量。"));
			return;
		}

		std::vector<double> jointDeltas = calculateJointValues(move, guideDir);
		if (jointDeltas.size() != guideDir.size()) {
			jointDeltas.resize(guideDir.size(), 0.0);
		}

		int jointCount = 0;
		double* dJoints = nullptr;
		if (FAILED(m_ptrKit->Doc_get_obj_joints(guideID, &jointCount, &dJoints)) || jointCount <= 0 || !dJoints) {
			QMessageBox::warning(this, tr("提示"), tr("无法读取导轨关节数据。"));
			if (dJoints) {
				m_ptrKit->PQAPIFreeArray((LONG_PTR*)dJoints);
			}
			return;
		}

		std::vector<double> newGuideJoints(jointCount, 0.0);
		for (int i = 0; i < jointCount; ++i) {
			const double delta = (i < static_cast<int>(jointDeltas.size())) ? jointDeltas[i] : 0.0;
			newGuideJoints[i] = dJoints[i] + delta;
		}
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)dJoints);

		int limitCount = 0;
		double* dLinks = nullptr;
		if (SUCCEEDED(m_ptrKit->Doc_get_obj_links(guideID, &limitCount, &dLinks)) && dLinks) {
			const int clampCount = std::min(limitCount, jointCount);
			for (int i = 0; i < clampCount; ++i) {
				const double lower = dLinks[2 * i];
				const double upper = dLinks[2 * i + 1];
				newGuideJoints[i] = std::max(lower, std::min(upper, newGuideJoints[i]));
			}
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)dLinks);
		}

		DOUBLE dRobotJoints[6] = { 0,0,0,0,0,0 };
		INT nRJointsCount = 6;
		DOUBLE dPositionerJoints[1] = { 0 };
		INT nPJointsCount = 1;
		DOUBLE dVelocity[1] = { 50 };
		DOUBLE dSpeedPercent[1] = { 50 };
		INT nApproach[1] = { 50 };
		INT pointCountOut = 1;

		ULONG uoPathID = 0;
		m_ptrKit->PQAPIAddAbsJointPath(robotID, dRobotJoints, nRJointsCount,
			newGuideJoints.data(), jointCount, dPositionerJoints, nPJointsCount, dVelocity,
			dSpeedPercent, nApproach, pointCountOut, ulPathID, &uoPathID);
	}
}

//void posCal::onShow()
//{
//	reject();
//}

void posCal::onShow()
{
	// 获取当前选中的机器人、路径组和路径名称
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_2->currentText();
	QString pathName = ui->comboBox_3->currentText();

	if (robotName.isEmpty() || groupName.isEmpty() || pathName.isEmpty()) {
		QMessageBox::warning(this, tr("警告"), tr("请选择机器人、路径组和路径"));
		return;
	}

	// 获取路径ID
	ULONG ulPathID = 0;
	GetObjIDByName(PQ_PATH, pathName.toStdWString(), ulPathID);

	int nPointsCount = 0;
	ULONG* ulPointsIDs = nullptr;

	// 获取路径上的所有点ID
	HRESULT hr = m_ptrKit->Path_get_point_id(ulPathID, &nPointsCount, &ulPointsIDs);
	if (FAILED(hr) || nPointsCount <= 0 || !ulPointsIDs) {
		QMessageBox::warning(this, tr("警告"), tr("当前路径没有可用的路径点"));
		return;
	}

	// 创建CSV文件
	QString fileName = QString("PathPoints_%1_%2_%3_%4.csv")
		.arg(robotName)
		.arg(groupName)
		.arg(pathName)
		.arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

	QFile csvFile(fileName);
	if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(this, tr("错误"), tr("无法创建CSV文件: %1").arg(fileName));
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);
		return;
	}

	QTextStream out(&csvFile);
	// 写入CSV头部
	out << "PointIndex,X,Y,Z,Qw,Qx,Qy,Qz,Velocity,Instruction\n";

	// 遍历所有路径点并获取信息
	for (int i = 0; i < nPointsCount; i++) {
		ULONG ulPointID = ulPointsIDs[i];

		PQPostureType nPostureType = QUATERNION; // 改为四元数
		INT nPostureCount = 0;
		double* dPointPosture = nullptr;
		double dVelocity = 0.0;
		double dSpeedPercent = 0.0; // 移除输出
		PQPointInstruction nInstruct = PQ_LINE;
		INT nApproach = 0; // 移除输出

		// 获取路径点信息
		hr = m_ptrKit->PQAPIGetPointInfo(
			ulPointID,
			nPostureType,
			&nPostureCount,
			&dPointPosture,
			&dVelocity,
			&dSpeedPercent,
			&nInstruct,
			&nApproach
		);

		if (SUCCEEDED(hr) && dPointPosture) {
			// 处理路径点信息
			double x = dPointPosture[0];
			double y = dPointPosture[1];
			double z = dPointPosture[2];

			// 如果姿态类型是四元数
			if (nPostureType == QUATERNION && nPostureCount >= 7) {
				double qw = dPointPosture[3];
				double qx = dPointPosture[4];
				double qy = dPointPosture[5];
				double qz = dPointPosture[6];

				// 输出到CSV
				out << i << ","
					<< x << "," << y << "," << z << ","
					<< qw << "," << qx << "," << qy << "," << qz << ","
					<< dVelocity << "," << static_cast<int>(nInstruct) << "\n";

				qDebug() << QString("路径点 %1: 位置=(%2, %3, %4), 四元数=(%5, %6, %7, %8), 速度=%9, 指令=%10")
					.arg(i)
					.arg(x).arg(y).arg(z)
					.arg(qw).arg(qx).arg(qy).arg(qz)
					.arg(dVelocity).arg(nInstruct);
			}
			else if (nPostureType == EULERANGLEXYZ && nPostureCount >= 6) {
				// 如果姿态类型是欧拉角，转换为四元数输出
				double rx = dPointPosture[3];
				double ry = dPointPosture[4];
				double rz = dPointPosture[5];

				// 将欧拉角转换为四元数 (ZYX顺序)
				double roll = rx * M_PI / 180.0;
				double pitch = ry * M_PI / 180.0;
				double yaw = rz * M_PI / 180.0;

				double cy = cos(yaw * 0.5);
				double sy = sin(yaw * 0.5);
				double cp = cos(pitch * 0.5);
				double sp = sin(pitch * 0.5);
				double cr = cos(roll * 0.5);
				double sr = sin(roll * 0.5);

				double qw = cy * cp * cr + sy * sp * sr;
				double qx = cy * cp * sr - sy * sp * cr;
				double qy = sy * cp * sr + cy * sp * cr;
				double qz = sy * cp * cr - cy * sp * sr;

				// 输出到CSV
				out << i << ","
					<< x << "," << y << "," << z << ","
					<< qw << "," << qx << "," << qy << "," << qz << ","
					<< dVelocity << "," << static_cast<int>(nInstruct) << "\n";

				qDebug() << QString("路径点 %1: 位置=(%2, %3, %4), 四元数=(%5, %6, %7, %8), 速度=%9, 指令=%10")
					.arg(i)
					.arg(x).arg(y).arg(z)
					.arg(qw).arg(qx).arg(qy).arg(qz)
					.arg(dVelocity).arg(nInstruct);
			}
			else {
				// 位置信息，没有姿态数据时使用单位四元数
				out << i << ","
					<< x << "," << y << "," << z << ","
					<< 1.0 << "," << 0.0 << "," << 0.0 << "," << 0.0 << ","
					<< dVelocity << "," << static_cast<int>(nInstruct) << "\n";

				qDebug() << QString("路径点 %1: 位置=(%2, %3, %4), 速度=%5, 指令=%6")
					.arg(i)
					.arg(x).arg(y).arg(z)
					.arg(dVelocity).arg(nInstruct);
			}

			// 释放内存
			m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
		}
		else {
			// 输出空数据到CSV
			out << i << ",,,,,,,,,\n";
			qDebug() << QString("获取路径点 %1 的信息失败").arg(i);
		}
	}

	// 释放路径点ID数组
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);

	// 关闭文件
	csvFile.close();

	// 显示成功消息
	QMessageBox::information(this, tr("完成"),
		tr("路径点信息已导出到文件: %1\n共导出 %2 个路径点").arg(fileName).arg(nPointsCount));
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

std::vector<double> posCal::calculateAveragePathDirection(ULONG pathID)
{
	std::vector<double> averageDir{ 0.0, 0.0, 1.0 };

	int nCount = 0;
	ULONG* ulPtIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nCount, &ulPtIDs);
	if (FAILED(hr) || nCount <= 1 || !ulPtIDs) {
		return averageDir;
	}

	std::vector<double> points;
	points.reserve(nCount * 3);

	for (int i = 0; i < nCount; ++i) {
		PQPostureType postureType = QUATERNION;
		INT postureCount = 0;
		double* dPosture = nullptr;
		double vel = 0.0;
		double sp = 0.0;
		PQPointInstruction instruct = PQ_LINE;
		INT approach = 0;

		if (SUCCEEDED(m_ptrKit->PQAPIGetPointInfo(
			ulPtIDs[i], postureType, &postureCount, &dPosture,
			&vel, &sp, &instruct, &approach)) && dPosture) {
			points.push_back(dPosture[0]);
			points.push_back(dPosture[1]);
			points.push_back(dPosture[2]);
			m_ptrKit->PQAPIFree((LONG_PTR*)dPosture);
		}
	}

	m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPtIDs);

	if (points.size() < 6) {
		return averageDir;
	}

	double sx = 0.0;
	double sy = 0.0;
	double sz = 0.0;
	int segCount = 0;

	for (size_t i = 3; i + 2 < points.size(); i += 3) {
		const double dx = points[i] - points[i - 3];
		const double dy = points[i + 1] - points[i - 2];
		const double dz = points[i + 2] - points[i - 1];
		const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (len <= 1e-6) {
			continue;
		}
		sx += dx / len;
		sy += dy / len;
		sz += dz / len;
		++segCount;
	}

	if (segCount == 0) {
		return averageDir;
	}

	const double norm = std::sqrt(sx * sx + sy * sy + sz * sz);
	if (norm > 1e-9) {
		averageDir[0] = sx / norm;
		averageDir[1] = sy / norm;
		averageDir[2] = sz / norm;
	}

	return averageDir;
}

std::vector<double> posCal::calculataRelativePos(ULONG robotID, std::vector<double> robotJoints)
{
	std::vector<double> relativePos;
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

