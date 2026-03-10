#include "cursePart.h"
#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

static void normalizeVector(std::vector<double>& v) {
	double length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (length > 1e-9) {
		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	}
}

static std::vector<double> crossProduct(const std::vector<double>& a, const std::vector<double>& b) {
	std::vector<double> result(3);
	result[0] = a[1] * b[2] - a[2] * b[1];
	result[1] = a[2] * b[0] - a[0] * b[2];
	result[2] = a[0] * b[1] - a[1] * b[0];
	return result;
}

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
	newBoundary["CoordinateName"] = boundary.CoordinateName.toStdString();
	newBoundary["DirectionName"] = boundary.DirectionName.toStdString();

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
	const QString& coordinateName,
	const QString& directionName,
	double targetTheta,
	double targetThickness) {

	parseJSON queryHelper(jsonFileName);
	std::map<std::string, json> conditions;
	conditions["robotID"] = robotID;
	conditions["CoordinateName"] = coordinateName.toStdString();
	conditions["DirectionName"] = directionName.toStdString();

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
	double theta,
	double thickness) {

	// 首先查找匹配的记录
	auto matchingPoints = findMatchingPoints(
		robotID, coordinateName, directionName, theta, thickness
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

	connect(ui->pushButton, &QPushButton::clicked, this, &cursePart::on_pickupPoint_clicked);//拾取按钮
	connect(ui->textEdit_6, &QTextEdit::textChanged, this, &cursePart::on_finishButton_clicked);//关闭拾取模式

	connect(ui->pushButton_3, &QPushButton::clicked, this, &cursePart::on_previewButton_clicked);//预览
	connect(ui->pushButton_9, &QPushButton::clicked, this, &cursePart::on_spaceSettingButton_clicked);//计算最大偏差角和厚度

	connect(ui->pushButton_10, &QPushButton::clicked, this, &cursePart::on_calculate_workspace);//计算出划分区域长和宽

	// 组合框和文本编辑框
	connect(ui->comboBox_6, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &cursePart::on_coordanateTextChanged);

	connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool checked) {
		// 获取当前选中机器人的 railNameStr
		QString currentRobotName = ui->comboBox_1->currentText();
		std::string robotStdName = currentRobotName.toStdString();
		std::string currentRailNameStr = "无"; // 默认值

		auto it = relationsMap.find(robotStdName);
		if (it != relationsMap.end() && it->second.first != "无" && !it->second.first.empty()) {
			currentRailNameStr = it->second.first;
		}

		// 调用辅助函数更新复选框状态
		updateLinkedJointCheckBoxes(currentRailNameStr);
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

	ui->comboBox_1->addItems(robotNames);
	if (!robotNames.isEmpty()) {
		ui->comboBox_1->setCurrentIndex(0);
	}


	//// 设置坐标系
	//PQDataType CoodernateType = PQ_COORD;
	//CoodernateMap = getObjectsByType(CoodernateType);

	//// 创建一个新的QMap，先插入"世界坐标系"，再插入原有的数据
	//QMap<ULONG, QString> newCoodernateMap;
	//newCoodernateMap.insert(0, "世界坐标系");  // 先插入首位

	//// 将原有数据插入到后面（键值从1开始）
	//for (auto it = CoodernateMap.begin(); it != CoodernateMap.end(); ++it) {
	//	newCoodernateMap.insert(it.key(), it.value());
	//}

	//CoodernateMap = newCoodernateMap;  // 替换原来的map
	////QStringList CoodernateNames = CoodernateMap.values();
	QStringList CoodernateNames;
	CoodernateNames.push_back("机器人基座标系");

	ui->comboBox_3->addItems(CoodernateNames);
	if (!CoodernateNames.isEmpty()) {
		ui->comboBox_3->setCurrentIndex(0);
	}

	//获取机器人导轨
	QString relations = m_tempDir + "relations.json";
	relationsMap = loadRobotRelations(relations.toStdString());

	// 连接机器人选择框的变化信号
	connect(ui->comboBox_1, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
		this, &cursePart::onRobotSelectionChanged);

	onRobotSelectionChanged(ui->comboBox_1->currentText());

	//设置采样点间距默认为500
	ui->textEdit->setPlainText("500");//初始化间距

	//设置确认和预览按钮不可选中
	ui->pushButton_1->setEnabled(false);
	ui->pushButton_3->setEnabled(false);

	ui->horizontalSlider->setMinimum(-50);
	ui->horizontalSlider->setMaximum(50);
	ui->horizontalSlider->setValue(0);  // 设置为中间点

	ui->verticalSlider->setMinimum(-50);
	ui->verticalSlider->setMaximum(50);
	ui->verticalSlider->setValue(0);  // 设置为中间点

	ui->comboBox_7->addItem("Y轴方向");
	ui->comboBox_7->addItem("Z轴方向");

	// 初始化变量
	x_value = 0.0;
	y_value = 0.0;
	z_value = 0.0;

	// 初始化包围盒相关数据
	OBBPosition.clear();
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

	// 更新textEdit_5的显示
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
	ui->comboBox_7->blockSignals(true);

	// 清空 comboBox_5
	ui->comboBox_7->clear();

	// 获取当前状态 - 使用comboBox_6的当前索引
	int state = ui->comboBox_6->currentIndex();

	// 根据状态添加不同的项
	if (state < 2) {
		ui->comboBox_7->addItem("Y轴方向");
		ui->comboBox_7->addItem("Z轴方向");
	}
	else if (1 < state && state < 4) {
		ui->comboBox_7->addItem("X轴方向");
		ui->comboBox_7->addItem("Z轴方向");
	}
	else {
		ui->comboBox_7->addItem("X轴方向");
		ui->comboBox_7->addItem("Y轴方向");
	}

	// 重新启用 comboBox_5 的信号
	ui->comboBox_7->blockSignals(false);

	// 可以在这里设置一个默认选中项，例如第一项
	if (ui->comboBox_7->count() > 0) {
		ui->comboBox_7->setCurrentIndex(0);
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

	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	HRESULT hr = m_ptrKit->Doc_start_module(cmd);
	if (SUCCEEDED(hr)) {
		istest = true;
	}

	QString robotName = ui->comboBox_1->currentText();
	ULONG robotID = 0;

	QString coordinateName = ui->comboBox_3->currentText();
	QString directionName = ui->comboBox_4->currentText();
	QString divisionDirName = ui->comboBox_7->currentText();
	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);

	//创建划分坐标系列表
	// 获取坐标系和方向
	QString coordanateName = ui->comboBox_5->currentText();
	QString mainDivisionDirectionText = ui->comboBox_6->currentText();
	QString otherDivisionDirectionText = ui->comboBox_7->currentText();

	//获取机器人基座标系
	ULONG robotCoordinateID = 0;
	m_ptrKit->Robot_get_base_coordinate(robotID, &robotCoordinateID);

	double robotCoordinate[6] = { 0 };
	if (robotCoordinateID != 0) {
		int nCount = 0;
		double* dPosture = nullptr;
		m_ptrKit->Doc_get_coordinate_posture(robotCoordinateID, EULERANGLEXYZ, &nCount, &dPosture, 0);
		if (dPosture) {
			for (int i = 0; i < 6; i++) robotCoordinate[i] = dPosture[i];
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
		}
	}

	// 创建划分坐标系
	QMap<int, std::vector<double>> divisionCoordinateMap;

	// 添加世界坐标系
	std::vector<double> worldCoor = { 0, 0, 0, 0, 0, 0 };
	divisionCoordinateMap.insert(0, worldCoor);

	// 添加机器人坐标系
	std::vector<double> robotCoor(6);
	std::copy(robotCoordinate, robotCoordinate + 6, robotCoor.begin());
	divisionCoordinateMap.insert(1, robotCoor);

	// 计算包围盒坐标系
	std::vector<double> OBBCoor = minBox.calculateCoordinateSystemFromCorners(OBBPosition);
	if (OBBCoor.size() != 6) {
		OBBCoor.resize(6, 0.0);
	}
	divisionCoordinateMap.insert(2, OBBCoor);

	// 获取划分坐标系
	int divisionCoorIndx = ui->comboBox_5->currentIndex();
	auto divisionCoorIt = divisionCoordinateMap.find(divisionCoorIndx);

	std::vector<std::vector<double>> divisionCoorVector;
	std::vector<double> mainDivisionDirection;
	std::vector<double> otherDivisionDirection;
	if (divisionCoorIt != divisionCoordinateMap.end()) {
		std::vector<double>& divisionCoor = divisionCoorIt.value();

		// 确保有6个元素
		if (divisionCoor.size() >= 6) {
			// 传递指针给函数
			divisionCoorVector = getCoordinateAxesFromEuler(divisionCoor.data());

			mainDivisionDirection = getAxisVector(divisionCoorVector, mainDivisionDirectionText);

			otherDivisionDirection = getAxisVector(divisionCoorVector, otherDivisionDirectionText);
		}
		else {
			// 处理数据不足
			qWarning() << "坐标数据不足6个元素";
		}
	}
	QString thetaStr = ui->textBrowser_1->toPlainText();
	thetaStr.remove(QRegExp(R"([°º˚°⁰])"));  // 移除常见的角度符号
	thetaStr = thetaStr.simplified();  // 去除多余的空白字符
	double theta = thetaStr.toDouble();

	QString thicknessStr = ui->textBrowser_4->toPlainText();
	double thickness = thicknessStr.toDouble();

	QString jsonName = m_tempDir + "workspace_" + robotName + ".json";

	// 创建或更新工作空间处理器
	if (m_workspaceHandler) {
		delete m_workspaceHandler;
	}
	m_workspaceHandler = new RobotWorkspaceHandler(jsonName.toStdString());

	// 使用RobotWorkspaceHandler进行查询
	auto foundPoints = m_workspaceHandler->processRobotWorkspaceQuery(
		robotName, robotID, coordinateName, directionName, theta, thickness
	);


	//查询划分区域的长和宽
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

	std::vector<Point3D> corners;
	for (int i = 0; i < OBBPosition.size() / 3; i++) {
		Point3D ans(OBBPosition[3 * i], OBBPosition[3 * i + 1], OBBPosition[3 * i + 2]);
		corners.push_back(ans);
	}
	minBox = minBox.calculateOBB(corners);
	auto grid = minBox.createGridOnClosestSurface(length, width, direction, true);

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

	if (indx == 2) {
		QString text1 = ui->textEdit_1->toPlainText().trimmed();
		QString text2 = ui->textEdit_2->toPlainText().trimmed();

		if (text1.isEmpty() || text2.isEmpty()) {
			QMessageBox::warning(this, "警告", "请先计算出划分区域长度和宽度，不能为空");
			return; // 阻止翻页
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
	if (isPickupActive || isPreview || isPoint) {
		isPickupActive = false;
		isPreview = false;
		isPoint = false;
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
		isPreview = true;
		isPickupActive = false; // 关闭拾取模式
	}
}

void cursePart::on_spaceSettingButton_clicked()
{
	//获取间距
	QString text = ui->textEdit->toPlainText().trimmed();
	bool ok;
	double spacing = text.toDouble(&ok);
	if (ok && spacing > 0) {
		//机器人工作空间和方向
		QString mainVectorText = ui->comboBox_4->currentText();

		//划分包围盒子
		QString coordanateName = ui->comboBox_5->currentText();
		QString mainDivisionDirectionText = ui->comboBox_6->currentText();
		QString otherDivisionDirectionText = ui->comboBox_7->currentText();

		//获取机器人基座标系
		QString robotName = ui->comboBox_1->currentText();
		ULONG robotID = 0;
		GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);
		ULONG robotCoordinateID = 0;
		m_ptrKit->Robot_get_base_coordinate(robotID, &robotCoordinateID);

		double robotCoordinate[6] = { 0 };
		if (robotCoordinateID != 0) {
			int nCount = 0;
			double* dPosture = nullptr;
			m_ptrKit->Doc_get_coordinate_posture(robotCoordinateID, EULERANGLEXYZ, &nCount, &dPosture, 0);
			if (dPosture) {
				for (int i = 0; i < 6; i++) robotCoordinate[i] = dPosture[i];
				m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
			}
		}

		//计算喷涂主法矢
		std::vector<std::vector<double>> axisVector = getCoordinateAxesFromEuler(robotCoordinate);
		std::vector<double> mainVector = getAxisVector(axisVector, mainVectorText);

		// 先计算次划分方向，再用于OBB计算
		std::vector<double> preOtherDivisionDirection;
		QMap<int, std::vector<double>> preDivisionCoordinateMap;

		//计算出最小包围盒坐标系
		int preDivisionCoorIndx = ui->comboBox_5->currentIndex();
		if (preDivisionCoorIndx == 0) {
			std::vector<double> tempOBBPosition = calculateOBBCornersFromPickupMap(pickupMap);
			if (!tempOBBPosition.empty()) {
				std::vector<Point3D> tempOBBPositions;
				for (int i = 0; i < tempOBBPosition.size() / 3; i++) {
					Point3D p(tempOBBPosition[3 * i], tempOBBPosition[3 * i + 1], tempOBBPosition[3 * i + 2]);
					tempOBBPositions.push_back(p);
				}
				OBB tempMinBox = OBB::calculateOBB(tempOBBPositions);
				std::vector<double> tempOBBCoor = tempMinBox.calculateCoordinateSystemFromCorners(tempOBBPosition);
				if (tempOBBCoor.size() != 6) {
					tempOBBCoor.resize(6, 0.0);
				}
				preDivisionCoordinateMap.insert(0, tempOBBCoor);
			}
		}
		//计算出世界坐标系
		preDivisionCoordinateMap.insert(1, { 0, 0, 0, 0, 0, 0 });
		//计算出机器人基坐标系
		std::vector<double> preRobotCoor(6);
		std::copy(robotCoordinate, robotCoordinate + 6, preRobotCoor.begin());
		preDivisionCoordinateMap.insert(2, preRobotCoor);

		//计算出划分坐标系
		auto preDivisionCoorIt = preDivisionCoordinateMap.find(preDivisionCoorIndx);
		if (preDivisionCoorIt != preDivisionCoordinateMap.end()) {
			std::vector<double>& preDivisionCoor = preDivisionCoorIt.value();
			if (preDivisionCoor.size() >= 6) {
				std::vector<std::vector<double>> preDivisionCoorVector = getCoordinateAxesFromEuler(preDivisionCoor.data());
				preOtherDivisionDirection = getAxisVector(preDivisionCoorVector, otherDivisionDirectionText);
			}
		}

		if (preOtherDivisionDirection.size() < 3) {
			preOtherDivisionDirection = mainVector;
		}


		OBBPosition = calculateOBBCornersFromPickupMap(pickupMap);//计算出包围盒子

		std::vector<Point3D> OBBPositions;
		for (int i = 0; i < OBBPosition.size() / 3; i++) {
			Point3D p(OBBPosition[3 * i], OBBPosition[3 * i + 1], OBBPosition[3 * i + 2]);
			OBBPositions.push_back(p);
		}

		//计算得到最小包围盒
		minBox = OBB::calculateOBB(OBBPositions);

		//计算得到用于计算喷涂主法矢的矢量
		QString originX = ui->textEdit_6->toPlainText().trimmed();
		QString originY = ui->textEdit_7->toPlainText().trimmed();
		QString originZ = ui->textEdit_8->toPlainText().trimmed();

		//获取原点
		std::vector<double> origin;
		origin.push_back(originX.toDouble());
		origin.push_back(originY.toDouble());
		origin.push_back(originZ.toDouble());

		// 创建划分坐标系
		QMap<int, std::vector<double>> divisionCoordinateMap;

		// 添加世界坐标系
		std::vector<double> worldCoor = { 0, 0, 0, 0, 0, 0 };
		divisionCoordinateMap.insert(0, worldCoor);

		// 添加机器人坐标系
		std::vector<double> robotCoor(6);
		std::copy(robotCoordinate, robotCoordinate + 6, robotCoor.begin());
		divisionCoordinateMap.insert(1, robotCoor);

		// 计算包围盒坐标系
		std::vector<double> OBBCoor = minBox.calculateCoordinateSystemFromCorners(OBBPosition);
		if (OBBCoor.size() != 6) {
			OBBCoor.resize(6, 0.0);
		}
		divisionCoordinateMap.insert(2, OBBCoor);

		// 获取划分坐标系
		int divisionCoorIndx = ui->comboBox_5->currentIndex();
		auto divisionCoorIt = divisionCoordinateMap.find(divisionCoorIndx);

		std::vector<std::vector<double>> divisionCoorVector;
		std::vector<double> divideCoor;
		std::vector<double> mainDivisionDirection;
		std::vector<double> otherDivisionDirection;
		if (divisionCoorIt != divisionCoordinateMap.end()) {
			std::vector<double>& divisionCoor = divisionCoorIt.value();
			divideCoor = divisionCoor;
			// 确保有6个元素
			if (divisionCoor.size() >= 6) {
				// 传递指针给函数
				divisionCoorVector = getCoordinateAxesFromEuler(divisionCoor.data());

				mainDivisionDirection = getAxisVector(divisionCoorVector, mainDivisionDirectionText);

				otherDivisionDirection = getAxisVector(divisionCoorVector, otherDivisionDirectionText);
			}
			else {
				// 处理数据不足
				qWarning() << "坐标数据不足6个元素";
			}
		}

		//创建划分坐标系
		//std::vector<double> divideCoor = convertToLocalEulerAngles(origin, mainDivisionDirection, otherDivisionDirection);

		AABBPosition = transformMultiplePointsToLocal(OBBPosition, divideCoor);
		std::vector<Point3D> AABBPositions;
		for (int i = 0; i < AABBPosition.size() / 3; i++) {
			Point3D ans(AABBPosition[3 * i], AABBPosition[3 * i + 1], AABBPosition[3 * i + 2]);
		}
		//划分包围盒
		divideBox = AABB::calculateAABB(AABBPositions);

		//计算得到主法矢（通过排除坐标系两个方向计算得到主方向）
		std::vector<double> spayVector = getThirdAxisVectorFromTwoAxes(divisionCoorVector,
			mainDivisionDirectionText, otherDivisionDirectionText);

		// 计算最大角度
		Point3D viewDirection(spayVector[0], spayVector[1], spayVector[2]);

		std::vector<Point3D> result = divideBox.createGridOnClosestSurface(spacing, spacing, viewDirection, true);

		double maxtheta = 0;
		for (const auto& key : pickupMap) {
			unsigned long k = key.first;
			for (const auto value : key.second) {
				CComBSTR whSurfaceName = value.c_str();
				for (Point3D P : result) {
					double dPosition[3] = { P.x,P.y,P.z };
					double* dIntersetionpoint = nullptr;
					int nArrsize = 1;
					m_ptrKit->Part_get_ray_surface_intersetion(whSurfaceName, dPosition, spayVector.data(),
						&dIntersetionpoint, &nArrsize);
					if (dIntersetionpoint != nullptr && nArrsize >= 6) {
						Eigen::Vector3d v1(spayVector[0], spayVector[1], spayVector[2]);
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

		// 计算最小包围盒的长度、宽度和厚度
		double length = 0;
		double width = 0;
		bool m_result = calculateOBBDimensionsFromCorners(OBBPosition, length, width, m_thickness);

		double d_length = 0;
		double d_width = 0;
		bool ans = calculateAABBDimensionsFromCorners(AABBPosition, mainDivisionDirectionText, d_length, d_width, d_thickness);
		// 显示长度、宽度和厚度
		ui->textBrowser_2->setPlainText(QString::number(length, 'f', 2)); // 长度
		ui->textBrowser_3->setPlainText(QString::number(width, 'f', 2));  // 宽度
		ui->textBrowser_4->setPlainText(QString::number(m_thickness, 'f', 2)); // 厚度

		ui->textBrowser_5->setPlainText(QString::number(d_length, 'f', 2)); // 长度
		ui->textBrowser_6->setPlainText(QString::number(d_width, 'f', 2));  // 宽度
		ui->textBrowser_7->setPlainText(QString::number(d_thickness, 'f', 2)); // 厚度

		qDebug() << "长度计算完成：" << length << "mm";
		qDebug() << "宽度计算完成：" << width << "mm";
		qDebug() << "厚度计算完成：" << m_thickness << "mm";

		// 赋值三个方向向量变量
		mainDirction = mainVector;					// 主方向向量
		divisionDirection = mainDivisionDirection;  // 主划分方向向量
		otherDirection = otherDivisionDirection;	// 次划分方向向量

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

void cursePart::on_pickupPoint_clicked()
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	HRESULT hr = m_ptrKit->Doc_start_module(cmd);
	if (SUCCEEDED(hr)) {
		isPoint = true;
	}

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

void cursePart::onRobotSelectionChanged(const QString & currentRobotName)
{
	if (currentRobotName.isEmpty()) {
		// 如果没有选中任何机器人，隐藏所有联动相关的复选框
		ui->checkBox->setEnabled(false); // 假设这是控制联动的主复选框
		ui->checkBox_3->setEnabled(false);
		ui->checkBox_4->setEnabled(false);
		ui->checkBox_5->setEnabled(false);
		return;
	}

	// 将QString转换为std::string以便查找
	std::string robotStdName = currentRobotName.toStdString();

	// 在relationsMap中查找当前机器人
	auto it = relationsMap.find(robotStdName);
	bool hasBinding = false;
	std::string railNameStr; // 临时存储rail名称的std::string
	if (it != relationsMap.end()) {
		// 检查rail是否不是"无" (注意：这里修正了逻辑错误)
		if (it->second.first != "无" && !it->second.first.empty()) {
			hasBinding = true;
			railNameStr = it->second.first; // 先存为std::string
		}
	}

	updateLinkedJointCheckBoxes(railNameStr);
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

std::map<std::string, std::pair<std::string, std::string>> cursePart::loadRobotRelations(const std::string & filePath)
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

void cursePart::updateLinkedJointCheckBoxes(const std::string & railNameStr)
{
	if (railNameStr.empty() || railNameStr == "无") {
		// 如果 railName 为空或为 "无"，则禁用所有联动复选框
		ui->checkBox_3->setEnabled(false);
		ui->checkBox_4->setEnabled(false);
		ui->checkBox_5->setEnabled(false);
		return;
	}

	ULONG railID = 0;
	// 获取railID，需要将std::string转换为std::wstring
	GetObjIDByName(PQ_ROBOT, std::wstring(railNameStr.begin(), railNameStr.end()), railID);

	if (railID != 0) { // 确保找到了railID
		int railCount = 0;
		double* railJoints = nullptr;
		HRESULT hr = m_ptrKit->Doc_get_obj_joints(railID, &railCount, &railJoints);

		if (ui->checkBox->isChecked()) { // 检查主联动复选框是否被勾选
			// 根据 railCount 设置复选框的启用状态
			ui->textEdit_9->setEnabled(true);
			switch (railCount) {
			case 1:
				ui->checkBox_3->setEnabled(true);
				ui->checkBox_4->setEnabled(false);
				ui->checkBox_5->setEnabled(false);
				break;
			case 2:
				ui->checkBox_3->setEnabled(true);
				ui->checkBox_4->setEnabled(true);
				ui->checkBox_5->setEnabled(false);
				break;
			case 3:
				ui->checkBox_3->setEnabled(true);
				ui->checkBox_4->setEnabled(true);
				ui->checkBox_5->setEnabled(true);
				break;
			default:
				// 如果 railCount 不是 1, 2, 3 或者小于 0，禁用所有子复选框
				ui->checkBox_3->setEnabled(false);
				ui->checkBox_4->setEnabled(false);
				ui->checkBox_5->setEnabled(false);
				break;
			}
		}
		else {
			// 如果主联动复选框未勾选，则禁用所有联动复选框
			ui->checkBox_3->setEnabled(false);
			ui->checkBox_4->setEnabled(false);
			ui->checkBox_5->setEnabled(false);
			ui->textEdit_9->setEnabled(false);
			ui->textEdit_9->clear();
		}

		// 释放分配的内存
		if (railJoints) {
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)railJoints);
		}
	}
	else {
		// 如果没有找到railID，禁用所有联动子复选框
		ui->checkBox_3->setEnabled(false);
		ui->checkBox_4->setEnabled(false);
		ui->checkBox_5->setEnabled(false);
		qWarning() << "未找到名为" << QString::fromStdString(railNameStr) << "的导轨对象。";
	}
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

std::vector<double> cursePart::convertToLocalEulerAngles(const std::vector<double>& point, const std::vector<double>& mainDirection, const std::vector<double>& otherDirection)
{
	// 验证输入向量维度
	if (point.size() != 3 || mainDirection.size() != 3 || otherDirection.size() != 3) {
		throw std::invalid_argument("所有输入向量必须是三维向量");
	}

	// 创建局部坐标系
	std::vector<double> x_axis(mainDirection);
	normalizeVector(x_axis);

	// 计算Y轴方向（通过主方向和另一个方向的叉积）
	std::vector<double> z_temp(otherDirection);
	normalizeVector(z_temp);

	std::vector<double> y_axis = crossProduct(x_axis, z_temp);
	normalizeVector(y_axis);

	// 重新计算Z轴以确保正交性
	std::vector<double> z_axis = crossProduct(x_axis, y_axis);
	normalizeVector(z_axis);

	// 构建旋转矩阵
	double R[3][3];
	R[0][0] = x_axis[0]; R[0][1] = y_axis[0]; R[0][2] = z_axis[0];
	R[1][0] = x_axis[1]; R[1][1] = y_axis[1]; R[1][2] = z_axis[1];
	R[2][0] = x_axis[2]; R[2][1] = y_axis[2]; R[2][2] = z_axis[2];

	// 将旋转矩阵转换为欧拉角 (XYZ顺序)
	std::vector<double> euler_angles(6, 0.0);

	// 位置信息 (前三个元素)
	euler_angles[0] = point[0];  // X坐标
	euler_angles[1] = point[1];  // Y坐标
	euler_angles[2] = point[2];  // Z坐标

	// 欧拉角 (后三个元素)
	double sy = sqrt(R[0][0] * R[0][0] + R[1][0] * R[1][0]);

	bool singular = sy < 1e-8;

	if (!singular) {
		euler_angles[3] = atan2(R[2][1], R[2][2]); // X rotation (roll)
		euler_angles[4] = atan2(-R[2][0], sy);     // Y rotation (pitch)
		euler_angles[5] = atan2(R[1][0], R[0][0]); // Z rotation (yaw)
	}
	else {
		euler_angles[3] = atan2(-R[1][2], R[1][1]); // X rotation
		euler_angles[4] = atan2(-R[2][0], sy);      // Y rotation
		euler_angles[5] = 0;                        // Z rotation
	}

	return euler_angles;
}

std::vector<double> cursePart::transformPointToLocal(const std::vector<double>& worldPoint, const std::vector<double>& localCoordinateSystem)
{
	if (worldPoint.size() != 3 || localCoordinateSystem.size() != 6) {
		throw std::invalid_argument("worldPoint必须是3维，localCoordinateSystem必须是6维 [origin_x, origin_y, origin_z, euler_x, euler_y, euler_z]");
	}

	// 提取局部坐标系参数
	std::vector<double> localOrigin = {
		localCoordinateSystem[0],
		localCoordinateSystem[1],
		localCoordinateSystem[2]
	};

	double alpha = localCoordinateSystem[3];  // 绕X轴旋转角度
	double beta = localCoordinateSystem[4];   // 绕Y轴旋转角度
	double gamma = localCoordinateSystem[5];  // 绕Z轴旋转角度

	// 构建旋转矩阵 (XYZ旋转顺序)
	double ca = cos(alpha), sa = sin(alpha);
	double cb = cos(beta), sb = sin(beta);
	double cg = cos(gamma), sg = sin(gamma);

	// 旋转矩阵 R = Rz * Ry * Rx
	double r11 = cb * cg;
	double r12 = -cb * sg;
	double r13 = sb;
	double r21 = cg * sa * sb + ca * sg;
	double r22 = ca * cg - sa * sb * sg;
	double r23 = -cb * sa;
	double r31 = -ca * cg * sb + sa * sg;
	double r32 = cg * sa + ca * sb * sg;
	double r33 = ca * cb;

	// 构建旋转矩阵的列向量（局部坐标系的基向量）
	std::vector<double> xAxis = { r11, r21, r31 };  // 局部X轴在世界坐标系中的方向
	std::vector<double> yAxis = { r12, r22, r32 };  // 局部Y轴在世界坐标系中的方向
	std::vector<double> zAxis = { r13, r23, r33 };  // 局部Z轴在世界坐标系中的方向

	// 计算相对于局部原点的偏移
	std::vector<double> offset(3);
	offset[0] = worldPoint[0] - localOrigin[0];
	offset[1] = worldPoint[1] - localOrigin[1];
	offset[2] = worldPoint[2] - localOrigin[2];

	// 转换到局部坐标系（通过与旋转矩阵的转置相乘，即逆旋转）
	std::vector<double> localPoint(3);
	localPoint[0] = offset[0] * r11 + offset[1] * r21 + offset[2] * r31;  // 局部X坐标
	localPoint[1] = offset[0] * r12 + offset[1] * r22 + offset[2] * r32;  // 局部Y坐标
	localPoint[2] = offset[0] * r13 + offset[1] * r23 + offset[2] * r33;  // 局部Z坐标

	return localPoint;
}

std::vector<double> cursePart::transformPointToWorld(const std::vector<double>& localPoint, const std::vector<double>& localCoordinateSystem)
{
	if (localPoint.size() != 3 || localCoordinateSystem.size() != 6) {
		throw std::invalid_argument("localPoint必须是3维，localCoordinateSystem必须是6维 [origin_x, origin_y, origin_z, euler_x, euler_y, euler_z]");
	}

	// 提取局部坐标系参数
	std::vector<double> localOrigin = {
		localCoordinateSystem[0],
		localCoordinateSystem[1],
		localCoordinateSystem[2]
	};

	double alpha = localCoordinateSystem[3];  // 绕X轴旋转角度
	double beta = localCoordinateSystem[4];   // 绕Y轴旋转角度
	double gamma = localCoordinateSystem[5];  // 绕Z轴旋转角度

	// 构建旋转矩阵 (XYZ旋转顺序)
	double ca = cos(alpha), sa = sin(alpha);
	double cb = cos(beta), sb = sin(beta);
	double cg = cos(gamma), sg = sin(gamma);

	// 旋转矩阵 R = Rz * Ry * Rx
	double r11 = cb * cg;
	double r12 = -cb * sg;
	double r13 = sb;
	double r21 = cg * sa * sb + ca * sg;
	double r22 = ca * cg - sa * sb * sg;
	double r23 = -cb * sa;
	double r31 = -ca * cg * sb + sa * sg;
	double r32 = cg * sa + ca * sb * sg;
	double r33 = ca * cb;

	// 应用旋转并加偏移
	std::vector<double> worldPoint(3);
	worldPoint[0] = localPoint[0] * r11 + localPoint[1] * r12 + localPoint[2] * r13 + localOrigin[0];
	worldPoint[1] = localPoint[0] * r21 + localPoint[1] * r22 + localPoint[2] * r23 + localOrigin[1];
	worldPoint[2] = localPoint[0] * r31 + localPoint[1] * r32 + localPoint[2] * r33 + localOrigin[2];

	return worldPoint;
}

std::vector<double> cursePart::transformMultiplePointsToLocal(const std::vector<double>& worldPoints, const std::vector<double>& localCoordinateSystem)
{
	if (localCoordinateSystem.size() != 6) {
		throw std::invalid_argument("localCoordinateSystem必须是6维 [origin_x, origin_y, origin_z, euler_x, euler_y, euler_z]");
	}

	if (worldPoints.size() % 3 != 0) {
		throw std::invalid_argument("worldPoints的大小必须是3的倍数（每个点需要3个坐标）");
	}

	// 提取局部坐标系参数
	std::vector<double> localOrigin = {
		localCoordinateSystem[0],
		localCoordinateSystem[1],
		localCoordinateSystem[2]
	};

	double alpha = localCoordinateSystem[3];  // 绕X轴旋转角度
	double beta = localCoordinateSystem[4];   // 绕Y轴旋转角度
	double gamma = localCoordinateSystem[5];  // 绕Z轴旋转角度

	// 构建旋转矩阵 (XYZ旋转顺序)
	double ca = cos(alpha), sa = sin(alpha);
	double cb = cos(beta), sb = sin(beta);
	double cg = cos(gamma), sg = sin(gamma);

	// 旋转矩阵 R = Rz * Ry * Rx
	double r11 = cb * cg;
	double r12 = -cb * sg;
	double r13 = sb;
	double r21 = cg * sa * sb + ca * sg;
	double r22 = ca * cg - sa * sb * sg;
	double r23 = -cb * sa;
	double r31 = -ca * cg * sb + sa * sg;
	double r32 = cg * sa + ca * sb * sg;
	double r33 = ca * cb;

	// 预分配结果向量
	std::vector<double> localPoints;
	localPoints.reserve(worldPoints.size());

	// 对每个点进行变换
	for (size_t i = 0; i < worldPoints.size(); i += 3) {
		if (i + 2 >= worldPoints.size()) break; // 确保有完整的三点

		// 计算相对于局部原点的偏移
		double offsetX = worldPoints[i] - localOrigin[0];
		double offsetY = worldPoints[i + 1] - localOrigin[1];
		double offsetZ = worldPoints[i + 2] - localOrigin[2];

		// 转换到局部坐标系（通过与旋转矩阵的转置相乘，即逆旋转）
		double localX = offsetX * r11 + offsetY * r21 + offsetZ * r31;  // 局部X坐标
		double localY = offsetX * r12 + offsetY * r22 + offsetZ * r32;  // 局部Y坐标
		double localZ = offsetX * r13 + offsetY * r23 + offsetZ * r33;  // 局部Z坐标

		localPoints.push_back(localX);
		localPoints.push_back(localY);
		localPoints.push_back(localZ);
	}

	return localPoints;
}

std::vector<double> cursePart::transformMultiplePointsToWorld(const std::vector<double>& localPoints, const std::vector<double>& localCoordinateSystem)
{
	if (localCoordinateSystem.size() != 6) {
		throw std::invalid_argument("localCoordinateSystem必须是6维 [origin_x, origin_y, origin_z, euler_x, euler_y, euler_z]");
	}

	if (localPoints.size() % 3 != 0) {
		throw std::invalid_argument("localPoints的大小必须是3的倍数（每个点需要3个坐标）");
	}

	// 提取局部坐标系参数
	std::vector<double> localOrigin = {
		localCoordinateSystem[0],
		localCoordinateSystem[1],
		localCoordinateSystem[2]
	};

	double alpha = localCoordinateSystem[3];  // 绕X轴旋转角度
	double beta = localCoordinateSystem[4];   // 绕Y轴旋转角度
	double gamma = localCoordinateSystem[5];  // 绕Z轴旋转角度

	// 构建旋转矩阵 (XYZ旋转顺序)
	double ca = cos(alpha), sa = sin(alpha);
	double cb = cos(beta), sb = sin(beta);
	double cg = cos(gamma), sg = sin(gamma);

	// 旋转矩阵 R = Rz * Ry * Rx
	double r11 = cb * cg;
	double r12 = -cb * sg;
	double r13 = sb;
	double r21 = cg * sa * sb + ca * sg;
	double r22 = ca * cg - sa * sb * sg;
	double r23 = -cb * sa;
	double r31 = -ca * cg * sb + sa * sg;
	double r32 = cg * sa + ca * sb * sg;
	double r33 = ca * cb;

	// 预分配结果向量
	std::vector<double> worldPoints;
	worldPoints.reserve(localPoints.size());

	// 对每个点进行变换
	for (size_t i = 0; i < localPoints.size(); i += 3) {
		if (i + 2 >= localPoints.size()) break; // 确保有完整的三点

		// 应用旋转
		double rotatedX = localPoints[i] * r11 + localPoints[i + 1] * r12 + localPoints[i + 2] * r13;
		double rotatedY = localPoints[i] * r21 + localPoints[i + 1] * r22 + localPoints[i + 2] * r23;
		double rotatedZ = localPoints[i] * r31 + localPoints[i + 1] * r32 + localPoints[i + 2] * r33;

		// 加上偏移得到世界坐标
		worldPoints.push_back(rotatedX + localOrigin[0]);
		worldPoints.push_back(rotatedY + localOrigin[1]);
		worldPoints.push_back(rotatedZ + localOrigin[2]);
	}

	return worldPoints;
}

std::vector<double> cursePart::getThirdAxisVectorFromTwoAxes(const std::vector<std::vector<double>>& axisVector, const QString & firstAxisName, const QString & secondAxisName)
{
	if (axisVector.size() < 3) {
		qDebug() << "错误: axisVector 必须包含至少3个轴向量";
		return {};
	}

	// 获取第一个轴向量
	std::vector<double> firstAxis = getAxisVector(axisVector, firstAxisName);
	if (firstAxis.empty()) {
		qDebug() << "错误: 无法获取第一个轴向量: " << firstAxisName;
		return {};
	}

	// 获取第二个轴向量
	std::vector<double> secondAxis = getAxisVector(axisVector, secondAxisName);
	if (secondAxis.empty()) {
		qDebug() << "错误: 无法获取第二个轴向量: " << secondAxisName;
		return {};
	}

	// 计算叉积得到第三个轴向量 (firstAxis × secondAxis)
	std::vector<double> thirdAxis(3);
	thirdAxis[0] = firstAxis[1] * secondAxis[2] - firstAxis[2] * secondAxis[1];  // i分量
	thirdAxis[1] = firstAxis[2] * secondAxis[0] - firstAxis[0] * secondAxis[2];  // j分量
	thirdAxis[2] = firstAxis[0] * secondAxis[1] - firstAxis[1] * secondAxis[0];  // k分量

	// 归一化向量
	double length = sqrt(thirdAxis[0] * thirdAxis[0] + thirdAxis[1] * thirdAxis[1] + thirdAxis[2] * thirdAxis[2]);
	if (length > 1e-9) {
		thirdAxis[0] /= length;
		thirdAxis[1] /= length;
		thirdAxis[2] /= length;
	}

	return thirdAxis;
}

bool cursePart::calculateAABBDimensionsFromCorners(const std::vector<double>& AABBPosition, const QString & mainVectorText, double & length, double & width, double & thickness)
{
	if (AABBPosition.size() != 24) {
		return false; // 需要恰好24个值（8个角点，每个点3个坐标）
	}

	// 计算包围盒的最小和最大坐标
	double minX = std::numeric_limits<double>::max();
	double maxX = std::numeric_limits<double>::lowest();
	double minY = std::numeric_limits<double>::max();
	double maxY = std::numeric_limits<double>::lowest();
	double minZ = std::numeric_limits<double>::max();
	double maxZ = std::numeric_limits<double>::lowest();

	// 遍历所有8个角点找出最大最小值
	for (int i = 0; i < 24; i += 3) {
		double x = AABBPosition[i];
		double y = AABBPosition[i + 1];
		double z = AABBPosition[i + 2];

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

	// 根据mainVectorText确定厚度方向和其他两个方向
	length = 0.0;  // 长度
	width = 0.0;   // 宽度
	thickness = 0.0;    // 厚度

	if (mainVectorText.contains("X", Qt::CaseInsensitive)) {
		// X方向是厚度方向
		thickness = xLength;

		// Y和Z方向中较大的作为长度，较小的作为宽度
		if (yLength >= zLength) {
			length = yLength;
			width = zLength;
		}
		else {
			length = zLength;
			width = yLength;
		}
	}
	else if (mainVectorText.contains("Y", Qt::CaseInsensitive)) {
		// Y方向是厚度方向
		thickness = yLength;

		// X和Z方向中较大的作为长度，较小的作为宽度
		if (xLength >= zLength) {
			length = xLength;
			width = zLength;
		}
		else {
			length = zLength;
			width = xLength;
		}
	}
	else {
		// Z方向是厚度方向
		thickness = zLength;

		// X和Y方向中较大的作为长度，较小的作为宽度
		if (xLength >= yLength) {
			length = xLength;
			width = yLength;
		}
		else {
			length = yLength;
			width = xLength;
		}
	}

	return true;
}

bool cursePart::calculateOBBDimensionsFromCorners(const std::vector<double>& OBBPosition,
	double& length, double& width, double& thickness)
{
	if (OBBPosition.size() != 24) {
		return false;
	}

	std::vector<Point3D> corners;
	corners.reserve(8);
	for (int i = 0; i < 24; i += 3) {
		corners.emplace_back(OBBPosition[i], OBBPosition[i + 1], OBBPosition[i + 2]);
	}

	const double edge1 = corners[0].distance(corners[1]); // axis[0]
	const double edge2 = corners[0].distance(corners[2]); // axis[1]
	const double edge3 = corners[0].distance(corners[4]); // axis[2]

	std::array<double, 3> dims = { edge1, edge2, edge3 };
	std::sort(dims.begin(), dims.end(), std::greater<double>());

	length = dims[0];
	width = dims[1];
	thickness = dims[2];

	return true;
}


std::vector<double> cursePart::calculateAABBCornersFromPickupMap(const std::map<unsigned long,
	std::vector<std::wstring>>&pickupMap)
{

	std::vector<double> resultPositions;

	// 初始化合并后的包围盒
	AABB mergedBox;
	bool hasValidBox = false;

	// 遍历pickupMap中的每个部件
	for (const auto& pair : pickupMap) {
		unsigned long key = pair.first;  // 零件ID
		const std::vector<std::wstring>& surfaces = pair.second;  // 表面列表

		// 遍历当前零件的所有表面
		for (const auto& surfaceName : surfaces) {
			// 为每个表面获取包围盒
			double dMin[3] = { 0.0 };
			double dMax[3] = { 0.0 };

			CComBSTR bstrSurfaceName(surfaceName.c_str());

			HRESULT hr = m_ptrKit->Part_get_face_bndbox(bstrSurfaceName, dMin, dMax);

			// 调用Part_get_face_bndbox获取单个表面的包围盒
			if(SUCCEEDED(hr)) {
				// 创建当前表面的包围盒
				AABB currentBox;
				currentBox.minPoint = Point3D(dMin[0], dMin[1], dMin[2]);
				currentBox.maxPoint = Point3D(dMax[0], dMax[1], dMax[2]);

				// 合并到总体包围盒
				if (!hasValidBox) {
					mergedBox = currentBox;
					hasValidBox = true;
				}
				else {
					mergedBox.merge(currentBox);
				}

				qDebug() << "表面" << QString::fromStdWString(surfaceName).toLocal8Bit().constData()
					<< "包围盒范围: ";
				qDebug() << "  最小点:" << dMin[0] << dMin[1] << dMin[2];
				qDebug() << "  最大点:" << dMax[0] << dMax[1] << dMax[2];
			}
			else {
				qDebug() << "获取表面" << QString::fromStdWString(surfaceName).toLocal8Bit().constData()
					<< "的包围盒失败";
			}
		}
	}

	if (hasValidBox) {
		// 获取合并后包围盒的8个角点
		std::vector<Point3D> corners = mergedBox.getCorners();

		// 将角点坐标展平为连续数组
		for (const auto& corner : corners) {
			resultPositions.push_back(corner.x);
			resultPositions.push_back(corner.y);
			resultPositions.push_back(corner.z);
		}

		qDebug() << "\n合并后的总体包围盒:";
		qDebug() << "  最小点:" << mergedBox.minPoint.x << mergedBox.minPoint.y << mergedBox.minPoint.z;
		qDebug() << "  最大点:" << mergedBox.maxPoint.x << mergedBox.maxPoint.y << mergedBox.maxPoint.z;
	}
	else {
		qDebug() << "没有找到有效的包围盒";
	}

	return resultPositions;
}

std::vector<double> cursePart::calculateOBBCornersFromPickupMap(const std::map<unsigned long,
	std::vector<std::wstring>>&pickupMap)
{
	std::vector<double> resultPositions;

	std::vector<double> aabbCornerValues = calculateAABBCornersFromPickupMap(pickupMap);
	if (aabbCornerValues.size() < 24) {
		qDebug() << "calculateOBBCornersFromPickupMap: 未获取到有效AABB";
		return resultPositions;
	}

	std::vector<Point3D> aabbCorners;
	aabbCorners.reserve(aabbCornerValues.size() / 3);
	for (size_t i = 0; i + 2 < aabbCornerValues.size(); i += 3) {
		aabbCorners.emplace_back(aabbCornerValues[i], aabbCornerValues[i + 1], aabbCornerValues[i + 2]);
	}

	AABB mergedBox = AABB::calculateAABB(aabbCorners);

	auto addUniquePoint = [](std::vector<Point3D>& points, const Point3D& candidate, double tolerance) {
		for (const auto& p : points) {
			if (p.distance(candidate) <= tolerance) {
				return;
			}
		}
		points.push_back(candidate);
	};

	double boxX = mergedBox.maxPoint.x - mergedBox.minPoint.x;
	double boxY = mergedBox.maxPoint.y - mergedBox.minPoint.y;
	double boxZ = mergedBox.maxPoint.z - mergedBox.minPoint.z;
	double minBoxDim = std::min(std::min(std::abs(boxX), std::abs(boxY)), std::abs(boxZ));
	double pointTolerance = std::max(1e-4, minBoxDim * 1e-4);

	// 1) 以AABB最小轴作为direction，不使用曲面顶点
	std::vector<double> direction = { 1.0, 0.0, 0.0 };
	if (std::abs(boxY) <= std::abs(boxX) && std::abs(boxY) <= std::abs(boxZ)) {
		direction = { 0.0, 1.0, 0.0 };
	}
	else if (std::abs(boxZ) <= std::abs(boxX) && std::abs(boxZ) <= std::abs(boxY)) {
		direction = { 0.0, 0.0, 1.0 };
	}

	normalizeVector(direction);
	if (std::abs(direction[0]) < 1e-9 && std::abs(direction[1]) < 1e-9 && std::abs(direction[2]) < 1e-9) {
		direction = { 0.0, 0.0, 1.0 };
	}

	Point3D rayDirection(direction[0], direction[1], direction[2]);

	// 2) 根据direction在AABB最接近的表面均匀取点
	double rectLength = std::max(1e-3, std::max(std::abs(boxX), std::abs(boxY)) / 10.0);
	double rectWidth = std::max(1e-3, std::max(std::abs(boxY), std::abs(boxZ)) / 10.0);

	std::vector<Point3D> samplePoints;
	std::vector<Point3D> forwardGrid = mergedBox.createGridOnClosestSurface(rectLength, rectWidth, rayDirection, true);
	for (const auto& p : forwardGrid) {
		addUniquePoint(samplePoints, p, pointTolerance);
	}

	for (int i = 0; i < samplePoints.size(); i++) {
		aabbPosition.push_back(samplePoints[i].x);
		aabbPosition.push_back(samplePoints[i].y);
		aabbPosition.push_back(samplePoints[i].z);
	}

	if (samplePoints.empty()) {
		samplePoints = mergedBox.getCorners();
	}

	// 3) 用direction与均匀点做射线，获取与选中曲面的交点
	std::vector<Point3D> rayIntersectionPoints;
	Point3D boxCenter(
		(mergedBox.minPoint.x + mergedBox.maxPoint.x) / 2.0,
		(mergedBox.minPoint.y + mergedBox.maxPoint.y) / 2.0,
		(mergedBox.minPoint.z + mergedBox.maxPoint.z) / 2.0);

	auto getAxisDirTowardCenter = [&](const Point3D& sample) {
		Point3D toCenter(boxCenter.x - sample.x, boxCenter.y - sample.y, boxCenter.z - sample.z);
		double len = std::sqrt(toCenter.x * toCenter.x + toCenter.y * toCenter.y + toCenter.z * toCenter.z);
		if (len < 1e-9) {
			return std::vector<double>{ direction[0], direction[1], direction[2] };
		}

		// Choose the axis direction closest to rayDirection, then orient toward the center.
		std::vector<double> rayDir = { rayDirection.x, rayDirection.y, rayDirection.z };
		normalizeVector(rayDir);
		if (std::abs(rayDir[0]) < 1e-9 && std::abs(rayDir[1]) < 1e-9 && std::abs(rayDir[2]) < 1e-9) {
			rayDir = { direction[0], direction[1], direction[2] };
			normalizeVector(rayDir);
		}

		std::array<std::vector<double>, 6> axisDirs = {
			std::vector<double>{ 1.0, 0.0, 0.0 },
			std::vector<double>{ -1.0, 0.0, 0.0 },
			std::vector<double>{ 0.0, 1.0, 0.0 },
			std::vector<double>{ 0.0, -1.0, 0.0 },
			std::vector<double>{ 0.0, 0.0, 1.0 },
			std::vector<double>{ 0.0, 0.0, -1.0 }
		};

		double bestDot = -1.0;
		std::vector<double> bestAxis = axisDirs[0];
		for (const auto& axis : axisDirs) {
			double dot = rayDir[0] * axis[0] + rayDir[1] * axis[1] + rayDir[2] * axis[2];
			if (dot > bestDot) {
				bestDot = dot;
				bestAxis = axis;
			}
		}

		double towardCenterDot = bestAxis[0] * toCenter.x + bestAxis[1] * toCenter.y + bestAxis[2] * toCenter.z;
		if (towardCenterDot < 0.0) {
			bestAxis[0] = -bestAxis[0];
			bestAxis[1] = -bestAxis[1];
			bestAxis[2] = -bestAxis[2];
		}

		return bestAxis;
	};

	auto collectIntersections = [&]() {
		for (const auto& pair : pickupMap) {
			const std::vector<std::wstring>& surfaces = pair.second;
			for (const auto& surfaceName : surfaces) {
				CComBSTR bstrSurfaceName(surfaceName.c_str());
				for (const auto& sample : samplePoints) {
					double dPosition[3] = { sample.x, sample.y, sample.z };
					std::vector<double> rayVec = getAxisDirTowardCenter(sample);
					double* dIntersectionPoint = nullptr;
					int nArrsize = 0;

					HRESULT hr = m_ptrKit->Part_get_ray_surface_intersetion(
						bstrSurfaceName, dPosition, const_cast<double*>(rayVec.data()),
						&dIntersectionPoint, &nArrsize);

					if (SUCCEEDED(hr) && dIntersectionPoint != nullptr && nArrsize >= 1) {
						Point3D hit(dIntersectionPoint[0], dIntersectionPoint[1], dIntersectionPoint[2]);
						addUniquePoint(rayIntersectionPoints, hit, pointTolerance);

					}

					if (dIntersectionPoint != nullptr) {
						m_ptrKit->PQAPIFree((LONG_PTR*)dIntersectionPoint);
					}
				}
			}
		}
	};
	collectIntersections();

	// 4) 仅使用射线交点作为OBB候选点
	std::vector<Point3D> selectedSurfaceVertices;
	for (const auto& p : rayIntersectionPoints) {
		addUniquePoint(selectedSurfaceVertices, p, pointTolerance);
	}

	if (selectedSurfaceVertices.size() < 3) {
		qDebug() << "calculateOBBCornersFromPickupMap: 顶点候选不足，无法构建OBB";
		return resultPositions;
	}

	for (int i = 0; i < selectedSurfaceVertices.size(); i++) {
		jiaodians.push_back(selectedSurfaceVertices[i].x);
		jiaodians.push_back(selectedSurfaceVertices[i].y);
		jiaodians.push_back(selectedSurfaceVertices[i].z);
	}

	// 5) 使用所有角点候选创建OBB并输出8个角点
	OBB obb = OBB::calculateOBB(selectedSurfaceVertices);
	std::vector<Point3D> obbCorners = obb.getCorners();

	resultPositions.reserve(obbCorners.size() * 3);
	for (const auto& corner : obbCorners) {
		resultPositions.push_back(corner.x);
		resultPositions.push_back(corner.y);
		resultPositions.push_back(corner.z);
	}

	qDebug() << "calculateOBBCornersFromPickupMap: 采样点=" << samplePoints.size()
		<< "交点=" << rayIntersectionPoints.size()
		<< "顶点候选=" << selectedSurfaceVertices.size()
		<< "输出角点=" << obbCorners.size();

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


void cursePart::OnDraw()
{

	if (istest) {

		/*CComBSTR strText1 = "aabb point";
		double dPos1[3] = { 0.0 };
		int counter1 = 0;
		for (size_t i = 0; i < aabbPosition.size(); i++)
		{
			dPos1[counter1++] = aabbPosition[i];
			if ((counter1 % 3) == 0)
			{
				m_ptrKit->View_draw_point(dPos1, 0, 3, RGB(255, 0, 0), strText1, RGB(255, 0, 0));
				counter1 = 0;
			}

		}

		CComBSTR strText2 = "jiaodian point";
		double dPos2[3] = { 0.0 };
		int counter2 = 0;
		for (size_t i = 0; i < jiaodians.size(); i++)
		{
			dPos2[counter2++] = jiaodians[i];
			if ((counter2 % 3) == 0)
			{
				m_ptrKit->View_draw_point(dPos2, 0, 3, RGB(10, 60, 200), strText2, RGB(20, 60, 20));
				counter2 = 0;
			}

		}*/

		/*CComBSTR strText = "point";
		double dPos[3] = { 0.0 };
		int counter = 0;
		for (size_t i = 0; i < OBBPosition.size(); i++)
		{
			dPos[counter++] = OBBPosition[i];
			if ((counter % 3) == 0)
			{
				m_ptrKit->View_draw_point(dPos, 0, 3, RGB(10, 100, 200), strText, RGB(20, 200, 20));
				counter = 0;
			}

		}*/
	}

	if (isPreview) {
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

	// 然后处理拾取结果
	if (isPoint && this) {
		// 可以添加更多条件检查，比如只处理特定类型的元素
		if (i_nEntityType == 8) { // 例如，只处理类型1的元素
			ui->textEdit_7->setPlainText(QString::number(i_dPointY, 'f', 2));
			ui->textEdit_8->setPlainText(QString::number(i_dPointZ, 'f', 2));
			ui->textEdit_6->setPlainText(QString::number(i_dPointX, 'f', 2));

			ui->textEdit_3->setPlainText(QString::number(i_dPointX, 'f', 2));
			ui->textEdit_4->setPlainText(QString::number(i_dPointY, 'f', 2));
			ui->textEdit_5->setPlainText(QString::number(i_dPointZ, 'f', 2));
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
	boundary.CoordinateName = ui->comboBox_3->currentText();
	boundary.DirectionName = ui->comboBox_4->currentText();

	// 添加当前points到boundary
	boundary.points = points;

	QString jsonName = m_tempDir + "workspace_" + robotName + ".json";
	if (m_workspaceHandler) {
		delete m_workspaceHandler;
	}
	m_workspaceHandler = new RobotWorkspaceHandler(jsonName.toStdString());
	m_workspaceHandler->writeRobotWorkspaceBoundary(boundary);
}


