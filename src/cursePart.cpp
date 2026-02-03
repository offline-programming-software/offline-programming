#include "cursePart.h"
#include <Eigen/Dense>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

// RobotWorkspaceHandler 类实现
void RobotWorkspaceHandler::writeRobotWorkspaceBoundary(const RobotWorkspaceBoundary& boundary) {
	json jsonData;

	// 尝试读取现有数据
	std::ifstream file(jsonFileName);
	if (file.is_open()) {
		try {
			file >> jsonData;
			file.close();
		}
		catch (...) {
			jsonData = json::array(); // 如果读取失败，初始化为空数组
		}
	}
	else {
		jsonData = json::array(); // 如果文件不存在，初始化为空数组
	}

	// 创建新的边界对象
	json newBoundary;
	newBoundary["robotID"] = boundary.robotID;
	newBoundary["thickness"] = boundary.thickness;
	newBoundary["theta"] = boundary.theta;
	newBoundary["isLink"] = boundary.isLink;
	newBoundary["CoordinateName"] = boundary.CoordinateName.toStdString();
	newBoundary["DirectionName"] = boundary.DirectionName.toStdString();

	// 转换railName向量
	json railNamesArray = json::array();
	for (const auto& name : boundary.railName) {
		railNamesArray.push_back(name.toStdString());
	}
	newBoundary["railName"] = railNamesArray;

	// 转换points向量
	json pointsArray = json::array();
	for (double point : boundary.points) {
		pointsArray.push_back(point);
	}
	newBoundary["points"] = pointsArray;

	// 添加到JSON数组中
	if (jsonData.is_array()) {
		jsonData.push_back(newBoundary);
	}
	else {
		jsonData = json::array({ newBoundary });
	}

	// 写回文件
	std::ofstream outFile(jsonFileName);
	if (outFile.is_open()) {
		outFile << jsonData.dump(4); // 格式化输出，缩进4个空格
		outFile.close();
	}
	else {
		throw std::runtime_error("Cannot write to JSON file: " + jsonFileName);
	}
}

std::vector<double> RobotWorkspaceHandler::findMatchingPoints(
	ULONG robotID,
	bool isLink,
	const QString& coordinateName,
	const QString& directionName,
	const QString& railName,
	double targetTheta,
	double targetThickness) {

	parseJSON queryHelper(jsonFileName);
	std::map<std::string, json> conditions;
	conditions["robotID"] = robotID;
	conditions["isLink"] = isLink;
	conditions["CoordinateName"] = coordinateName.toStdString();
	conditions["DirectionName"] = directionName.toStdString();

	// 修复railName匹配问题 - 将字符串转换为数组元素匹配
	conditions["railName"] = railName.toStdString();

	auto results = queryHelper.findObjectsByMultipleKeys(conditions);

	if (results.empty()) {
		return std::vector<double>(); // 返回空向量表示没有找到匹配项
	}

	// 寻找theta和thickness都大于等于目标值且最接近的记录
	std::vector<double> bestPoints;
	double minSumDiff = std::numeric_limits<double>::max(); // 最小差值总和
	bool foundMatch = false;

	for (const auto& result : results) {
		// 提取theta和thickness进行比较
		double currentTheta = result.value("theta", 0.0);
		double currentThickness = result.value("thickness", 0.0);

		// 只考虑theta和thickness都大于等于目标值的记录（向上取整逻辑）
		if (currentTheta >= targetTheta && currentThickness >= targetThickness) {
			// 计算差值总和
			double thetaDiff = currentTheta - targetTheta;
			double thicknessDiff = currentThickness - targetThickness;
			double sumDiff = thetaDiff + thicknessDiff;

			// 如果当前差值总和更小，则更新最佳匹配
			if (sumDiff < minSumDiff) {
				minSumDiff = sumDiff;
				foundMatch = true;

				// 获取对应的points数组
				if (result.contains("points") && result["points"].is_array()) {
					bestPoints.clear();
					for (const auto& point : result["points"]) {
						if (point.is_number()) {
							bestPoints.push_back(point.get<double>());
						}
					}
				}
			}
		}
	}

	// 如果没有找到满足条件的记录（theta和thickness都大于等于目标值），则返回最接近的记录
	if (!foundMatch) {
		double minEuclideanDistance = std::numeric_limits<double>::max();

		for (const auto& result : results) {
			// 提取theta和thickness进行比较
			double currentTheta = result.value("theta", 0.0);
			double currentThickness = result.value("thickness", 0.0);

			// 计算与目标值的欧几里得距离
			double euclideanDistance = std::sqrt(
				std::pow(currentTheta - targetTheta, 2) +
				std::pow(currentThickness - targetThickness, 2)
			);

			if (euclideanDistance < minEuclideanDistance) {
				minEuclideanDistance = euclideanDistance;

				// 获取对应的points数组
				if (result.contains("points") && result["points"].is_array()) {
					bestPoints.clear();
					for (const auto& point : result["points"]) {
						if (point.is_number()) {
							bestPoints.push_back(point.get<double>());
						}
					}
				}
			}
		}
	}

	return bestPoints;
}

double RobotWorkspaceHandler::findClosestPointUp(const std::vector<double>& points, double targetValue) {
	if (points.empty()) {
		return targetValue; // 如果没有points，返回目标值
	}

	// 查找大于等于目标值的最小点
	auto upperIt = std::upper_bound(points.begin(), points.end(), targetValue);

	if (upperIt != points.end()) {
		return *upperIt; // 返回第一个大于targetValue的点
	}
	else if (!points.empty()) {
		// 如果没有更大的点，则返回最大的点
		return points.back();
	}

	return targetValue; // 默认返回目标值
}

std::vector<double> RobotWorkspaceHandler::processRobotWorkspaceQuery(
	const QString& robotName,
	ULONG robotID,
	const QString& coordinateName,
	const QString& directionName,
	bool isLink,
	double theta,
	double thickness,
	const QString& railName) {

	// 首先查找匹配的记录
	auto matchingPoints = findMatchingPoints(
		robotID, isLink, coordinateName, directionName, railName, theta, thickness
	);

	if (matchingPoints.empty()) {
		return matchingPoints; // 返回空向量
	}

	// 直接返回匹配到的points，不再进行向上取整
	return matchingPoints;
}

cursePart::cursePart(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::cursePartClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);

	// stackedWidget默认显示page为0页
	ui->stackedWidget->setCurrentIndex(0);

	// 设置图形场景
	setupGraphicsScenes();

	// 设置步骤解释
	setStepsExplanation();

	//初始化数据
	init();
	//上一页按钮
	QPushButton* prevButton = ui->pushButton_1;
	connect(prevButton, &QPushButton::clicked, this, &cursePart::on_prev_page_clicked);

	//下一页按钮
	QPushButton* nextButton = ui->pushButton_2;
	connect(nextButton, &QPushButton::clicked, this, &cursePart::on_next_page_clicked);

	//预览按钮
	QPushButton* previewButton = ui->pushButton_3;
	connect(previewButton, &QPushButton::clicked, this, &cursePart::on_confirm_clicked);

	//确认按钮
	QPushButton* confirmButton = ui->pushButton_4;
	connect(confirmButton, &QPushButton::clicked, this, &cursePart::on_cancel_clicked);
	ui->pushButton_4->setEnabled(false);

	//取消按钮
	QPushButton* cancelButton = ui->pushButton_5;
	connect(cancelButton, &QPushButton::clicked, this, &cursePart::on_cancel_clicked);

	// 功能按钮

	connect(ui->pushButton_6, &QPushButton::clicked, this, &cursePart::on_pickUpButton_clicked);//拾取
	connect(ui->pushButton_7, &QPushButton::clicked, this, &cursePart::on_deleteButton_clicked);//删除曲面
	connect(ui->pushButton_8, &QPushButton::clicked, this, &cursePart::on_finishButton_clicked);//结束拾取模式

	connect(ui->pushButton_3, &QPushButton::clicked, this, &cursePart::on_previewButton_clicked);//预览
	connect(ui->pushButton_9, &QPushButton::clicked, this, &cursePart::on_spaceSettingButton_clicked);//计算最大偏差角和厚度

	connect(ui->pushButton_10, &QPushButton::clicked, this, &cursePart::on_calculate_workspace);//计算出划分区域长和宽

	// 组合框和文本编辑框
	connect(ui->comboBox_1, &QComboBox::currentTextChanged, this, &cursePart::on_comboBox_currentTextChanged);
	connect(ui->comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &cursePart::on_coordanateTextChanged);

	//设置是否联动
	connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool checked) {
		ui->comboBox_2->setEnabled(checked);
	});

	connect(ui->horizontalSlider, &QSlider::valueChanged, this, &cursePart::on_horizontalSlider_valueChanged);
	connect(ui->verticalSlider, &QSlider::valueChanged, this, &cursePart::on_verticalSlider_valueChanged);

	connect(m_ptrKitCallback, &CPQKitCallback::signalDraw, this, &cursePart::OnDraw);//绘制划分区域分界线
	connect(m_ptrKitCallback, &CPQKitCallback::signalElementPickup, this, &cursePart::OnElementPickup);//拾取
}

cursePart::~cursePart()
{
	delete ui;
	if (m_workspaceHandler) {
		delete m_workspaceHandler;
		m_workspaceHandler = nullptr;
	}
}


//设置界面参数
void cursePart::init() {

	isPickupActive = false; // 重置拾取状态
	isPreview = false;//是否进行预览

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		delete ui;
		return;
	}

	// 将机器人名称设置到对话框中
	ui->comboBox_1->addItems(robotNames);
	if (!robotNames.isEmpty()) {
		ui->comboBox_1->setCurrentIndex(0);
	}

	// 初始设置轨道信息
	QString currentRobot = robotNames.isEmpty() ? "" : robotNames.first();
	updateRailOptions(currentRobot, m_robotMap);


	// 设置坐标系
	PQDataType CoodernateType = PQ_COORD;
	CoodernateMap = getObjectsByType(CoodernateType);

	// 创建一个新的QMap，先插入"世界坐标系"，再插入原有的数据
	QMap<ULONG, QString> newCoodernateMap;
	newCoodernateMap.insert(0, "世界坐标系");  // 先插入首位

	// 将原有数据插入到后面（键值从1开始）
	for (auto it = CoodernateMap.begin(); it != CoodernateMap.end(); ++it) {
		newCoodernateMap.insert(it.key(), it.value());
	}

	CoodernateMap = newCoodernateMap;  // 替换原来的map
	QStringList CoodernateNames = CoodernateMap.values();

	ui->comboBox_3->addItems(CoodernateNames);
	if (!CoodernateNames.isEmpty()) {
		ui->comboBox_3->setCurrentIndex(0);
	}

	ui->textEdit->setPlainText("500");//初始化间距

	ui->pushButton_1->setEnabled(false);
	ui->pushButton_3->setEnabled(false);
	ui->comboBox_2->setEnabled(false);//设置联动轴复选框不能使用

	ui->horizontalSlider->setMinimum(-50);
	ui->horizontalSlider->setMaximum(50);
	ui->horizontalSlider->setValue(0);  // 设置为中间点

	ui->verticalSlider->setMinimum(-50);
	ui->verticalSlider->setMaximum(50);
	ui->verticalSlider->setValue(0);  // 设置为中间点

	ui->comboBox_5->addItem("Y轴方向");
	ui->comboBox_5->addItem("Z轴方向");

	// 初始化变量
	x_value = 0.0;
	y_value = 0.0;
	z_value = 0.0;

	// 初始化包围盒相关数据
	m_vPosition.clear();
	ABBPosition.clear();
	points.clear();
	pickupMap.clear();
}


void cursePart::setupGraphicsScenes()
{
	// 创建图形场景
	QGraphicsScene *scene = new QGraphicsScene(this);
	QGraphicsScene *scene1 = new QGraphicsScene(this);
	QGraphicsScene *scene2 = new QGraphicsScene(this);
	QGraphicsScene *scene3 = new QGraphicsScene(this);

	// 加载图片
	QPixmap pixmap(":/image/resource/pickup.png");
	QPixmap pixmap1(":/image/resource/coordinate.png");
	QPixmap pixmap2(":/image/resource/maxTheta.png");
	QPixmap pixmap3(":/image/resource/divide.png");


	// 添加图片到场景
	QGraphicsPixmapItem* pixmapItem = scene->addPixmap(pixmap);
	QGraphicsPixmapItem* pixmapItem1 = scene1->addPixmap(pixmap1);
	QGraphicsPixmapItem* pixmapItem2 = scene2->addPixmap(pixmap2);
	QGraphicsPixmapItem* pixmapItem3 = scene3->addPixmap(pixmap3);

	// 设置场景矩形为图片大小
	scene->setSceneRect(pixmap.rect());
	scene1->setSceneRect(pixmap1.rect());
	scene2->setSceneRect(pixmap2.rect());
	scene3->setSceneRect(pixmap3.rect());

	// 设置QGraphicsView
	ui->graphicsView_1->setScene(scene);
	ui->graphicsView_2->setScene(scene1);
	ui->graphicsView_3->setScene(scene2);
	ui->graphicsView_4->setScene(scene3);

	// 图片自适应大小
	ui->graphicsView_1->setAlignment(Qt::AlignTop | Qt::AlignHCenter); // 顶部水平居中
	ui->graphicsView_2->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	ui->graphicsView_3->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	ui->graphicsView_4->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

	// 去除边框
	ui->graphicsView_1->setFrameShape(QFrame::NoFrame);
	ui->graphicsView_2->setFrameShape(QFrame::NoFrame);
	ui->graphicsView_3->setFrameShape(QFrame::NoFrame);
	ui->graphicsView_4->setFrameShape(QFrame::NoFrame);

	// 设置背景颜色
	ui->graphicsView_1->setStyleSheet("background-color: #f0f0f0;");
	ui->graphicsView_2->setStyleSheet("background-color: #f0f0f0;");
	ui->graphicsView_3->setStyleSheet("background-color: #f0f0f0;");
	ui->graphicsView_4->setStyleSheet("background-color: #f0f0f0;");


	// 设置视图属性
	ui->graphicsView_1->setRenderHint(QPainter::SmoothPixmapTransform);
	ui->graphicsView_2->setRenderHint(QPainter::SmoothPixmapTransform);
	ui->graphicsView_3->setRenderHint(QPainter::SmoothPixmapTransform);
	ui->graphicsView_4->setRenderHint(QPainter::SmoothPixmapTransform);
}

void cursePart::setStepsExplanation()
{
	ui->label_16->setWordWrap(true);
	ui->label_16->setText(tr("1、选择喷涂作业机器人（如果需要喷涂机器人联动作业则勾选联动，并选择机器人联动方向）\n"
		"2、选取喷涂曲面，并点击结束选取"));

	ui->label_17->setWordWrap(true);
	ui->label_17->setText(tr("1、选择喷涂曲面主法矢（通过选取坐标系，以及坐标系上的轴来实现）\n"
		"2、选择划分方向"));

	ui->label_18->setWordWrap(true);
	ui->label_18->setText(tr("1、选择在包围盒上选取一定间距的点阵，通过投影的方式在喷涂表面去交点，计算交点处曲面法向量\n"
		"2、通过遍历各点法向量与喷涂主法矢的夹角，计算出法矢最大偏差角\n"
		"3、通过以上信息来计算出划分工作空间的长、宽"));

	ui->label_19->setWordWrap(true);
	ui->label_19->setText(tr("1、划分网格由左下角点位置决定，通过改变左下角点位置来实现改变网格位置\n"
		"2、也可以稍微增加工作空间的长和宽来实现减少喷涂次数"));

}

void cursePart::addItemToListView(const QString& item)
{
	QAbstractItemModel* model = ui->listView->model();
	if (!model) {
		model = new QStringListModel(this);
		ui->listView->setModel(model);
	}

	QStringListModel* listModel = qobject_cast<QStringListModel*>(model);
	if (listModel) {
		QStringList items = listModel->stringList();
		items.append(item);
		listModel->setStringList(items);
	}
}

void cursePart::on_horizontalSlider_valueChanged(int value)
{
	// 计算百分比变化：value范围是-50到50，对应-50%到+50%
	double percentage = value / 100.0; // 转换为小数形式

	// 计算新值：500 + 500 * 百分比
	double newValue = x_value + 500 * percentage;

	// 更新textEdit_3的显示
	ui->textEdit_3->setPlainText(QString::number(newValue, 'f', 2));

	// 实现onAreaPosition()功能
	std::vector<double> areaPosition;
	areaPosition = [this]() {
		std::vector<double> result;
		double x_value = ui->textEdit_3->toPlainText().toDouble();
		double y_value = ui->textEdit_4->toPlainText().toDouble();
		double z_value = ui->textEdit_5->toPlainText().toDouble();

		result.push_back(x_value);
		result.push_back(y_value);
		result.push_back(z_value);
		return result;
	}();
	std::vector<double> difference;
	for (int i = 0; i < areaPosition.size(); i++) {
		double diff = areaPosition[i] - points[i];
		difference.push_back(diff);
	}

	for (int i = 0; i < points.size(); i += 3) {
		points[i] = points[i] + difference[0];
		points[i + 1] = points[i + 1] + difference[1];
		points[i + 2] = points[i + 2] + difference[2];
	}
}

void cursePart::on_verticalSlider_valueChanged(int value)
{
	// 计算百分比变化：value范围是-50到50，对应-50%到+50%
	double percentage = value / 100.0; // 转换为小数形式

	// 计算新值：500 + 500 * 百分比
	double newValue = z_value + 500 * percentage;

	// 更新textEdit_3的显示
	ui->textEdit_5->setPlainText(QString::number(newValue, 'f', 2));

	// 实现onAreaPosition()功能
	std::vector<double> areaPosition;
	areaPosition = [this]() {
		std::vector<double> result;
		double x_value = ui->textEdit_3->toPlainText().toDouble();
		double y_value = ui->textEdit_4->toPlainText().toDouble();
		double z_value = ui->textEdit_5->toPlainText().toDouble();

		result.push_back(x_value);
		result.push_back(y_value);
		result.push_back(z_value);
		return result;
	}();
	std::vector<double> difference;
	for (int i = 0; i < areaPosition.size(); i++) {
		double diff = areaPosition[i] - points[i];
		difference.push_back(diff);
	}

	for (int i = 0; i < points.size(); i += 3) {
		points[i] = points[i] + difference[0];
		points[i + 1] = points[i + 1] + difference[1];
		points[i + 2] = points[i + 2] + difference[2];
	}
}

void cursePart::on_coordanateTextChanged()
{
	// 阻塞 comboBox_5 的信号，防止在清空和添加项时触发不必要的信号（如 currentIndexChanged）
	ui->comboBox_5->blockSignals(true);

	// 清空 comboBox_5
	ui->comboBox_5->clear();

	// 获取当前状态 - 使用comboBox_4的当前索引
	int state = ui->comboBox_4->currentIndex();

	// 根据状态添加不同的项
	if (state < 2) {
		ui->comboBox_5->addItem("Y轴方向");
		ui->comboBox_5->addItem("Z轴方向");
	}
	else if (1 < state && state < 4) {
		ui->comboBox_5->addItem("X轴方向");
		ui->comboBox_5->addItem("Z轴方向");
	}
	else {
		ui->comboBox_5->addItem("X轴方向");
		ui->comboBox_5->addItem("Y轴方向");
	}

	// 重新启用 comboBox_5 的信号
	ui->comboBox_5->blockSignals(false);

	// 可以在这里设置一个默认选中项，例如第一项
	if (ui->comboBox_5->count() > 0) {
		ui->comboBox_5->setCurrentIndex(0);
	}
}

void cursePart::on_confirm_clicked()
{
	// 确认按钮逻辑
	done(QDialog::Rejected);
	this->close();

}

void cursePart::on_calculate_workspace()
{
	QString robotName = ui->comboBox_1->currentText();
	ULONG robotID = 0;

	QString coordinateName = ui->comboBox_3->currentText();
	QString directionName = ui->comboBox_4->currentText();

	QString divisionDirName = ui->comboBox_5->currentText();

	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);
	bool isLink = ui->checkBox->isChecked();
	QString thetaStr = ui->textBrowser_1->toPlainText();
	thetaStr.remove(QRegExp(R"([°º˚°⁰])"));  // 移除常见的角度符号
	thetaStr = thetaStr.simplified();  // 去除多余的空白字符
	double theta = thetaStr.toDouble();

	QString railName = ui->comboBox_2->currentText();

	QString thicknessStr = ui->textBrowser_2->toPlainText();
	double thickness = thicknessStr.toDouble();

	QString jsonName = m_tempDir + "workspace_" + robotName + ".json";

	theta = 8;
	thickness = 300;

	// 创建或更新工作空间处理器
	if (m_workspaceHandler) {
		delete m_workspaceHandler;
	}
	m_workspaceHandler = new RobotWorkspaceHandler(jsonName.toStdString());

	// 使用RobotWorkspaceHandler进行查询
	auto foundPoints = m_workspaceHandler->processRobotWorkspaceQuery(
		robotName, robotID, coordinateName, directionName, isLink, theta, thickness, railName
	);

	if (!foundPoints.empty()) {
		// 使用查询到的points更新UI
		if (foundPoints.size() >= 1) {
			ui->textEdit_3->setPlainText(QString::number(foundPoints[0]));
		}
		if (foundPoints.size() >= 2) {
			ui->textEdit_4->setPlainText(QString::number(foundPoints[1]));
		}
		if (foundPoints.size() >= 3) {
			ui->textEdit_5->setPlainText(QString::number(foundPoints[2]));
		}

		// 更新x_value和z_value
		if (foundPoints.size() >= 1) x_value = foundPoints[0];
		if (foundPoints.size() >= 3) z_value = foundPoints[2];

		// 更新points数组
		points = foundPoints;

		// 从查询到的点中计算工作空间的长和宽
		if (foundPoints.size() == 24) { // 包围盒的8个角点，每个点3个坐标，共24个值
			// 计算包围盒的最小和最大坐标
			double minX = std::numeric_limits<double>::max();
			double maxX = std::numeric_limits<double>::lowest();
			double minY = std::numeric_limits<double>::max();
			double maxY = std::numeric_limits<double>::lowest();
			double minZ = std::numeric_limits<double>::max();
			double maxZ = std::numeric_limits<double>::lowest();

			// 遍历所有8个角点找出最大最小值
			for (int i = 0; i < 24; i += 3) {
				double x = foundPoints[i];
				double y = foundPoints[i + 1];
				double z = foundPoints[i + 2];

				minX = std::min(minX, x);
				maxX = std::max(maxX, x);
				minY = std::min(minY, y);
				maxY = std::max(maxY, y);
				minZ = std::min(minZ, z);
				maxZ = std::max(maxZ, z);
			}

			// 计算三个方向的长度
			double xLength = maxX - minX;
			double yLength = maxY - minY;
			double zLength = maxZ - minZ;

			// 根据directionName确定主要划分方向和次要划分方向
			double length = 0.0;  // 主要划分方向的长度
			double width = 0.0;   // 次要划分方向的宽度

			if (divisionDirName.contains("X", Qt::CaseInsensitive)) {
				// X方向是主要划分方向
				length = xLength;

				// 从Y和Z中选择较小者作为次要划分方向
				if (directionName.contains("Y", Qt::CaseInsensitive)) {
					width = zLength;
				}
				else {
					width = yLength;
				}
			}
			else if (divisionDirName.contains("Y", Qt::CaseInsensitive)) {
				// Y方向是主要划分方向
				length = yLength;

				// 从Y和Z中选择较小者作为次要划分方向
				if (directionName.contains("X", Qt::CaseInsensitive)) {
					width = xLength;
				}
				else {
					width = zLength;
				}
			}
			else {
				// Z方向是主要划分方向
				length = zLength;

				// 从Y和Z中选择较小者作为次要划分方向
				if (directionName.contains("X", Qt::CaseInsensitive)) {
					width = xLength;
				}
				else {
					width = yLength;
				}
			}

			// 将计算出的长和宽设置到textEdit_1和textEdit_2
			ui->textEdit_1->setPlainText(QString::number(length, 'f', 2));
			ui->textEdit_2->setPlainText(QString::number(width, 'f', 2));
		}
		else if (foundPoints.size() >= 6) { // 确保有足够的点来计算包围盒
			double minX = foundPoints[0], maxX = foundPoints[0];
			double minY = foundPoints[1], maxY = foundPoints[1];
			double minZ = foundPoints[2], maxZ = foundPoints[2];

			// 遍历所有点找出最大最小值
			for (size_t i = 0; i < foundPoints.size(); i += 3) {
				if (i + 2 < foundPoints.size()) {
					minX = std::min(minX, foundPoints[i]);
					maxX = std::max(maxX, foundPoints[i]);
					minY = std::min(minY, foundPoints[i + 1]);
					maxY = std::max(maxY, foundPoints[i + 1]);
					minZ = std::min(minZ, foundPoints[i + 2]);
					maxZ = std::max(maxZ, foundPoints[i + 2]);
				}
			}

			// 计算长和宽（除了厚度方向）
			double length, width;

			// 根据directionName判断哪个是厚度方向，其他两个是长宽
			if (directionName.contains("X", Qt::CaseInsensitive) ||
				directionName.contains("x", Qt::CaseInsensitive)) {
				// X方向是厚度方向
				length = maxY - minY; // Y方向长度
				width = maxZ - minZ;  // Z方向宽度
			}
			else if (directionName.contains("Y", Qt::CaseInsensitive) ||
				directionName.contains("y", Qt::CaseInsensitive)) {
				// Y方向是厚度方向
				length = maxX - minX; // X方向长度
				width = maxZ - minZ;  // Z方向宽度
			}
			else {
				// Z方向是厚度方向
				length = maxX - minX; // X方向长度
				width = maxY - minY;  // Y方向宽度
			}

			// 将计算出的长和宽设置到textEdit_1和textEdit_2
			ui->textEdit_1->setPlainText(QString::number(length, 'f', 2));
			ui->textEdit_2->setPlainText(QString::number(width, 'f', 2));
		}
	}
	else {
		// 如果没有找到匹配项，继续执行原始逻辑

		ui->textEdit_1->setPlainText("500");
		ui->textEdit_2->setPlainText("500");
	}

	int length = ui->textEdit_1->toPlainText().toDouble();
	int width = ui->textEdit_2->toPlainText().toDouble();

	Point3D direction(mainDirction[0], mainDirction[1], mainDirction[2]);
	// 使用之前计算的包围盒信息生成点阵
	auto grid = createGridOnClosestSurface(box, length, width, direction);

	points.clear(); // 清空之前的点数据
	for (auto p : grid) {
		points.push_back(p.x);
		points.push_back(p.y);
		points.push_back(p.z);
	}

	if (!points.empty()) {
		QString value1 = QString("%1").arg(points[0]);
		QString value2 = QString("%1").arg(points[1]);
		QString value3 = QString("%1").arg(points[2]);

		ui->textEdit_3->setPlainText(value1);
		ui->textEdit_4->setPlainText(value2);
		ui->textEdit_5->setPlainText(value3);

		// 更新x_value和z_value
		x_value = points[0];
		z_value = points[2];
	}
}

// 页面导航槽函数
void cursePart::on_next_page_clicked()
{
	// 检查是否在第一页且graphicsView_1为空
	if (indx == 0) {
		QAbstractItemModel* model = ui->listView->model();
		if (model == nullptr || model->rowCount() == 0) {
			QMessageBox::warning(this, "警告", "请先选取曲面，不能为空！");
			return; // 不执行翻页操作
		}
	}

	int maxPage = ui->stackedWidget->count() - 1;
	if (indx < maxPage) {
		indx++;
		ui->stackedWidget->setCurrentIndex(indx);
	}

	ui->pushButton_1->setEnabled(indx != 0);
	ui->pushButton_2->setEnabled(indx != maxPage);

	if (indx == maxPage) {
		ui->pushButton_3->setEnabled(true);
		ui->pushButton_4->setEnabled(true);
	}

	// 实现onCloseSignal()功能
	if (isPickupActive || isPreview) {
		isPickupActive = false;
		isPreview = false;
		this->setModal(true);
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		HRESULT hr = m_ptrKit->Doc_end_module(cmd);
		this->setWindowModality(Qt::ApplicationModal);
		qDebug() << "拾取模块已停止";
	}
}

void cursePart::on_prev_page_clicked()
{

	if (indx > 0) {
		indx--;
		ui->stackedWidget->setCurrentIndex(indx);
	}
	int maxPage = ui->stackedWidget->count() - 1;
	ui->pushButton_1->setEnabled(indx != 0);
	ui->pushButton_2->setEnabled(indx != maxPage);
}

void cursePart::on_cancel_clicked()
{
	done(QDialog::Rejected);
	this->close();
}


void cursePart::on_pickUpButton_clicked()
{
	// 实现onPickUpSignal()功能
	if (!isPickupActive && !isPreview) {
		// 启动拾取模块
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		HRESULT hr = m_ptrKit->Doc_start_module(cmd);
		if (SUCCEEDED(hr)) {
			isPickupActive = true;
			isPreview = false; // 确保预览模式关闭
			this->setModal(false);
			this->setWindowModality(Qt::NonModal);
			qDebug() << "曲面拾取模块已启动，请在3D窗口中点击元素";
		}
		else {
			QMessageBox::warning(this, "错误", "启动曲面拾取模块失败！");
		}
	}
	else {
		QString mode = isPreview ? "预览模式" : "曲面拾取模式";
		qDebug() << mode << "已在运行中";
	}
}

void cursePart::on_finishButton_clicked()
{

	// 实现onCloseSignal()功能
	if (isPickupActive || isPreview) {
		isPickupActive = false;
		isPreview = false;
		this->setModal(true);
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		HRESULT hr = m_ptrKit->Doc_end_module(cmd);
		this->setWindowModality(Qt::ApplicationModal);
		qDebug() << "拾取模块已停止";
	}
}

void cursePart::on_previewButton_clicked()
{
	// 实现onPreviewSignal()功能
	// 启动拾取模块
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	HRESULT hr = m_ptrKit->Doc_start_module(cmd);
	if (SUCCEEDED(hr)) {
		isPoint = true;
		isPickupActive = false; // 关闭拾取模式
	}
}

void cursePart::on_spaceSettingButton_clicked()
{
	m_vPosition.clear(); // 清空之前的数据

	ABBPosition = calculateAABBCornersFromPickupMap(pickupMap);//计算出包围盒子

	QString text = ui->textEdit->toPlainText().trimmed();
	bool ok;
	double spacing = text.toDouble(&ok);

	if (ok && spacing > 0) {
		// 获取坐标系和方向
		QString coordanateName = ui->comboBox_3->currentText();
		QString mainVectorText = ui->comboBox_4->currentText();
		QString mainDivisionDirectionText = ui->comboBox_5->currentText();

		// 获取坐标系姿态
		ULONG selectCoorID = 0;
		PQDataType CoodernateType = PQ_COORD;
		selectCoorID = CoodernateMap.key(coordanateName);

		double coordanate[6] = { 0 };
		if (selectCoorID != 0) {
			int nCount = 0;
			double* dPosture = nullptr;
			m_ptrKit->Doc_get_coordinate_posture(selectCoorID, EULERANGLEXYZ, &nCount, &dPosture, 0);
			if (dPosture) {
				for (int i = 0; i < 6; i++) coordanate[i] = dPosture[i];
				m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
			}
		}

		// 获取轴向量
		std::vector<std::vector<double>> axisVector = getCoordinateAxesFromEuler(coordanate);
		std::vector<double> mainVector = getAxisVector(axisVector, mainVectorText);
		std::vector<double> mainDivisionDirection = getAxisVector(axisVector, mainDivisionDirectionText);

		// 计算最大角度
		Point3D viewDirection(mainVector[0], mainVector[1], mainVector[2]);
		std::vector<Point3D> corners = box.getCorners();
		std::vector<Point3D> result = createGridOnClosestSurface(corners, spacing, spacing, viewDirection);

		double maxtheta = 0;
		for (const auto& key : pickupMap) {
			unsigned long k = key.first;
			for (const auto value : key.second) {
				CComBSTR whSurfaceName = value.c_str();
				for (Point3D P : result) {
					double dPosition[3] = { P.x,P.y,P.z };
					double* dIntersetionpoint = nullptr;
					int nArrsize = 1;
					m_ptrKit->Part_get_ray_surface_intersetion(whSurfaceName, dPosition, mainVector.data(),
						&dIntersetionpoint, &nArrsize);
					if (dIntersetionpoint != nullptr && nArrsize >= 6) {
						Eigen::Vector3d v1(mainVector[0], mainVector[1], mainVector[2]);
						Eigen::Vector3d v2(dIntersetionpoint[3], dIntersetionpoint[4], dIntersetionpoint[5]);
						Eigen::Vector3d v1_norm = v1.normalized();
						Eigen::Vector3d v2_norm = v2.normalized();
						double dot = std::max(-1.0, std::min(1.0, v1_norm.dot(v2_norm)));
						maxtheta = std::max(std::acos(dot), maxtheta);
					}
					if (dIntersetionpoint != nullptr) {
						m_ptrKit->PQAPIFree((LONG_PTR*)dIntersetionpoint);
					}
				}
			}
		}
		maxtheta = maxtheta * 180 / M_PI;
		ui->textBrowser_1->setPlainText(QString("%1").arg(maxtheta) + "°");

		// 计算厚度
		if (ABBPosition.size() == 24) {
			std::vector<double> mainVector = getAxisVector(axisVector, mainVectorText);
			if (mainVector.size() == 3) {
				// 标准化方向向量
				double norm = sqrt(mainVector[0] * mainVector[0] + mainVector[1] * mainVector[1] + mainVector[2] * mainVector[2]);
				if (norm > 0) {
					mainVector[0] /= norm;
					mainVector[1] /= norm;
					mainVector[2] /= norm;
				}

				// 计算8个角点在方向上的投影
				std::vector<double> projections;
				for (int i = 0; i < 8; i++) {
					double x = ABBPosition[i * 3];
					double y = ABBPosition[i * 3 + 1];
					double z = ABBPosition[i * 3 + 2];
					double projection = x * mainVector[0] + y * mainVector[1] + z * mainVector[2];
					projections.push_back(projection);
				}

				if (!projections.empty()) {
					double minProj = *std::min_element(projections.begin(), projections.end());
					double maxProj = *std::max_element(projections.begin(), projections.end());
					m_thickness = maxProj - minProj;
					ui->textBrowser_2->setPlainText(QString::number(m_thickness, 'f', 2));
					qDebug() << "厚度计算完成：" << m_thickness << "mm";
				}
			}
		}

		// 赋值三个方向向量变量
		mainDirction = mainVector;              // 主方向向量
		divisionDirection = mainDivisionDirection; // 划分方向向量

		// 通过叉积计算次要划分方向
		if (mainVector.size() >= 3 && mainDivisionDirection.size() >= 3) {
			// 计算叉积：mainVector × mainDivisionDirection
			double cross_x = mainVector[1] * mainDivisionDirection[2] - mainVector[2] * mainDivisionDirection[1];
			double cross_y = mainVector[2] * mainDivisionDirection[0] - mainVector[0] * mainDivisionDirection[2];
			double cross_z = mainVector[0] * mainDivisionDirection[1] - mainVector[1] * mainDivisionDirection[0];

			// 将叉积结果赋值给次要划分方向
			otherDirection = { cross_x, cross_y, cross_z };
		}

		// 输出调试信息
		qDebug() << "主方向向量:" << mainDirction[0] << "," << mainDirction[1] << "," << mainDirction[2];
		qDebug() << "划分方向向量:" << divisionDirection[0] << "," << divisionDirection[1] << "," << divisionDirection[2];
		qDebug() << "次要划分方向向量:" << otherDirection[0] << "," << otherDirection[1] << "," << otherDirection[2];
	}
	else {
		QMessageBox::warning(this, "Warning", "请输入有效的间距值");
	}
}


void cursePart::on_deleteButton_clicked()
{
	QItemSelectionModel* selectionModel = ui->listView->selectionModel();
	if (!selectionModel) return;

	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	if (selectedIndexes.isEmpty()) {
		qDebug() << "No item selected";
		return;
	}

	QAbstractItemModel* model = ui->listView->model();
	if (!model) return;

	QStringListModel* listModel = qobject_cast<QStringListModel*>(model);
	if (!listModel) return;

	QStringList items = listModel->stringList();
	QStringList deletedSurfaceNames;
	QList<int> rows;

	for (const QModelIndex& index : selectedIndexes) {
		int row = index.row();
		if (row >= 0 && row < items.size()) {
			deletedSurfaceNames.append(items[row]);
			rows.append(row);
		}
	}

	// 去重并排序（从大到小）
	rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
	std::sort(rows.begin(), rows.end(), std::greater<int>());

	for (int row : rows) {
		if (row >= 0 && row < items.size()) {
			items.removeAt(row);
		}
	}

	listModel->setStringList(items);
	selectionModel->clearSelection();

	if (!deletedSurfaceNames.isEmpty()) {
		// 实现onDeleteSelectedSurfaces(deletedSurfaceNames)功能
		if (deletedSurfaceNames.isEmpty()) {
			qDebug() << "没有需要删除的曲面";
			return;
		}

		// 从 pickupMap 中删除指定的曲面
		int deletedCount = 0;

		for (const QString& surfaceName : deletedSurfaceNames) {
			// 将 QString 转换为 std::wstring
			std::wstring wSurfaceName = surfaceName.toStdWString();

			// 遍历 pickupMap 查找并删除对应的曲面
			auto it = pickupMap.begin();
			while (it != pickupMap.end()) {
				auto& surfaces = it->second;
				auto surfaceIt = std::find(surfaces.begin(), surfaces.end(), wSurfaceName);

				if (surfaceIt != surfaces.end()) {
					// 找到曲面，从向量中删除
					surfaces.erase(surfaceIt);
					deletedCount++;
					qDebug() << "已从 pickupMap 中删除曲面:" << surfaceName;

					// 如果该键对应的向量为空，可以选择删除整个键值对
					if (surfaces.empty()) {
						it = pickupMap.erase(it);
					}
					else {
						++it;
					}

					// 假设曲面名称在 pickupMap 中唯一，找到后跳出内层循环
					break;
				}
				else {
					++it;
				}
			}
		}

		qDebug() << "总共删除了" << deletedCount << "个曲面";
	}
}

void cursePart::on_comboBox_currentTextChanged(const QString& text)
{
	updateRailOptions(text, m_robotMap);
}

void cursePart::onDialogFinished(int result)
{
	Q_UNUSED(result)
		// 停止任何正在运行的模块
		if (isPickupActive || isPreview) {
			CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
			m_ptrKit->Doc_end_module(cmd);
		}
	isPickupActive = false;
	isPreview = false;
}

QMap<ULONG, QString> cursePart::getObjectsByType(PQDataType objType)
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

QStringList cursePart::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> cursePart::extractLongArrayFromVariant(const VARIANT& variant)
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


QStringList cursePart::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

void cursePart::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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


std::vector<double> cursePart::calculateAABBCornersFromPickupMap(const std::map<unsigned long,
	std::vector<std::wstring>>&pickupMap)
{
	// 将 ABBPosition 替换为 surfacePoints
	std::vector<double> surfacePoints;
	std::vector<double> resultPositions;

	// 遍历pickupMap中的每个部件
	for (const auto& pair : pickupMap) {
		unsigned long key = pair.first;
		const std::vector<std::wstring>& values = pair.second;

		// 获取部件顶点数量
		long lCount = 0;
		m_ptrKit->PQAPIGetWorkPartVertexCount(key, &lCount);

		if (lCount <= 0) {
			continue;
		}

		// 获取部件顶点坐标
		std::vector<double> dSrc(3 * lCount, 0);
		double* dSrcPosition = dSrc.data();

		CComBSTR bsName;
		m_ptrKit->Doc_get_obj_name(key, &bsName);
		std::wstring wstr(bsName, SysStringLen(bsName));
		ULONG uCoordinate = 0;
		GetObjIDByName(PQ_COORD, wstr, uCoordinate);
		m_ptrKit->PQAPIGetWorkPartVertex(key, 0, lCount, dSrcPosition);

		// 检查每个表面上的点
		for (const auto& value : values) {
			for (long i = 0; i < lCount; i++) {
				double dPosition[3] = {
					dSrcPosition[3 * i],
					dSrcPosition[3 * i + 1],
					dSrcPosition[3 * i + 2]
				};

				double dTol = 10;
				LONG bPointOnSurface = 0;

				// 转换表面名称格式
				std::vector<wchar_t> buffer(value.begin(), value.end());
				buffer.push_back(L'\0');
				LPWSTR name = buffer.data();

				// 检查点是否在表面上
				m_ptrKit->Part_cheak_point_on_surface(name, dPosition, dTol, &bPointOnSurface);

				if (bPointOnSurface) {
					surfacePoints.push_back(dPosition[0]);
					surfacePoints.push_back(dPosition[1]);
					surfacePoints.push_back(dPosition[2]);
				}
			}
		}
	}

	// 计算AABB包围盒
	AABB box;

	if (surfacePoints.empty()) {
		return resultPositions; // 返回空结果
	}

	box.minPoint = { surfacePoints[0], surfacePoints[1], surfacePoints[2] };
	box.maxPoint = { surfacePoints[0], surfacePoints[1], surfacePoints[2] };

	for (size_t i = 0; i < surfacePoints.size(); i += 3) {
		if (i + 2 < surfacePoints.size()) {
			box.minPoint.x = std::min(box.minPoint.x, surfacePoints[i]);
			box.minPoint.y = std::min(box.minPoint.y, surfacePoints[i + 1]);
			box.minPoint.z = std::min(box.minPoint.z, surfacePoints[i + 2]);

			box.maxPoint.x = std::max(box.maxPoint.x, surfacePoints[i]);
			box.maxPoint.y = std::max(box.maxPoint.y, surfacePoints[i + 1]);
			box.maxPoint.z = std::max(box.maxPoint.z, surfacePoints[i + 2]);
		}
	}

	// 计算AABB并获取8个角点
	std::vector<Point3D> boxCorners = box.getCorners();

	// 将角点坐标展平为连续数组
	for (const auto& corner : boxCorners) {
		resultPositions.push_back(corner.x);
		resultPositions.push_back(corner.y);
		resultPositions.push_back(corner.z);
	}
	return resultPositions;
}

std::vector<std::vector<double>> cursePart::getCoordinateAxesFromEuler(double * eulerAngles)
{
	double alpha = eulerAngles[3];
	double beta = eulerAngles[4];
	double gamma = eulerAngles[5];

	// 计算三角函数值
	double cosA = cos(alpha), sinA = sin(alpha);
	double cosB = cos(beta), sinB = sin(beta);
	double cosG = cos(gamma), sinG = sin(gamma);

	// 计算旋转矩阵的元素（XYZ顺序）
	// 旋转矩阵 R = Rx * Ry * Rz
	double r11 = cosB * cosG;
	double r12 = cosG * sinA * sinB - cosA * sinG;
	double r13 = cosA * cosG * sinB + sinA * sinG;

	double r21 = cosB * sinG;
	double r22 = cosA * cosG + sinA * sinB * sinG;
	double r23 = cosA * sinB * sinG - cosG * sinA;

	double r31 = -sinB;
	double r32 = cosB * sinA;
	double r33 = cosA * cosB;

	// 创建结果二维数组
	std::vector<std::vector<double>> result(3, std::vector<double>(3));

	// 旋转矩阵的列向量就是坐标轴方向向量
	// 第一列是X轴方向
	result[0][0] = r11; // X轴的X分量
	result[0][1] = r21; // X轴的Y分量
	result[0][2] = r31; // X轴的Z分量

	// 第二列是Y轴方向
	result[1][0] = r12; // Y轴的X分量
	result[1][1] = r22; // Y轴的Y分量
	result[1][2] = r32; // Y轴的Z分量

	// 第三列是Z轴方向
	result[2][0] = r13; // Z轴的X分量
	result[2][1] = r23; // Z轴的Y分量
	result[2][2] = r33; // Z轴的Z分量

	return result;
}


std::vector<double> cursePart::getAxisVector(const std::vector<std::vector<double>>& axis, const QString& name)
{
	if (axis.size() < 3) {
		return {};
	}

	// 检查是否包含"负"字
	bool isNegative = name.contains("负", Qt::CaseInsensitive);

	// 获取原始向量
	std::vector<double> result;

	if (name.contains("X", Qt::CaseInsensitive)) {
		result = axis[0]; // 获取第一行方向向量
	}
	else if (name.contains("Y", Qt::CaseInsensitive)) {
		result = axis[1]; // 获取第二行方向向量
	}
	else if (name.contains("Z", Qt::CaseInsensitive)) {
		result = axis[2]; // 获取第三行方向向量
	}
	else {
		// 如果没有匹配的坐标轴，返回空向量
		return {};
	}

	// 如果结果为空，直接返回
	if (result.empty()) {
		return {};
	}

	// 如果包含"负"字，将向量反向
	if (isNegative) {
		for (auto& component : result) {
			component = -component; // 每个分量取负
		}
	}

	return result;
}

void cursePart::updateRailOptions(const QString & robotName, const QMap<ULONG, QString>& robotMap)
{
	if (!this || robotName.isEmpty()) {
		return;
	}
	// 获取轨道信息
	ULONG uExternalID = 0;
	QString railname = robotName + "_rail";
	GetObjIDByName(PQ_ROBOT, railname.toStdWString(), uExternalID);

	int railCount = 0;
	HRESULT hr = m_ptrKit->Doc_get_obj_joint_count(uExternalID, &railCount);

	QStringList rail;
	if (SUCCEEDED(hr) && railCount > 0) {
		for (int i = 0; i < railCount; i++) {
			QString railName = "J" + QString::number(i + 1);
			rail.append(railName);
		}
	}
	else {
		// 如果轨道数量为0或调用失败，确保rail为空列表
		rail.clear();
	}

	// 更新对话框中的轨道选项
	ui->comboBox_2->clear();
	ui->comboBox_2->addItems(rail);
	if (!rail.isEmpty()) {
		ui->comboBox_2->setCurrentIndex(0);
	}
}


void cursePart::CreateBoundingBox()
{
	double start[6] = { 0.0 };
	double dEnd[6] = { 100,100,100,200,200,200 };
	double dRGB[3] = { 255, 0, 0 };
	ULONG i_uCoordinateID = 0;
	ULONG o_uCylinderID = 0;
	m_ptrKit->Doc_draw_cylinder(start, 6, dEnd, 6, 16,
		dRGB, 3, i_uCoordinateID, &o_uCylinderID, false);
}

void cursePart::OnDraw()
{
	if (isPoint) {
		std::map<int, std::array<double, 3>> pointMap;
		std::array<double, 3> tempPoint;

		for (size_t i = 0; i < points.size(); i++)
		{
			tempPoint[i % 3] = points[i];

			if ((i % 3) == 2 && i >= 2)
			{
				int pointIndex = i / 3; // 顶点索引是连续的
				pointMap.emplace(pointIndex, tempPoint);
			}
		}

		// 假设每个长方形由4个顶点构成，按顺序排列
		int rectanglesCount = pointMap.size() / 4; // 计算能组成的长方形数量

		// 定义单个长方体的边连接关系（本地偏移）
		std::vector<std::pair<int, int>> singleRectangleEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 0} // 单个长方形的四条边
		};

		// 为所有长方体生成边定义
		std::vector<std::pair<int, int>> edgeDefinitions;
		for (int rectIndex = 0; rectIndex < rectanglesCount; ++rectIndex) {
			int vertexOffset = rectIndex * 4; // 每个长方形占用4个顶点
			for (const auto& edge : singleRectangleEdges) {
				// 将本地顶点索引转换为全局顶点索引
				int globalStart = vertexOffset + edge.first;
				int globalEnd = vertexOffset + edge.second;
				edgeDefinitions.push_back({ globalStart, globalEnd });
			}
		}

		// 计算需要的数组大小：边的数量 * 每个点3个坐标分量 * 2（起点和终点）
		int totalEdges = edgeDefinitions.size();
		double* start = new double[totalEdges * 3]; // 起点坐标数组
		double* dEnd = new double[totalEdges * 3];  // 终点坐标数组

		// 填充数组
		int arrayIndex = 0;
		for (const auto& edge : edgeDefinitions) {
			int startIndex = edge.first;
			int endIndex = edge.second;

			if (pointMap.find(startIndex) != pointMap.end() &&
				pointMap.find(endIndex) != pointMap.end()) {
				const std::array<double, 3>& startPoint = pointMap.at(startIndex);
				const std::array<double, 3>& endPoint = pointMap.at(endIndex);

				for (int coord = 0; coord < 3; ++coord) {
					start[arrayIndex] = startPoint[coord];
					dEnd[arrayIndex] = endPoint[coord];
					arrayIndex++;
				}
			}
		}

		double dRGB[3] = { 255, 0, 0 };
		ULONG i_uCoordinateID = 0;
		ULONG o_uCylinderID = 0;

		// 绘制所有边
		for (int i = 0; i < totalEdges * 3; i += 3) {
			double start_new[3] = { start[i], start[i + 1], start[i + 2] };
			double end_new[3] = { dEnd[i], dEnd[i + 1], dEnd[i + 2] };
			m_ptrKit->Doc_draw_cylinder(start_new, 3, end_new, 3, 3,
				dRGB, 3, i_uCoordinateID, &o_uCylinderID, false);
		}

		delete[] start;
		delete[] dEnd;
	}
}

void cursePart::OnElementPickup(ULONG i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
	double i_dPointX, double i_dPointY, double i_dPointZ)
{
	// 然后处理拾取结果（如果处于拾取模式且有活动的对话框）
	if (isPickupActive && this && this->isVisible()) {
		// 可以添加更多条件检查，比如只处理特定类型的元素
		if (i_nEntityType == 2) { // 例如，只处理类型1的元素
			QString entityName = QString::fromWCharArray(i_lEntityID);
			this->addItemToListView(entityName);
			qDebug() << "拾取到元素:" << entityName;
			//记录
			pickupMap[i_ulObjID].push_back(i_lEntityID ? i_lEntityID : L"");
		}
	}
}

// 保存工作空间数据的方法实现
void cursePart::saveWorkspaceData() {
	QString robotName = ui->comboBox_1->currentText();
	ULONG robotID = 0;
	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);

	RobotWorkspaceBoundary boundary;
	boundary.robotID = robotID;
	boundary.thickness = m_thickness;
	boundary.theta = ui->textBrowser_1->toPlainText().toDouble();
	boundary.isLink = ui->checkBox->isChecked();
	boundary.CoordinateName = ui->comboBox_3->currentText();
	boundary.DirectionName = ui->comboBox_4->currentText();

	// 添加轨道名称到railName向量
	if (!ui->comboBox_2->currentText().isEmpty()) {
		boundary.railName.push_back(ui->comboBox_2->currentText());
	}

	// 添加当前points到boundary
	boundary.points = points;

	QString jsonName = m_tempDir + "workspace_" + robotName + ".json";
	if (m_workspaceHandler) {
		delete m_workspaceHandler;
	}
	m_workspaceHandler = new RobotWorkspaceHandler(jsonName.toStdString());
	m_workspaceHandler->writeRobotWorkspaceBoundary(boundary);
}
