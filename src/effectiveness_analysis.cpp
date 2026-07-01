#include "effectiveness_analysis.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <unordered_map>
#include <cmath>

effectiveness_analysis::effectiveness_analysis(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::effectiveness_analysisClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	// 初始化成员变量
	barChart = nullptr;
	pieChart = nullptr;
	columnGraph = nullptr;
	pieGraph = nullptr;

	ui->setupUi(this);

	setWindowTitle("效能分析");//设置界面标题

	// 初始化数据
	initData();

	// 初始化图表
	chart_init();
}

effectiveness_analysis::~effectiveness_analysis()
{
	// 清理barSets
	for (auto set : barSets) {
		if (set) {
			delete set;
		}
	}
	barSets.clear();

	// 清理图表资源
	if (barChart) {
		delete barChart;
		barChart = nullptr;
	}
	if (pieChart) {
		delete pieChart;
		pieChart = nullptr;
	}

	delete ui;
}

void effectiveness_analysis::calculatePointInformation()
{
	QMap<ULONG, QString> m_robotMap;
	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		return;
	}

	for (int i = 0; i < robotNames.size(); i++) {
		ULONG robotID = m_robotMap.key(robotNames[i]);
		QStringList groupNames = getPathGroupNames(robotID);

		for (int j = 0; j < groupNames.size(); j++) {
			QStringList pathNames = getPathNames(robotID, groupNames[j]);
			for (int k = 0; k < pathNames.size(); k++) {

				ULONG ulPathID = 0;
				GetObjIDByName(PQ_PATH, pathNames[k].toStdWString(), ulPathID);
				int nPointsCount = 0;
				ULONG* ulPointsIDs = nullptr;
				m_ptrKit->Path_get_point_id(ulPathID, &nPointsCount, &ulPointsIDs);

				std::vector<pointInformation> dPointInformation;

				for (int r = 0; r < nPointsCount; r++) {  // 修复循环变量错误
					ULONG ulPointID = ulPointsIDs[r];      // 修复数组索引错误

					PQPostureType nPostureType = EULERANGLEXYZ;
					INT nPostureCount = 6;
					double* dPointPosture = nullptr;
					double dVelocity = 0;
					double dSpeedPercent = 0;
					PQPointInstruction nInstruct = PQ_LINE;
					INT nApproach = 0;

					HRESULT hr = m_ptrKit->PQAPIGetPointInfo(ulPointID, nPostureType, &nPostureCount, &dPointPosture,
						&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

					if (SUCCEEDED(hr) && dPointPosture != nullptr) {
						pointInformation dPoint;
						dPoint.setValues(dPointPosture[0], dPointPosture[1], dPointPosture[2],
							dVelocity, nInstruct);
						dPointInformation.push_back(dPoint);
					}

					if (dPointPosture) {
						m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
					}
				}

				// 将路径信息存储到新的数据结构中
				robotPathPointsMap[robotID].push_back(std::make_pair(ulPathID, dPointInformation));

				// 同时将所有点合并到机器人点信息中
				for (const auto& point : dPointInformation) {
					robotPointsMap[robotID].push_back(point);
				}

				// 释放路径点ID数组
				if (ulPointsIDs) {
					m_ptrKit->PQAPIFree((LONG_PTR*)ulPointsIDs);
				}
			}
		}
	}
}

// 计算AGV轨迹点信息
void effectiveness_analysis::calculateAGVInformation()
{
	QMap<ULONG, QString> m_agvMap;
	// 使用封装好的函数获取AGV列表
	PQDataType agvType = PQ_WORKINGPART;
	m_agvMap = getObjectsByType(agvType);

	if (m_agvMap.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的AGV！");
		return;
	}

	// 遍历所有AGV
	for (auto it = m_agvMap.constBegin(); it != m_agvMap.constEnd(); ++it) {
		ULONG agvID = it.key();
		QString agvName = it.value();

		qDebug() << "开始处理AGV:" << agvName << "ID:" << agvID;


		INT nCount = 0;
		WCHAR* whPathNames = nullptr;
		ULONG* uPathIDs = nullptr;
		m_ptrKit->Part_get_path(agvID, &nCount, &whPathNames, &uPathIDs);

		std::vector<pointInformation> dPointInformation;

		// 遍历所有点
		for (int r = 0; r < nCount; r++) {
			ULONG ulPointID = uPathIDs[r];

			int nCount = 0;
			ULONG* ulPtIDs = nullptr;
			m_ptrKit->Path_get_point_id(ulPointID, &nCount, &ulPtIDs);

			PQPostureType nPostureType = EULERANGLEXYZ;
			INT nPostureCount = 6;
			double* dPointPosture = nullptr;
			double dVelocity = 0;
			double dSpeedPercent = 0;
			PQPointInstruction nInstruct = PQ_LINE;
			INT nApproach = 0;

			HRESULT pointHr = m_ptrKit->PQAPIGetPointInfo(ulPtIDs[0], nPostureType, &nPostureCount, &dPointPosture,
				&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);

			if (SUCCEEDED(pointHr) && dPointPosture != nullptr) {
				pointInformation dPoint;
				dPoint.setValues(dPointPosture[0], dPointPosture[1], dPointPosture[2],
					dVelocity, nInstruct);
				dPointInformation.push_back(dPoint);

				qDebug() << "AGV路径" << whPathNames << "的轨迹点信息: X=" << dPointPosture[0]
					<< ", Y=" << dPointPosture[1] << ", Z=" << dPointPosture[2]
					<< ", 速度=" << dVelocity;
			}

			if (dPointPosture) {
				m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
			}
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPtIDs);
		}
		m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
		m_ptrKit->PQAPIFree((LONG_PTR*)uPathIDs);

		// 将所有点添加到AGV的点信息列表中
		for (const auto& point : dPointInformation) {
			AGVPointMap[agvID].push_back(point);
		}
	}
}

// 计算AGV总时间
// AGV时间计算函数
double effectiveness_analysis::calculateAGVTotalTime(ULONG agvID) {
	// 遍历特定AGV的所有路径信息
	auto agvIt = AGVPointMap.find(agvID);
	if (agvIt == AGVPointMap.end()) {
		qDebug() << "未找到AGV ID为" << agvID << "的路径信息";
		return 0.0;
	}

	const std::vector<pointInformation>& points = AGVPointMap[agvID];
	if (points.size() < 2) {
		qDebug() << "AGV路径点数量不足，无法计算时间";
		// 对于AGV，如果只有一个点，时间可能为0或需要特殊处理
		return 0.0;
	}

	double totalTime = 0.0;

	// 遍历相邻的点对，计算每段的运动时间
	for (size_t i = 1; i < points.size(); i++) {
		const pointInformation& prevPoint = points[i - 1];
		const pointInformation& currPoint = points[i];

		// 计算两点之间的距离
		double dx = currPoint.x - prevPoint.x;
		double dy = currPoint.y - prevPoint.y;
		double dz = currPoint.z - prevPoint.z;
		double distance = sqrt(dx*dx + dy * dy + dz * dz);

		// 使用前一个点的速度来计算这段距离的运动时间
		// 注意：速度单位需要根据实际情况确定，这里假设速度单位是mm/s
		if (prevPoint.vel > 0) {
			double segmentTime = distance / prevPoint.vel;
			totalTime += segmentTime;
		}
		else {
			// 如果速度为0或负数，跳过该段计算
			qDebug() << "警告：AGV点" << i - 1 << "的速度为0或负数，跳过该段计算";
		}
	}

	return totalTime;
}

// 计算路径喷涂距离的函数
double effectiveness_analysis::calculatePathSprayDistance(ULONG robotID, ULONG pathID) {
	// 遍历特定机器人的所有路径信息
	auto robotIt = robotPathPointsMap.find(robotID);
	if (robotIt == robotPathPointsMap.end()) {
		qDebug() << "未找到机器人ID为" << robotID << "的路径信息";
		return 0.0;
	}

	// 查找指定路径ID的点信息
	for (const auto& pathPair : robotIt->second) {
		if (pathPair.first == pathID) {
			const std::vector<pointInformation>& points = pathPair.second;
			if (points.size() < 2) {
				qDebug() << "路径点数量不足，无法计算喷涂距离";
				// 对于AGV，如果只有一个点，距离为0
				return 0.0;
			}

			double totalDistance = 0.0;

			// 遍历相邻的点对，计算每段的运动距离
			for (size_t i = 1; i < points.size(); i++) {
				const pointInformation& prevPoint = points[i - 1];
				const pointInformation& currPoint = points[i];

				// 计算两点之间的距离
				double dx = currPoint.x - prevPoint.x;
				double dy = currPoint.y - prevPoint.y;
				double dz = currPoint.z - prevPoint.z;
				double segmentDistance = sqrt(dx*dx + dy * dy + dz * dz);

				// 累加距离
				totalDistance += segmentDistance;
			}

			return totalDistance;
		}
	}

	qDebug() << "未找到机器人ID为" << robotID << "，路径ID为" << pathID << "的点信息";
	return 0.0;
}

// 计算所有路径的总喷涂距离
double effectiveness_analysis::calculateTotalSprayDistance() {
	double totalDistance = 0.0;

	for (const auto& robotPair : robotPathPointsMap) {
		for (const auto& pathPair : robotPair.second) {
			totalDistance += calculatePathSprayDistance(robotPair.first, pathPair.first);
		}
	}

	return totalDistance;
}

// 计算机器人总喷涂距离
double effectiveness_analysis::calculateRobotSprayDistance(ULONG robotID) {
	double totalDistance = 0.0;

	auto robotIt = robotPathPointsMap.find(robotID);
	if (robotIt == robotPathPointsMap.end()) {
		qDebug() << "未找到机器人ID为" << robotID << "的路径信息";
		return 0.0;
	}

	for (const auto& pathPair : robotIt->second) {
		totalDistance += calculatePathSprayDistance(robotID, pathPair.first);
	}

	return totalDistance;
}

// 计算各机器人喷涂距离占比
QMap<ULONG, double> effectiveness_analysis::calculateRobotSprayDistancePercentage() {
	QMap<ULONG, double> distancePercentage;
	double totalDistance = calculateTotalSprayDistance();

	if (totalDistance <= 0) {
		return distancePercentage;
	}

	for (const auto& robotPair : robotPathPointsMap) {
		double robotDistance = calculateRobotSprayDistance(robotPair.first);
		double percentage = (robotDistance / totalDistance) * 100.0;
		distancePercentage[robotPair.first] = percentage;
	}

	return distancePercentage;
}

// 计算路径时间的函数
double effectiveness_analysis::calculatePathTime(ULONG robotID, ULONG pathID) {
	// 遍历特定机器人的所有路径信息
	auto robotIt = robotPathPointsMap.find(robotID);
	if (robotIt == robotPathPointsMap.end()) {
		qDebug() << "未找到机器人ID为" << robotID << "的路径信息";
		return 0.0;
	}

	// 查找指定路径ID的点信息
	for (const auto& pathPair : robotIt->second) {
		if (pathPair.first == pathID) {
			const std::vector<pointInformation>& points = pathPair.second;
			if (points.size() < 2) {
				qDebug() << "路径点数量不足，无法计算时间";
				// 对于AGV，如果只有一个点，时间可能为0或需要特殊处理
				return 0.0;
			}

			double totalTime = 0.0;

			// 遍历相邻的点对，计算每段的运动时间
			for (size_t i = 1; i < points.size(); i++) {
				const pointInformation& prevPoint = points[i - 1];
				const pointInformation& currPoint = points[i];

				// 计算两点之间的距离
				double dx = currPoint.x - prevPoint.x;
				double dy = currPoint.y - prevPoint.y;
				double dz = currPoint.z - prevPoint.z;
				double distance = sqrt(dx*dx + dy * dy + dz * dz);

				// 使用前一个点的速度来计算这段距离的运动时间
				// 注意：速度单位需要根据实际情况确定，这里假设速度单位是mm/s
				if (prevPoint.vel > 0) {
					double segmentTime = distance / prevPoint.vel;
					totalTime += segmentTime;
				}
				else {
					// 如果速度为0或负数，跳过该段计算
					qDebug() << "警告：点" << i - 1 << "的速度为0或负数，跳过该段计算";
				}
			}

			return totalTime;
		}
	}

	qDebug() << "未找到机器人ID为" << robotID << "，路径ID为" << pathID << "的点信息";
	return 0.0;
}

// 计算所有路径的总时间
double effectiveness_analysis::calculateTotalTime() {
	double totalTime = 0.0;

	for (const auto& robotPair : robotPathPointsMap) {
		for (const auto& pathPair : robotPair.second) {
			totalTime += calculatePathTime(robotPair.first, pathPair.first);
		}
	}

	return totalTime;
}

// 计算机器人总时间
double effectiveness_analysis::calculateRobotTime(ULONG robotID) {
	double totalTime = 0.0;

	auto robotIt = robotPathPointsMap.find(robotID);
	if (robotIt == robotPathPointsMap.end()) {
		qDebug() << "未找到机器人ID为" << robotID << "的路径信息";
		return 0.0;
	}

	for (const auto& pathPair : robotIt->second) {
		totalTime += calculatePathTime(robotID, pathPair.first);
	}

	return totalTime;
}

// 计算各机器人时间占比
QMap<ULONG, double> effectiveness_analysis::calculateRobotTimePercentage() {
	QMap<ULONG, double> timePercentage;
	double totalTime = calculateTotalTime();

	if (totalTime <= 0) {
		return timePercentage;
	}

	for (const auto& robotPair : robotPathPointsMap) {
		double robotTime = calculateRobotTime(robotPair.first);
		double percentage = (robotTime / totalTime) * 100.0;
		timePercentage[robotPair.first] = percentage;
	}

	return timePercentage;
}

// 计算喷涂效率（距离/时间）
double effectiveness_analysis::calculateSprayEfficiency(ULONG robotID, ULONG pathID) {
	double distance = calculatePathSprayDistance(robotID, pathID);
	double time = calculatePathTime(robotID, pathID);

	if (time > 0) {
		return distance / time;  // 单位：mm/s
	}
	return 0.0;
}

// 计算平均喷涂效率
double effectiveness_analysis::calculateAverageSprayEfficiency() {
	double totalDistance = calculateTotalSprayDistance();
	double totalTime = calculateTotalTime();

	if (totalTime > 0) {
		return totalDistance / totalTime;  // 单位：mm/s
	}
	return 0.0;
}

// 获取机器人名称
QString effectiveness_analysis::getRobotName(ULONG robotID) {
	CComBSTR bsName;
	HRESULT hr = m_ptrKit->Doc_get_obj_name(robotID, &bsName);

	if (SUCCEEDED(hr) && bsName.m_str != nullptr) {
		QString name = QString::fromWCharArray(bsName);
		return name;
	}
	else {
		// 如果获取失败，返回默认名称
		return QString("机器人%1").arg(robotID);
	}
}

// 获取AGV名称
QString effectiveness_analysis::getAGVName(ULONG agvID) {
	CComBSTR bsName;
	HRESULT hr = m_ptrKit->Doc_get_obj_name(agvID, &bsName);

	if (SUCCEEDED(hr) && bsName.m_str != nullptr) {
		QString name = QString::fromWCharArray(bsName);
		return name;
	}
	else {
		// 如果获取失败，返回默认名称
		return QString("AGV%1").arg(agvID);
	}
}

void effectiveness_analysis::initData()
{
	// 清理可能存在的旧数据
	for (auto set : barSets) {
		if (set) {
			delete set;
		}
	}
	barSets.clear();
	categories.clear();

	// 计算各机器人的时间数据
	calculatePointInformation(); // 先计算机器人点信息
	calculateAGVInformation();   // 再计算AGV点信息

	// 获取机器人列表
	QMap<ULONG, QString> m_robotMap;
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	// 获取AGV列表，筛选名称为"喷涂件"的AGV
	QMap<ULONG, QString> m_agvMap;
	PQDataType agvType = PQ_WORKINGPART;
	m_agvMap = getObjectsByType(agvType);
	ULONG sprayAGVID = 0;
	for (auto it = m_agvMap.constBegin(); it != m_agvMap.constEnd(); ++it) {
		if (it.value() == "喷涂件") {
			sprayAGVID = it.key();
			break;
		}
	}

	// 创建柱状图数据集 - 包含机器人喷涂时间、AGV时间和调整时间
	QBarSet *robotTimeSet = new QBarSet(("机器人喷涂时间"));
	QBarSet *agvTimeSet = new QBarSet(("喷涂件AGV时间"));
	QBarSet *adjustTimeSet = new QBarSet(("喷涂调整时间"));

	// 根据实际机器人数量设置数据和类别
	if (!robotNames.isEmpty()) {
		for (int i = 0; i < robotNames.size(); i++) {
			ULONG robotID = m_robotMap.key(robotNames[i]);
			double robotTime = calculateRobotTime(robotID);

			// 如果是第一个机器人且存在喷涂件AGV，获取AGV时间
			if (i == 0 && sprayAGVID != 0) {
				double agvTime = calculateAGVTotalTime(sprayAGVID);
				*robotTimeSet << robotTime;
				*agvTimeSet << agvTime;
				*adjustTimeSet << 100.0; // 每个机器人添加1000s调整时间
			}
			else {
				*robotTimeSet << robotTime;
				*agvTimeSet << 0;  // 其他机器人AGV时间为0
				*adjustTimeSet << 100.0; // 每个机器人添加1000s调整时间
			}

			// 获取机器人名称并添加到类别列表
			QString robotName = getRobotName(robotID);
			categories << robotName;
		}
	}
	else {
		// 如果没有机器人，使用默认数据和类别
		categories << ("机器人1")
			<< ("机器人2")
			<< ("机器人3")
			<< ("机器人4");
		*robotTimeSet << 85 << 72 << 90 << 65;
		*agvTimeSet << 20 << 0 << 0 << 0;  // 默认AGV时间只在第一个机器人处有值
		*adjustTimeSet << 1000.0 << 1000.0 << 1000.0 << 1000.0; // 默认调整时间
	}

	barSets.append(robotTimeSet);
	barSets.append(agvTimeSet);
	barSets.append(adjustTimeSet); // 添加调整时间数据集
}

void effectiveness_analysis::chart_init()
{
	// 获取UI中的QChartView控件
	columnGraph = ui->columnGraph;
	pieGraph = ui->pieGraph;

	// 创建柱状图 - 显示喷涂时间
	createBarChart();

	// 创建扇形图 - 显示喷涂距离占比
	createPieChart();

	// 设置默认图表到对应的ChartView
	if (columnGraph && barChart) {
		columnGraph->setChart(barChart);
	}

	if (pieGraph && pieChart) {
		pieGraph->setChart(pieChart);
	}
}

void effectiveness_analysis::createBarChart()
{
	// 创建图表
	barChart = new QChart();
	barChart->setTitle(("机器人喷涂时间分析 - 柱状图"));
	barChart->setAnimationOptions(QChart::SeriesAnimations); // 添加动画效果

	// 安全检查
	if (barSets.size() < 1) {
		qDebug() << "Error: Not enough bar sets for bar chart";
		return;
	}

	// 检查数据集是否有效
	for (int i = 0; i < barSets.size(); ++i) {
		if (!barSets[i]) {
			qDebug() << "Error: barSet" << i << "is null";
			return;
		}
	}

	// 创建堆叠柱状图系列
	QStackedBarSeries *series = new QStackedBarSeries();

	// 添加数据集
	for (auto set : barSets) {
		if (set) {
			series->append(set);
		}
	}

	series->setLabelsVisible(true);
	series->setLabelsFormat(("@value"));

	// 添加到图表
	barChart->addSeries(series);

	// 设置X轴
	QBarCategoryAxis *axisX = new QBarCategoryAxis();
	axisX->append(categories);
	axisX->setTitleText(("机器人"));
	barChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// 设置Y轴
	QValueAxis *axisY = new QValueAxis();
	// 获取最大值以设置合适的范围
	double maxValue = 0;
	for (auto set : barSets) {
		if (set) {
			for (int i = 0; i < set->count(); i++) {
				if (set->at(i) > maxValue) {
					maxValue = set->at(i);
				}
			}
		}
	}
	axisY->setRange(0, maxValue * 1.2); // 设置为最大值的1.2倍
	axisY->setTitleText(("时间 (秒)"));
	axisY->setTickCount(7);
	axisY->setLabelFormat("%d");
	barChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	// 设置图例
	barChart->legend()->setVisible(true);
	barChart->legend()->setAlignment(Qt::AlignTop);

	// 为不同数据集设置不同颜色
	if (barSets.size() >= 3) {
		QBarSet *robotSet = barSets[0];
		QBarSet *agvSet = barSets[1];
		QBarSet *adjustSet = barSets[2];

		// 为机器人时间设置颜色（蓝色）
		for (int i = 0; i < robotSet->count(); i++) {
			robotSet->setColor(QColor(65, 105, 225)); // 蓝色
		}

		// 为AGV时间设置颜色（红色）
		for (int i = 0; i < agvSet->count(); i++) {
			if (agvSet->at(i) > 0) {
				agvSet->setColor(QColor(220, 20, 60)); // 红色
			}
		}

		// 为调整时间设置颜色（绿色）
		for (int i = 0; i < adjustSet->count(); i++) {
			adjustSet->setColor(QColor(34, 139, 34)); // 绿色
		}
	}
}

void effectiveness_analysis::createPieChart()
{
	// 创建图表
	pieChart = new QChart();
	pieChart->setTitle(("机器人喷涂距离占比 - 扇形图"));
	pieChart->setAnimationOptions(QChart::SeriesAnimations); // 添加动画效果

	// 计算各机器人喷涂距离占比
	QMap<ULONG, double> distancePercentage = calculateRobotSprayDistancePercentage();

	// 获取机器人列表
	QMap<ULONG, QString> m_robotMap;
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	// 创建扇形图系列
	QPieSeries *series = new QPieSeries();

	// 添加数据切片
	for (int i = 0; i < robotNames.size(); i++) {
		ULONG robotID = m_robotMap.key(robotNames[i]);
		double percentage = distancePercentage.value(robotID, 0.0);

		if (percentage > 0) {
			// 获取机器人实际名称
			QString robotName = getRobotName(robotID);
			QPieSlice *slice = series->append(robotName, percentage);
			slice->setLabelVisible(true);

			// 设置标签格式
			slice->setLabel(QString("%1\n%2%")
				.arg(robotName)
				.arg(QString::number(percentage, 'f', 1) + "%"));

			// 设置不同颜色
			switch (i % 6) {
			case 0: slice->setBrush(QColor(65, 105, 225)); break;  // 蓝色
			case 1: slice->setBrush(QColor(220, 20, 60)); break;   // 红色
			case 2: slice->setBrush(QColor(34, 139, 34)); break;   // 绿色
			case 3: slice->setBrush(QColor(255, 165, 0)); break;   // 橙色
			case 4: slice->setBrush(QColor(138, 43, 226)); break;  // 紫色
			case 5: slice->setBrush(QColor(255, 215, 0)); break;   // 金色
			}
		}
	}

	// 添加到图表
	pieChart->addSeries(series);
	pieChart->legend()->setVisible(true);
	pieChart->legend()->setAlignment(Qt::AlignRight);
}

QStringList effectiveness_analysis::getPathGroupNames(ULONG robotID)
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

QStringList effectiveness_analysis::getPathNames(ULONG robotID, const QString& groupName)
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

QMap<ULONG, QString> effectiveness_analysis::getObjectsByType(PQDataType objType)
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

QStringList effectiveness_analysis::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> effectiveness_analysis::extractLongArrayFromVariant(const VARIANT& variant)
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

QStringList effectiveness_analysis::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

void effectiveness_analysis::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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