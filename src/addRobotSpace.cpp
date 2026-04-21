#include "addRobotSpace.h"
#include <QMessageBox>
#include "..\include\addRobotSpace.h"

addRobotSpace::addRobotSpace(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::addRobotSpaceClass())
{
	ui->setupUi(this);

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

QString addRobotSpace::getCoordinate()
{
	return ui->comboBox->currentText();
}

QString addRobotSpace::getMainDir()
{
	return ui->comboBox_2->currentText();
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

	// 发送计算信号
	emit calculateRequested();

}
