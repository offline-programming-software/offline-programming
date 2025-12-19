#include "robotSpaceDefine.h"

robotSpaceDefine::robotSpaceDefine(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::robotSpaceDefineClass()),
	axisModel(new QStandardItemModel(this))
{
	ui->setupUi(this);


	// 初始化表格视图
	setupTableView();

	// 连接信号槽（假设UI中有添加和删除按钮）
	connect(ui->pushButton, &QPushButton::clicked, this, &robotSpaceDefine::onAddAxis);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &robotSpaceDefine::onDeleteAxis);
	connect(ui->pushButton_4, &QPushButton::clicked, this, &robotSpaceDefine::onDeleteAxis);
}

robotSpaceDefine::~robotSpaceDefine()
{
	delete ui;
}

void robotSpaceDefine::setupTableView()
{
	// 设置表头
	QStringList headers;
	headers << "序号" << "坐标系名称" << "法向量" << "是否有导轨" << "导轨名称";
	axisModel->setHorizontalHeaderLabels(headers);

	// 设置模型
	ui->tableView->setModel(axisModel);

	// 设置选择行为
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

	// 设置列宽
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->setColumnWidth(0, 60);  
	ui->tableView->setColumnWidth(1, 100);
	ui->tableView->setColumnWidth(2, 100);
	ui->tableView->setColumnWidth(3, 80);  
	ui->tableView->setColumnWidth(4, 120);
}

void robotSpaceDefine::addAxisInfo(int number,const QString& axisName, 
	const QString& mainNormalVector,bool hasGuideRail, const QString guideName)
{
	AxisData data;
	data.number = number;
	data.axisName = axisName;
	data.mainNormalVector = mainNormalVector;
	data.hasGuideRail = hasGuideRail;
	data.guideName = guideName;

	axisList.append(data);
	updateTableView();
}

void robotSpaceDefine::setRobotOptions(const QString & robotOption)
{
	if (!robotOption.isEmpty()) {
		ui->comboBox->addItem(robotOption);
		ui->comboBox->setCurrentIndex(0);
	}
}

QString robotSpaceDefine::getRobotName()
{
	QString RobotName = ui->comboBox->currentText();
	return RobotName;
}

void robotSpaceDefine::updateTableView()
{
	axisModel->removeRows(0, axisModel->rowCount());

	for (int i = 0; i < axisList.size(); ++i) {
		const AxisData& data = axisList.at(i);

		QList<QStandardItem*> items;

		// 自动生成序号（从1开始）
		QStandardItem* numberItem = new QStandardItem(QString::number(i + 1));
		numberItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(numberItem);

		// 轴名称
		QStandardItem* nameItem = new QStandardItem(data.axisName);
		nameItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(nameItem);

		// 法向量
		QStandardItem* normalItem = new QStandardItem(data.mainNormalVector);
		normalItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(normalItem);

		// 是否有导轨
		QStandardItem* railItem = new QStandardItem(data.hasGuideRail ? "是" : "否");
		railItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(railItem);

		// 导轨名称
		QStandardItem* speedItem = new QStandardItem(data.guideName);
		speedItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐（之前是右对齐）
		items.append(speedItem);

		axisModel->appendRow(items);
	}
}

void robotSpaceDefine::onAddAxis()
{
	// 添加默认坐标轴信息
	/*addAxisInfo("世界坐标系", "X轴正方向", true, 100.0);*/
	addRobotSpace dlg;
	QString robotName = getRobotName();
	dlg.setRobotName(robotName);
	dlg.setModal(true);
	dlg.exec();
}

void robotSpaceDefine::onDeleteAxis()
{
	QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
	if (selected.isEmpty()) {
		QMessageBox::warning(this, "警告", "请选择要删除的坐标轴");
		return;
	}

	int row = selected.first().row();
	if (row >= 0 && row < axisList.size()) {
		axisList.removeAt(row);
		updateTableView();
	}
}