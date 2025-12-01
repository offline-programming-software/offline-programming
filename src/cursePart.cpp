#include "cursePart.h"

cursePart::cursePart(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::cursePartClass())
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


