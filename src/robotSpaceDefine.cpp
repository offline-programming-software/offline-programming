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
	headers << "序号" << "坐标轴选取" << "主法矢" << "是否存在导轨" << "联动导轨" ;
	axisModel->setHorizontalHeaderLabels(headers);

	// 设置表格模型
	ui->tableView->setModel(axisModel);

	// 设置表格属性
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

	// 设置列宽
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->setColumnWidth(0, 100); // 坐标轴名称列宽
	ui->tableView->setColumnWidth(1, 100);
	ui->tableView->setColumnWidth(2, 100); // 主法序列宽
	ui->tableView->setColumnWidth(3, 120); // 是否存在导轨列宽
	ui->tableView->setColumnWidth(4, 120); // 导轨速度列宽
}

void robotSpaceDefine::addAxisInfo(const QString& axisName, const QString& mainNormalVector,
	bool hasGuideRail, double guideSpeed)
{
	AxisData data;
	data.axisName = axisName;
	data.mainNormalVector = mainNormalVector;
	data.hasGuideRail = hasGuideRail;
	data.guideSpeed = guideSpeed;

	axisList.append(data);
	updateTableView();
}

void robotSpaceDefine::updateTableView()
{
	axisModel->removeRows(0, axisModel->rowCount());

	for (int i = 0; i < axisList.size(); ++i) {
		const AxisData& data = axisList.at(i);

		QList<QStandardItem*> items;

		// 坐标轴名称
		QStandardItem* nameItem = new QStandardItem(data.axisName);
		items.append(nameItem);

		// 主法矢
		QStandardItem* normalItem = new QStandardItem(data.mainNormalVector);
		items.append(normalItem);

		// 是否存在导轨
		QStandardItem* railItem = new QStandardItem(data.hasGuideRail ? "是" : "否");
		items.append(railItem);

		// 导轨速度
		QStandardItem* speedItem = new QStandardItem(QString::number(data.guideSpeed, 'f', 2));
		speedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		items.append(speedItem);

		axisModel->appendRow(items);
	}
}

void robotSpaceDefine::onAddAxis()
{
	// 添加默认坐标轴信息
	/*addAxisInfo("世界坐标系", "X轴正方向", true, 100.0);*/
	addRobotSpace dlg;
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