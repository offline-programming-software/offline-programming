#include "kinetic_analysis.h"
#include <algorithm>
#include <numeric>

// 将弧度转换为角度的常量
const double RAD_TO_DEG = 180.0 / M_PI;

kinetic_analysis::kinetic_analysis(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::kinetic_analysisClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);
	init();

	//链接生成动力学按钮
	connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(chartDrawing()));
}

kinetic_analysis::~kinetic_analysis()
{
	delete ui;
}

void kinetic_analysis::chartDrawing()
{
	QString pathName = ui->comboBox_3->currentText();

	ULONG pathID = 0;
	GetObjIDByName(PQ_PATH, pathName.toStdWString(), pathID);

	times = calculateTime(pathID);
	angles = calculateAngle(pathID, times);  // 新增角度计算
	vels = calculateVel(pathID, times);
	accs = calculateAcc(times, vels);

	chart1 = new QChart();
	chart2 = new QChart();
	chart3 = new QChart();

	// 设置图表标题和样式
	chart1->setTitle("关节角度随时间变化图");
	chart2->setTitle("关节角速度随时间变化图");
	chart3->setTitle("关节角加速度随时间变化图");

	// 设置图表主题和动画
	chart1->setTheme(QChart::ChartThemeBlueIcy);
	chart2->setTheme(QChart::ChartThemeBlueIcy);
	chart3->setTheme(QChart::ChartThemeBlueIcy);

	chart1->setAnimationOptions(QChart::AllAnimations);
	chart2->setAnimationOptions(QChart::AllAnimations);
	chart3->setAnimationOptions(QChart::AllAnimations);

	// 检查数据有效性
	if (times.empty() || angles.empty() || vels.empty() || accs.empty()) {
		qDebug() << "错误：数据为空，无法绘制图表";
		return;
	}

	const size_t nPoints = times.size();
	const size_t nJoints = vels[0].size(); // 从速度数据获取关节数量

	// 为每个关节创建曲线系列
	QVector<QColor> jointColors = { Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::cyan, Qt::yellow };

	for (size_t j = 0; j < nJoints; ++j) {
		// 为每个关节创建角度曲线
		QSplineSeries* angleSeries = new QSplineSeries();
		angleSeries->setName(QString("关节 %1").arg(j + 1));
		angleSeries->setColor(jointColors[j % jointColors.size()]);
		angleSeries->setPen(QPen(jointColors[j % jointColors.size()], 2));

		// 为每个关节创建角速度曲线
		QSplineSeries* velSeries = new QSplineSeries();
		velSeries->setName(QString("关节 %1").arg(j + 1));
		velSeries->setColor(jointColors[j % jointColors.size()]);
		velSeries->setPen(QPen(jointColors[j % jointColors.size()], 2));

		// 为每个关节创建角加速度曲线
		QSplineSeries* accSeries = new QSplineSeries();
		accSeries->setName(QString("关节 %1").arg(j + 1));
		accSeries->setColor(jointColors[j % jointColors.size()]);
		accSeries->setPen(QPen(jointColors[j % jointColors.size()], 2));

		// 为每个系列添加数据点
		for (size_t i = 0; i < nPoints; ++i) {
			double time = times[i];
			double angle = angles[i][j] * RAD_TO_DEG;  // 转换为角度制
			double velocity = vels[i][j] * RAD_TO_DEG;  // 转换为角度制
			double acceleration = accs[i][j] * RAD_TO_DEG;  // 转换为角度制

			angleSeries->append(time, angle);
			velSeries->append(time, velocity);
			accSeries->append(time, acceleration);
		}

		// 将系列添加到对应的图表
		chart1->addSeries(angleSeries);
		chart2->addSeries(velSeries);
		chart3->addSeries(accSeries);
	}

	// 计算数据范围
	double minTime = 0, maxTime = 0;
	double minAngle = 0, maxAngle = 0;
	double minVel = 0, maxVel = 0;
	double minAcc = 0, maxAcc = 0;

	if (!times.empty()) {
		minTime = *std::min_element(times.begin(), times.end());
		maxTime = *std::max_element(times.begin(), times.end());
	}

	// 计算所有关节的数据范围
	for (size_t i = 0; i < nPoints; ++i) {
		for (size_t j = 0; j < nJoints; ++j) {
			double angle = angles[i][j] * RAD_TO_DEG;  // 转换为角度制
			double velocity = vels[i][j] * RAD_TO_DEG;  // 转换为角度制
			double acceleration = accs[i][j] * RAD_TO_DEG;  // 转换为角度制

			if (i == 0 && j == 0) {
				minAngle = maxAngle = angle;
				minVel = maxVel = velocity;
				minAcc = maxAcc = acceleration;
			}
			else {
				minAngle = std::min(minAngle, angle);
				maxAngle = std::max(maxAngle, angle);
				minVel = std::min(minVel, velocity);
				maxVel = std::max(maxVel, velocity);
				minAcc = std::min(minAcc, acceleration);
				maxAcc = std::max(maxAcc, acceleration);
			}
		}
	}

	// 添加边距
	double timeMargin = (maxTime - minTime) * 0.05;
	double angleMargin = (maxAngle - minAngle) * 0.1;
	double velMargin = (maxVel - minVel) * 0.1;
	double accMargin = (maxAcc - minAcc) * 0.1;

	// 配置坐标轴
	configureAxis(chart1, minTime - timeMargin, maxTime + timeMargin,
		minAngle - angleMargin, maxAngle + angleMargin, "时间(s)", "角度(°)");
	configureAxis(chart2, minTime - timeMargin, maxTime + timeMargin,
		minVel - velMargin, maxVel + velMargin, "时间(s)", "角速度(°/s)");
	configureAxis(chart3, minTime - timeMargin, maxTime + timeMargin,
		minAcc - accMargin, maxAcc + accMargin, "时间(s)", "角加速度(°/s²)");

	// 设置图例
	chart1->legend()->setVisible(true);
	chart1->legend()->setAlignment(Qt::AlignBottom);
	chart2->legend()->setVisible(true);
	chart2->legend()->setAlignment(Qt::AlignBottom);
	chart3->legend()->setVisible(true);
	chart3->legend()->setAlignment(Qt::AlignBottom);

	// 将图表设置到视图并启用交互
	setupChartView(ui->angle, chart1);
	setupChartView(ui->speed, chart2);
	setupChartView(ui->acceleration, chart3);

	qDebug() << "图表绘制完成，点数:" << nPoints << "关节数:" << nJoints;
}

// 辅助函数：配置坐标轴
void kinetic_analysis::configureAxis(QChart* chart, double xMin, double xMax,
	double yMin, double yMax, const QString& xTitle, const QString& yTitle)
{
	QValueAxis *axisX = new QValueAxis;
	QValueAxis *axisY = new QValueAxis;

	axisX->setRange(xMin, xMax);
	axisY->setRange(yMin, yMax);
	axisX->setTitleText(xTitle);
	axisY->setTitleText(yTitle);

	// 设置网格线样式
	axisX->setGridLineVisible(true);
	axisY->setGridLineVisible(true);
	axisX->setGridLinePen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
	axisY->setGridLinePen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));

	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);

	// 将系列关联到坐标轴
	foreach(QAbstractSeries* series, chart->series()) {
		series->attachAxis(axisX);
		series->attachAxis(axisY);
	}
}

// 辅助函数：设置图表视图属性
void kinetic_analysis::setupChartView(QChartView* chartView, QChart* chart)
{
	chartView->setChart(chart);
	chartView->setRenderHint(QPainter::Antialiasing, true);

	//// 启用交互功能
	//chartView->setRubberBand(QChartView::RectangleRubberBand);
	//chartView->setDragMode(QGraphicsView::ScrollHandDrag);
}

void kinetic_analysis::init()
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
		this, &kinetic_analysis::onComboBox1CurrentIndexChanged);
	connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &kinetic_analysis::onComboBox2CurrentIndexChanged);
}

std::vector<double> kinetic_analysis::calculateTime(ULONG pathID)
{
	std::vector<double> timeStamps;

	// 1. 获取轨迹点信息
	int nCount = 0;
	ULONG* pointIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nCount, &pointIDs);

	if (FAILED(hr) || nCount <= 0) {
		qDebug() << "获取轨迹点ID失败，路径ID:" << pathID << "错误码:" << hr;
		return timeStamps;
	}

	std::vector<ULONG> PointIDs;

	for (int i = 0; i < nCount; i++) {
		PointIDs.push_back(pointIDs[i]);
	}

	m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointIDs);


	// 2. 存储每个点的位置和速度信息
	std::vector<std::vector<double>> positions;  // 位置坐标 (x, y, z)
	std::vector<double> velocities;              // 速度值

	for (int i = 0; i < nCount; i++) {
		PQPostureType nPostureType = QUATERNION;
		INT nPostureCount = 7;
		double* dPointPosture = nullptr;
		double dVelocity = 0;
		double dSpeedPercent = 0;
		PQPointInstruction nInstruct = PQ_LINE;
		INT nApproach = 0;

		hr = m_ptrKit->PQAPIGetPointInfo(PointIDs[i], nPostureType, &nPostureCount,
			&dPointPosture, &dVelocity, &dSpeedPercent,
			&nInstruct, &nApproach);

		if (SUCCEEDED(hr) && dPointPosture != nullptr && nPostureCount >= 3) {
			// 存储当前位置 (x, y, z) 和速度
			std::vector<double> currentPos = { dPointPosture[0], dPointPosture[1], dPointPosture[2] };
			positions.push_back(currentPos);
			velocities.push_back(dVelocity);
		}
		else {
			qDebug() << "获取轨迹点信息失败，点ID:" << pointIDs[i] << "错误码:" << hr;
			// 添加默认值以避免数组越界
			positions.push_back({ 0.0, 0.0, 0.0 });
			velocities.push_back(0.1); // 默认低速
		}

		if (dPointPosture) {
			m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
		}
	}

	// 3. 检查数据有效性
	if (positions.size() <= 1) {
		qDebug() << "有效的轨迹点数量不足，无法计算时间";
		if (positions.size() == 1) {
			timeStamps.push_back(0.0); // 只有一个点，时间为0
		}
		return timeStamps;
	}

	// 4. 计算时间戳（核心算法）
	timeStamps.push_back(0.0); // 第一个点的时间为0
	double totalTime = 0.0;

	for (size_t i = 1; i < positions.size(); i++) {
		// 计算当前段与上一段点的距离
		double dx = positions[i][0] - positions[i - 1][0];
		double dy = positions[i][1] - positions[i - 1][1];
		double dz = positions[i][2] - positions[i - 1][2];
		double distance = sqrt(dx*dx + dy * dy + dz * dz);

		// 使用当前点（即下一个轨迹点）的速度计算时间
		double currentVelocity = velocities[i];
		double segmentTime = 0.0;

		if (currentVelocity > 1e-6) { // 避免除以0
			segmentTime = distance / currentVelocity;
		}
		else {
			// 如果速度接近0，使用最小时间间隔或默认处理
			segmentTime = 0.1; // 100ms默认间隔
			qDebug() << "警告：点" << i << "速度接近0，使用默认时间间隔";
		}

		// 确保时间间隔非负
		segmentTime = std::max(segmentTime, 0.0);
		totalTime += segmentTime;
		timeStamps.push_back(totalTime);
	}

	qDebug() << "时间计算完成，路径ID:" << pathID
		<< "点数:" << timeStamps.size()
		<< "总时间:" << totalTime << "秒";

	return timeStamps;
}

// 新增函数：计算关节角度
std::vector<std::vector<double>> kinetic_analysis::calculateAngle(ULONG pathID, const std::vector<double>& time)
{
	std::vector<std::vector<double>> angles;

	// 1. 输入验证增强
	if (time.empty()) {
		qDebug() << "错误：时间向量为空，路径ID:" << pathID;
		return angles;
	}

	// 2. 获取轨迹点信息
	int nPointsCount = 0;
	ULONG* ulPointsIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nPointsCount, &ulPointsIDs);

	if (FAILED(hr) || nPointsCount <= 0 || time.size() != static_cast<size_t>(nPointsCount)) {
		qDebug() << "获取轨迹点失败或数据不匹配，路径ID:" << pathID
			<< "点数:" << nPointsCount << "时间点数:" << time.size();
		if (ulPointsIDs) {
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);
		}
		return angles;
	}

	// 3. 使用RAII模式管理资源
	struct PointsGuard {
		ULONG* points;
		kinetic_analysis* analyzer;
		PointsGuard(ULONG* p, kinetic_analysis* a) : points(p), analyzer(a) {}
		~PointsGuard() {
			if (points) analyzer->m_ptrKit->PQAPIFreeArray((LONG_PTR*)points);
		}
	} guard(ulPointsIDs, this);

	// 4. 获取所有轨迹点的关节角度数据
	std::vector<std::vector<double>> jointAngles;
	jointAngles.reserve(nPointsCount);

	for (int i = 0; i < nPointsCount; i++) {
		VARIANT varJointsArray;
		VariantInit(&varJointsArray);

		hr = m_ptrKit->PQAPIGetRobotJointsFromPoints(ulPointsIDs[i], &varJointsArray);

		if (SUCCEEDED(hr) && varJointsArray.vt == (VT_ARRAY | VT_R8)) {
			SAFEARRAY* psa = varJointsArray.parray;
			double* pData = nullptr;

			if (SUCCEEDED(SafeArrayAccessData(psa, (void**)&pData))) {
				LONG lBound, uBound;
				SafeArrayGetLBound(psa, 1, &lBound);
				SafeArrayGetUBound(psa, 1, &uBound);
				int jointCount = uBound - lBound + 1;

				std::vector<double> angles(jointCount);
				for (int j = 0; j < jointCount; j++) {
					angles[j] = pData[j];
				}
				jointAngles.push_back(std::move(angles));

				SafeArrayUnaccessData(psa);
			}
		}
		else {
			qDebug() << "获取关节角度失败，点ID:" << ulPointsIDs[i] << "错误码:" << hr;
			// 添加默认角度值
			jointAngles.push_back(std::vector<double>(6, 0.0));
		}
		VariantClear(&varJointsArray);
	}

	// 5. 数据有效性检查
	if (jointAngles.empty()) {
		qDebug() << "错误：未获取到有效的关节角度数据";
		return angles;
	}

	const int jointCount = jointAngles[0].size();
	if (jointCount == 0) {
		qDebug() << "错误：关节数量为0";
		return angles;
	}

	// 检查所有点具有相同关节数量
	for (size_t i = 1; i < jointAngles.size(); i++) {
		if (jointAngles[i].size() != static_cast<size_t>(jointCount)) {
			qDebug() << "错误：点" << i << "的关节数量不一致";
			return angles;
		}
	}

	// 6. 初始化输出向量
	angles.resize(nPointsCount, std::vector<double>(jointCount, 0.0));

	// 7. 复制关节角度数据
	for (int i = 0; i < nPointsCount; i++) {
		for (int j = 0; j < jointCount; j++) {
			angles[i][j] = jointAngles[i][j];
		}
	}

	qDebug() << "关节角度计算完成，路径ID:" << pathID
		<< "点数:" << nPointsCount << "关节数:" << jointCount;

	return angles;
}

std::vector<std::vector<double>> kinetic_analysis::calculateVel(ULONG pathID, const std::vector<double>& time)
{
	std::vector<std::vector<double>> velocities;

	// 1. 输入验证增强
	if (time.empty()) {
		qDebug() << "错误：时间向量为空，路径ID:" << pathID;
		return velocities;
	}

	// 2. 获取轨迹点信息
	int nPointsCount = 0;
	ULONG* ulPointsIDs = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathID, &nPointsCount, &ulPointsIDs);

	if (FAILED(hr) || nPointsCount <= 0 || time.size() != static_cast<size_t>(nPointsCount)) {
		qDebug() << "获取轨迹点失败或数据不匹配，路径ID:" << pathID
			<< "点数:" << nPointsCount << "时间点数:" << time.size();
		if (ulPointsIDs) {
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);
		}
		return velocities;
	}

	// 3. 使用RAII模式管理资源
	struct PointsGuard {
		ULONG* points;
		kinetic_analysis* analyzer;
		PointsGuard(ULONG* p, kinetic_analysis* a) : points(p), analyzer(a) {}
		~PointsGuard() {
			if (points) analyzer->m_ptrKit->PQAPIFreeArray((LONG_PTR*)points);
		}
	} guard(ulPointsIDs, this);

	// 4. 获取所有轨迹点的关节角度数据
	std::vector<std::vector<double>> jointAngles;
	jointAngles.reserve(nPointsCount);

	for (int i = 0; i < nPointsCount; i++) {
		VARIANT varJointsArray;
		VariantInit(&varJointsArray);

		hr = m_ptrKit->PQAPIGetRobotJointsFromPoints(ulPointsIDs[i], &varJointsArray);

		if (SUCCEEDED(hr) && varJointsArray.vt == (VT_ARRAY | VT_R8)) {
			SAFEARRAY* psa = varJointsArray.parray;
			double* pData = nullptr;

			if (SUCCEEDED(SafeArrayAccessData(psa, (void**)&pData))) {
				LONG lBound, uBound;
				SafeArrayGetLBound(psa, 1, &lBound);
				SafeArrayGetUBound(psa, 1, &uBound);
				int jointCount = uBound - lBound + 1;

				std::vector<double> angles(jointCount);
				for (int j = 0; j < jointCount; j++) {
					angles[j] = pData[j];
				}
				jointAngles.push_back(std::move(angles));

				SafeArrayUnaccessData(psa);
			}
		}
		else {
			qDebug() << "获取关节角度失败，点ID:" << ulPointsIDs[i] << "错误码:" << hr;
			// 添加默认角度值
			jointAngles.push_back(std::vector<double>(6, 0.0));
		}
		VariantClear(&varJointsArray);
	}

	// 5. 数据有效性检查
	if (jointAngles.empty()) {
		qDebug() << "错误：未获取到有效的关节角度数据";
		return velocities;
	}

	const int jointCount = jointAngles[0].size();
	if (jointCount == 0) {
		qDebug() << "错误：关节数量为0";
		return velocities;
	}

	// 检查所有点具有相同关节数量
	for (size_t i = 1; i < jointAngles.size(); i++) {
		if (jointAngles[i].size() != static_cast<size_t>(jointCount)) {
			qDebug() << "错误：点" << i << "的关节数量不一致";
			return velocities;
		}
	}

	// 6. 初始化输出向量
	velocities.resize(nPointsCount, std::vector<double>(jointCount, 0.0));

	// 7. 使用改进的差分法计算角速度 - 添加数值平滑
	for (int j = 0; j < jointCount; j++) {
		for (int i = 0; i < nPointsCount; i++) {
			double angularVelocity = 0.0;

			try {
				if (i == 0 && nPointsCount > 1) {
					// 第一个点：前向差分
					double deltaAngle = jointAngles[i + 1][j] - jointAngles[i][j];
					double deltaTime = time[i + 1] - time[i];
					if (std::abs(deltaTime) > 1e-6) {
						angularVelocity = deltaAngle / deltaTime;
					}
				}
				else if (i == nPointsCount - 1 && nPointsCount > 1) {
					// 最后一个点：后向差分
					double deltaAngle = jointAngles[i][j] - jointAngles[i - 1][j];
					double deltaTime = time[i] - time[i - 1];
					if (std::abs(deltaTime) > 1e-6) {
						angularVelocity = deltaAngle / deltaTime;
					}
				}
				else if (nPointsCount > 2) {
					// 中间点：中心差分（更高精度）
					double deltaAngle = jointAngles[i + 1][j] - jointAngles[i - 1][j];
					double deltaTime = time[i + 1] - time[i - 1];
					if (std::abs(deltaTime) > 1e-6) {
						angularVelocity = deltaAngle / deltaTime;
					}
				}

				// 应用Savitzky-Golay平滑滤波器（简化版）来减少噪声
				if (i > 0 && i < nPointsCount - 1) {
					// 使用三点平滑
					double prevVel = (i > 1) ?
						(jointAngles[i][j] - jointAngles[i - 2][j]) / (time[i] - time[i - 2]) : angularVelocity;
					double nextVel = (i < nPointsCount - 2) ?
						(jointAngles[i + 2][j] - jointAngles[i][j]) / (time[i + 2] - time[i]) : angularVelocity;

					angularVelocity = (prevVel + 2 * angularVelocity + nextVel) / 4.0;
				}

				// 角速度限幅（物理约束）
				const double MAX_ANGULAR_VELOCITY = 10.0; // 最大角速度限制(rad/s)
				angularVelocity = std::max(-MAX_ANGULAR_VELOCITY,
					std::min(angularVelocity, MAX_ANGULAR_VELOCITY));

			}
			catch (const std::out_of_range& e) {
				qDebug() << "数组访问越界 at point" << i << "joint" << j << ":" << e.what();
				angularVelocity = 0.0;
			}

			velocities[i][j] = angularVelocity;
		}
	}

	qDebug() << "角速度计算完成，路径ID:" << pathID
		<< "点数:" << nPointsCount << "关节数:" << jointCount;

	return velocities;
}

std::vector<std::vector<double>> kinetic_analysis::calculateAcc(
	const std::vector<double>& time,
	const std::vector<std::vector<double>>& vels)
{
	std::vector<std::vector<double>> accelerations;

	// 1. 增强输入验证
	if (time.empty() || vels.empty()) {
		qDebug() << "错误：输入数据为空，时间点数量:" << time.size()
			<< "速度数据点数:" << vels.size();
		return accelerations;
	}

	const size_t nPoints = time.size();
	if (nPoints != vels.size()) {
		qDebug() << "错误：时间点数量与速度数据点数不匹配，时间点数:" << nPoints
			<< "速度点数:" << vels.size();
		return accelerations;
	}

	if (nPoints < 2) {
		qDebug() << "错误：至少需要2个时间点才能计算加速度，当前点数:" << nPoints;
		return accelerations;
	}

	// 检查速度数据的关节数量一致性
	const size_t nJoints = vels[0].size();
	for (size_t i = 1; i < nPoints; ++i) {
		if (vels[i].size() != nJoints) {
			qDebug() << "错误：速度数据关节数量不一致，点" << i
				<< "的关节数:" << vels[i].size() << "期望:" << nJoints;
			return accelerations;
		}
	}

	if (nJoints == 0) {
		qDebug() << "错误：关节数量为0";
		return accelerations;
	}

	// 2. 预分配内存
	accelerations.resize(nPoints);
	for (size_t i = 0; i < nPoints; ++i) {
		accelerations[i].resize(nJoints, 0.0);
	}

	// 3. 物理约束参数
	const double MAX_ACCELERATION = 1000.0; // 最大角加速度(rad/s²)

	// 4. 使用改进的差分法计算角加速度 - 添加数值平滑
	for (size_t j = 0; j < nJoints; ++j) {
		for (size_t i = 0; i < nPoints; ++i) {
			double angularAcceleration = 0.0;

			// 根据点的位置选择差分方法
			if (i == 0) {
				// 第一个点：前向差分
				if (i < nPoints - 1) {
					const double deltaVel = vels[i + 1][j] - vels[i][j];
					const double deltaTime = time[i + 1] - time[i];
					if (std::abs(deltaTime) > 1e-6) {
						angularAcceleration = deltaVel / deltaTime;
					}
				}
			}
			else if (i == nPoints - 1) {
				// 最后一个点：后向差分
				if (i > 0) {
					const double deltaVel = vels[i][j] - vels[i - 1][j];
					const double deltaTime = time[i] - time[i - 1];
					if (std::abs(deltaTime) > 1e-6) {
						angularAcceleration = deltaVel / deltaTime;
					}
				}
			}
			else {
				// 中间点：中心差分（更高精度）
				const double deltaVel = vels[i + 1][j] - vels[i - 1][j];
				const double deltaTime = time[i + 1] - time[i - 1];
				if (std::abs(deltaTime) > 1e-6) {
					angularAcceleration = deltaVel / deltaTime;
				}
			}

			// 应用平滑处理以减少数值噪声
			if (i > 0 && i < nPoints - 1) {
				// 使用三点平滑
				double prevAcc = (i > 1) ?
					(vels[i][j] - vels[i - 2][j]) / (time[i] - time[i - 2]) : angularAcceleration;
				double nextAcc = (i < nPoints - 2) ?
					(vels[i + 2][j] - vels[i][j]) / (time[i + 2] - time[i]) : angularAcceleration;

				angularAcceleration = (prevAcc + 2 * angularAcceleration + nextAcc) / 4.0;
			}

			// 物理约束：角加速度限幅
			angularAcceleration = std::max(-MAX_ACCELERATION,
				std::min(angularAcceleration, MAX_ACCELERATION));

			accelerations[i][j] = angularAcceleration;
		}
	}

	qDebug() << "角加速度计算完成，点数:" << nPoints << "关节数:" << nJoints;
	return accelerations;
}

QMap<ULONG, QString> kinetic_analysis::getObjectsByType(PQDataType objType)
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

QStringList kinetic_analysis::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> kinetic_analysis::extractLongArrayFromVariant(const VARIANT& variant)
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

QStringList kinetic_analysis::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

void kinetic_analysis::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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

QStringList kinetic_analysis::getPathGroupNames(ULONG robotID)
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

QStringList kinetic_analysis::getPathNames(ULONG robotID, const QString& groupName)
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

void kinetic_analysis::onComboBox1CurrentIndexChanged(int index)
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

void kinetic_analysis::onComboBox2CurrentIndexChanged(int index)
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
