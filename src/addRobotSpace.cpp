#include "addRobotSpace.h"
#include <QMessageBox>
#include "..\include\addRobotSpace.h"

addRobotSpace::addRobotSpace(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::addRobotSpaceClass())
{
	ui->setupUi(this);

	// 初始化：comboBox_3为不可用状态
	ui->comboBox_3->setEnabled(false);

	// 连接checkBox的状态变化信号到槽函数
	connect(ui->checkBox, &QCheckBox::stateChanged,this, &addRobotSpace::onLinkRail);

	connect(ui->pushButton_1, &QPushButton::clicked, this, &addRobotSpace::onCancelButtonClicked);

	connect(ui->pushButton, &QPushButton::clicked, this, &addRobotSpace::onCalculateButtonClicked);
}

addRobotSpace::~addRobotSpace()
{
	delete ui;
}

void addRobotSpace::setRobotName(const QString &robotName)
{
	ui->textBrowser->setPlainText(robotName);
}

void addRobotSpace::setCoordinate(const QStringList & coordList)
{
	ui->comboBox->clear();
	ui->comboBox->addItems(coordList);

	// 可选：设置默认选中项
	if (!coordList.isEmpty()) {
		ui->comboBox->setCurrentIndex(0);
	}
}

void addRobotSpace::setRail(const QStringList & railList)
{
	ui->comboBox_3->clear();
	ui->comboBox_3->addItems(railList);

	// 可选：设置默认选中项
	if (!railList.isEmpty()) {
		ui->comboBox_3->setCurrentIndex(0);
	}
}

QString addRobotSpace::getCoordinate()
{
	return ui->comboBox->currentText();
}

QString addRobotSpace::getMainDir()
{
	return ui->comboBox_2->currentText();
}

QString addRobotSpace::getRail()
{
	return ui->comboBox_3->currentText();
}

bool addRobotSpace::isLink()
{
	return ui->checkBox->isChecked();
}


void addRobotSpace::onLinkRail(int state)
{
	// 根据checkBox的状态设置comboBox_3的可用性
	if (state == Qt::Checked) {
		ui->comboBox_3->setEnabled(true);
	}
	else {
		ui->comboBox_3->setEnabled(false);
	}
}

void addRobotSpace::onCancelButtonClicked()
{
	// 关闭对话框并返回Rejected结果
	this->reject();
}

void addRobotSpace::onCalculateButtonClicked()
{
	// 可选：添加数据验证
	if (ui->comboBox->currentText().isEmpty()) {
		QMessageBox::warning(this, "警告", "请先选择坐标系！");
		return;
	}

	// 如果comboBox_3已启用且需要验证其内容
	if (ui->comboBox_3->isEnabled() && ui->comboBox_3->currentText().isEmpty()) {
		QMessageBox::warning(this, "警告", "请先选择第二个坐标系！");
		return;
	}

	// 发送计算信号
	emit calculateRequested();

	// 可选：显示计算开始的消息
	// QMessageBox::information(this, "信息", "计算请求已发送");
}
