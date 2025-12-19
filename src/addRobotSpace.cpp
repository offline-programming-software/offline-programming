#include "addRobotSpace.h"

addRobotSpace::addRobotSpace(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::addRobotSpaceClass())
{
	ui->setupUi(this);
}

addRobotSpace::~addRobotSpace()
{
	delete ui;
}

void addRobotSpace::setRobotName(const QString &robotName)
{
	ui->textBrowser->setPlainText(robotName);
}
