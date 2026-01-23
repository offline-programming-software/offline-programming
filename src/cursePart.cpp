#include "cursePart.h"

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

	// 连接按钮信号到槽函数
	setupConnections();

	// 设置步骤解释
	setStepsExplanation();

	//初始化数据
	init();

	isPickupActive = false; // 重置拾取状态
	isPreview = false;//是否进行预览

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		delete ui;
		return;
	}

	// 将机器人名称设置到对话框中
	for (const QString& name : robotNames) {
		curseDialog->setRobotOptions(name);
	}


	// 连接机器人选择改变信号
	connect(curseDialog, &cursePart::robotSelectionChanged, this, [this, robotMap](const QString& robotName) {
		updateRailOptions(robotName, robotMap);
	});

	// 初始设置轨道信息
	QString currentRobot = robotNames.isEmpty() ? "" : robotNames.first();
	updateRailOptions(currentRobot, robotMap);


	// 对于曲面选取
	curseDialog->setModal(true);
	curseDialog->setWindowModality(Qt::ApplicationModal);

	// 连接拾取按钮信号 - 启动拾取模块
	connect(curseDialog, &cursePart::pickUpSignal, this, [this]() {
		if (!isPickupActive && !isPreview) {
			// 启动拾取模块
			CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
			HRESULT hr = m_ptrKit->Doc_start_module(cmd);
			if (SUCCEEDED(hr)) {
				isPickupActive = true;
				isPreview = false; // 确保预览模式关闭
				curseDialog->setModal(false);
				curseDialog->setWindowModality(Qt::NonModal);
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
	});

	// 连接关闭拾取按钮信号
	connect(curseDialog, &cursePart::closeSignal, this, [this]() {
		if (isPickupActive || isPreview) {
			isPickupActive = false;
			isPreview = false;
			curseDialog->setModal(true);
			CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
			HRESULT hr = m_ptrKit->Doc_end_module(cmd);
			curseDialog->setWindowModality(Qt::ApplicationModal);
			qDebug() << "拾取模块已停止";
		}
	});

	// 删除信号
	connect(curseDialog, &cursePart::deleteSelectedSurfaces, this, &MainWindow::onDeleteSelectedSurfaces);

	// 计算选中曲面的最小包围盒
	connect(curseDialog, &cursePart::calculateAABB, this, [this]() {
		for (const auto& pair : pickupMap) {
			unsigned long key = pair.first;
			const std::vector<std::wstring>& values = pair.second;
			long lCount = 0;
			m_ptrKit->PQAPIGetWorkPartVertexCount(key, &lCount);//获取顶点个数
			std::vector<double> dSrc(3 * lCount, 0);
			double* dSrcPosition = dSrc.data();//顶点位置（通过一维数组表示）
			BSTR sName;
			m_ptrKit->Doc_get_obj_name(key, &sName);
			m_ptrKit->PQAPIGetWorkPartVertex(key, 0, lCount, dSrcPosition);//获取顶点位置
			for (const auto& value : values) {
				for (long i = 0; i < lCount; i++) {
					double dPosition[3] = { dSrcPosition[3 * i],dSrcPosition[3 * i + 1],dSrcPosition[3 * i + 2] };
					double dTol = 10;
					LONG bPointOnSurface = 0;
					std::vector<wchar_t> buffer(value.begin(), value.end());
					buffer.push_back(L'\0'); // 添加终止符
					LPWSTR name = buffer.data();
					m_ptrKit->Part_cheak_point_on_surface(name, dPosition, dTol, &bPointOnSurface);//检查顶点是否在曲面上

					if (bPointOnSurface) {
						m_vPosition.push_back(dPosition[0]);
						m_vPosition.push_back(dPosition[1]);
						m_vPosition.push_back(dPosition[2]);
					}
				}
			}
		}
		box.minPoint = { 0,0,0 };
		box.maxPoint = { 0,0,0 };

		std::vector<Point3D> curse;
		for (int i = 0; i < m_vPosition.size(); i += 3) {
			Point3D p;
			p.x = m_vPosition[i];
			p.y = m_vPosition[i + 1];
			p.z = m_vPosition[i + 2];
			curse.push_back(p);
		}

		box = calculateAABB(curse);

		std::vector<Point3D> box_8 = box.getCorners();

		for (int i = 0; i < 8; i++) {
			ABBPosition.push_back(box_8[i].x);
			ABBPosition.push_back(box_8[i].y);
			ABBPosition.push_back(box_8[i].z);
		}
	});

	// 设置坐标系
	PQDataType CoodernateType = PQ_COORD;
	QMap<ULONG, QString> CoodernateMap = getObjectsByType(CoodernateType);

	// 创建一个新的QMap，先插入"世界坐标系"，再插入原有的数据
	QMap<ULONG, QString> newCoodernateMap;
	newCoodernateMap.insert(0, "世界坐标系");  // 先插入首位

	// 将原有数据插入到后面（键值从1开始）
	for (auto it = CoodernateMap.begin(); it != CoodernateMap.end(); ++it) {
		newCoodernateMap.insert(it.key(), it.value());
	}

	CoodernateMap = newCoodernateMap;  // 替换原来的map
	QStringList CoodernateNames = CoodernateMap.values();
	curseDialog->setCoodernateOptions(CoodernateNames);

	//创建点阵
	connect(curseDialog, &cursePart::spaceSetting, this, [this, CoodernateMap]() {

		//获取设置好的坐标轴

		QString coordanateName = curseDialog->getCoodernateSelection();
		ULONG selectCoorID = CoodernateMap.key(coordanateName);


		//记录坐标系的位置和姿态  采用欧拉角XYZ表示
		double coordanate[6];
		if (selectCoorID == 0) {
			for (int i = 0; i < 6; i++) {
				coordanate[i] = 0;
			}
		}
		else {

			int nCount = 0;
			double* dPosture = nullptr;
			m_ptrKit->Doc_get_coordinate_posture(selectCoorID, EULERANGLEXYZ, &nCount, &dPosture, 0);

			for (int i = 0; i < 6; i++) {
				coordanate[i] = dPosture[i];
			}

			m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
		}


		std::vector<std::vector<double>> axisVector;
		axisVector = getCoordinateAxesFromEuler(coordanate);

		//根据选择的主法矢选择坐标轴方向向量
		QString mainVectorText = curseDialog->getComboBox_4();
		std::vector<double> mainVector = getAxisVector(axisVector, mainVectorText);

		//根据选择的主划分方向
		QString mainDivisionDirectionText = curseDialog->getComboBox_5();
		std::vector<double> mainDivisionDirection = getAxisVector(axisVector, mainDivisionDirectionText);

		// 2. 生成点阵参数                
		double spacing = curseDialog->geteditSelection().toDouble();

		Point3D viewDirection(mainVector[0], mainVector[1], mainVector[2]); // 从Y轴方向观看
		std::vector<Point3D> corners = box.getCorners();
		// 自动选择最近的面并生成点阵
		std::vector<Point3D> result = createGridOnClosestSurface(corners, spacing, spacing, viewDirection);
		double* dIntersetionpoint = nullptr;

		double maxtheta = 0;
		for (const auto& key : pickupMap) {
			unsigned long k = key.first;
			for (const auto value : key.second) {
				CComBSTR whSurfaceName = value.c_str();
				double* dIntersetionpoint = nullptr;
				int nArrsize = 1;
				for (Point3D P : result) {
					double dPosition[3] = { P.x,P.y,P.z };
					m_ptrKit->Part_get_ray_surface_intersetion(whSurfaceName, dPosition, mainVector.data(),
						&dIntersetionpoint, &nArrsize);
					Eigen::Vector3d v1(mainVector[0], mainVector[1], mainVector[2]);
					Eigen::Vector3d v2(dIntersetionpoint[3], dIntersetionpoint[4], dIntersetionpoint[5]);
					Eigen::Vector3d v1_norm = v1.normalized();
					Eigen::Vector3d v2_norm = v2.normalized();

					// 计算点积并限制范围
					double dot = v1_norm.dot(v2_norm);
					dot = std::max(-1.0, std::min(1.0, dot));

					maxtheta = std::max(std::acos(dot), maxtheta);
				}
				m_ptrKit->PQAPIFree((LONG_PTR*)dIntersetionpoint);
			}
		}
		maxtheta = maxtheta * 180 / M_PI;
		curseDialog->setTextBrowser2(QString("%1").arg(maxtheta) + "°");

	});

	connect(curseDialog, &cursePart::calculateSpace, this, [this]() {


		Point3D direction(0, 1, 0);
		curseDialog->setTextEdit("500");
		curseDialog->setTextEdit2("500");
		int length = curseDialog->getTextEdit().toDouble();
		int width = curseDialog->getTextEdit2().toDouble();
		auto grid = createGridOnClosestSurface(box, length, width, direction);

		for (auto p : grid) {
			points.push_back(p.x);
			points.push_back(p.y);
			points.push_back(p.z);
		}

		QString value1 = QString("%1").arg(points[0]);
		QString value2 = QString("%1").arg(points[1]);
		QString value3 = QString("%1").arg(points[2]);

		curseDialog->setTextEditValues(value1, value2, value3);
	});

	connect(curseDialog, &cursePart::previewSignal, this, [this]() {
		// 启动拾取模块
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		HRESULT hr = m_ptrKit->Doc_start_module(cmd);
		if (SUCCEEDED(hr)) {
			isPoint = true;
		}

	});

	connect(curseDialog, &cursePart::areaPosition, this, [this]() {
		std::vector<double> areaPosition;
		areaPosition = curseDialog->getVertexValues();
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
	});

	// 对话框关闭时清理资源
	connect(curseDialog, &cursePart::finished, this, [this](int result) {
		Q_UNUSED(result)
			// 停止任何正在运行的模块
			if (isPickupActive || isPreview) {
				CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
				m_ptrKit->Doc_end_module(cmd);
			}
		isPickupActive = false;
		isPreview = false;
		curseDialog = nullptr;
	});


}

cursePart::~cursePart()
{
	delete ui;
}

void cursePart::init() {
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

	ui->comboBox_3->clear();
	ui->comboBox_1->clear();

	ui->comboBox_5->addItem("Y轴方向");
	ui->comboBox_5->addItem("Z轴方向");

}

void cursePart::setupConnections()
{
	// 页面导航按钮
	//上一页按钮
	QPushButton* prevButton = ui->pushButton_1;
	connect(prevButton, &QPushButton::clicked, this, &cursePart::on_prev_page_clicked);

	//下一页按钮
	QPushButton* nextButton =  ui->pushButton_2 ;
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

	connect(ui->pushButton_3, &QPushButton::clicked, this, &cursePart::on_previewButton_clicked);
	connect(ui->pushButton_9, &QPushButton::clicked, this, &cursePart::on_spaceSettingButton_clicked);

	// 组合框和文本编辑框
	connect(ui->comboBox_1, &QComboBox::currentTextChanged, this, &cursePart::on_comboBox_currentTextChanged);
	connect(ui->comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &cursePart::on_coordanateTextChanged);
	connect(ui->textEdit, &QTextEdit::textChanged, this, &cursePart::on_textEdit_4_textChanged);

	//设置是否联动
	connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool checked) {
		ui->comboBox_2->setEnabled(checked);
	});

	connect(ui->horizontalSlider, &QSlider::valueChanged, this, &cursePart::on_horizontalSlider_valueChanged);
	connect(ui->verticalSlider, &QSlider::valueChanged, this, &cursePart::on_verticalSlider_valueChanged);
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
	QGraphicsPixmapItem* pixmapItem3  = scene3->addPixmap(pixmap3);

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

void cursePart::setRobotOptions(const QStringList& robotOptions)
{
	ui->comboBox_1->addItems(robotOptions);
	if (!robotOptions.isEmpty()) {
		ui->comboBox_1->setCurrentIndex(0);
	}
}

void cursePart::setRobotOptions(const QString& robotOption)
{
	if (!robotOption.isEmpty()) {
		ui->comboBox_1->addItem(robotOption);
		ui->comboBox_1->setCurrentIndex(0);
	}
}

void cursePart::setCoodernateOptions(const QStringList& coordinateOptions)
{
	ui->comboBox_3->addItems(coordinateOptions);
	if (!coordinateOptions.isEmpty()) {
		ui->comboBox_3->setCurrentIndex(0);
	}
}

void cursePart::setCoodernateOptions(const QString& coordinateOption)
{
	
	if (!coordinateOption.isEmpty()) {
		ui->comboBox_3->addItem(coordinateOption);
		ui->comboBox_3->setCurrentIndex(0);
	}
}

QString cursePart::getCoodernateSelection() const
{
	return ui->comboBox_3->currentText();
}

QString cursePart::getRobotSelection() const
{
	return ui->comboBox_1->currentText();
}

void cursePart::setRailOptions(const QStringList& railOptions)
{
	ui->comboBox_2->clear();
	ui->comboBox_2->addItems(railOptions);
	if (!railOptions.isEmpty()) {
		ui->comboBox_2->setCurrentIndex(0);
	}
}

void cursePart::setRailOptions(const QString& railOption)
{
	ui->comboBox_2->clear();
	if (!railOption.isEmpty()) {
		ui->comboBox_2->addItem(railOption);
		ui->comboBox_2->setCurrentIndex(0);
	}
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

void cursePart::addItemsToListView(const QStringList& items)
{
	QAbstractItemModel* model = ui->listView->model();
	if (!model) {
		model = new QStringListModel(this);
		ui->listView->setModel(model);
	}

	QStringListModel* listModel = qobject_cast<QStringListModel*>(model);
	if (listModel) {
		QStringList currentItems = listModel->stringList();
		currentItems.append(items);
		listModel->setStringList(currentItems);
	}
}

QString cursePart::getComboBox_4() const
{
	return ui->comboBox_4->currentText();
}

QString cursePart::getComboBox_5() const
{
	return ui->comboBox_5->currentText();
}

QString cursePart::geteditSelection() const
{
	return ui->textEdit->toPlainText();
}

void cursePart::on_textEdit_4_textChanged()
{
	QString text = ui->textEdit->toPlainText().trimmed();
	// 文本改变处理逻辑
}

void cursePart::on_horizontalSlider_valueChanged(int value)
{

	// 计算百分比变化：value范围是-50到50，对应-50%到+50%
	double percentage = value / 100.0; // 转换为小数形式

	// 计算新值：500 + 500 * 百分比
	double newValue = x_value + 500 * percentage;

	// 更新textEdit_3的显示
	ui->textEdit_3->setPlainText(QString::number(newValue, 'f', 2));

	emit areaPosition();
}

void cursePart::on_verticalSlider_valueChanged(int value)
{

	// 计算百分比变化：value范围是-50到50，对应-50%到+50%
	double percentage = value / 100.0; // 转换为小数形式

	// 计算新值：500 + 500 * 百分比
	double newValue = z_value + 500 * percentage;

	// 更新textEdit_3的显示
	ui->textEdit_5->setPlainText(QString::number(newValue, 'f', 2));

	emit areaPosition();
}

void cursePart::on_coordanateTextChanged(int state)
{
	// 阻塞 comboBox_5 的信号，防止在清空和添加项时触发不必要的信号（如 currentIndexChanged）
	ui->comboBox_5->blockSignals(true);

	// 清空 comboBox_5
	ui->comboBox_5->clear();

	// 根据状态添加不同的项
	if (state < 2) {
		ui->comboBox_5->addItem("Y轴方向");
		ui->comboBox_5->addItem("Z轴方向");
	}
	else if(1 < state && state < 4) {
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
	emit confirm();
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

	emit closeSignal();
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
	emit cancel(QDialog::Rejected);
	this->close();
}


void cursePart::on_pickUpButton_clicked()
{
	emit pickUpSignal();
}

void cursePart::on_finishButton_clicked()
{
	emit calculateAABB();
	emit closeSignal();
}

void cursePart::on_previewButton_clicked()
{
	emit previewSignal();
}

void cursePart::on_spaceSettingButton_clicked()
{
	QString text = ui->textEdit->toPlainText().trimmed();
	bool ok;
	int number = text.toInt(&ok);

	if (ok) {
		emit spaceSetting(number);
		qDebug() << "Valid number:" << number;
		emit calculateSpace();
	}
	else {
		qDebug() << "Invalid input";
		QMessageBox::warning(this, "Warning", "Invalid input");
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
		emit deleteSelectedSurfaces(deletedSurfaceNames);
	}
}

void cursePart::on_comboBox_currentTextChanged(const QString& text)
{
	emit robotSelectionChanged(text);
}

void cursePart::setTextBrowser2(const QString& text)
{
	ui->textBrowser_1->setPlainText(text);
}

// 设置textEdit的值
void cursePart::setTextEdit(const QString& text)
{
	ui->textEdit_1->setPlainText(text);
}

void cursePart::setTextEdit2(const QString& text)
{
	ui->textEdit_2->setPlainText(text);
}

// 获取textEdit的值
QString cursePart::getTextEdit() const
{
	return ui->textEdit_1->toPlainText();
}

QString cursePart::getTextEdit2() const
{
	return ui->textEdit_2->toPlainText();
}


void cursePart::setTextEditValues(const QString& text3, const QString& text5, const QString& text6)
{
	ui->textEdit_3->setPlainText(text3);
	ui->textEdit_4->setPlainText(text5);
	ui->textEdit_5->setPlainText(text6);

	x_value = text3.toDouble();
	y_value = text5.toDouble();
	z_value = text6.toDouble();
}

std::vector<double> cursePart::getVertexValues()
{
	std::vector<double> result;
	double x_value = ui->textEdit_3->toPlainText().toDouble();
	double y_value = ui->textEdit_4->toPlainText().toDouble();
	double z_value = ui->textEdit_5->toPlainText().toDouble();

	result.push_back(x_value);
	result.push_back(y_value);
	result.push_back(z_value);
	return result;
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
