#include "AGVpath.h"

AGVpath::AGVpath(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::AGVpathClass())
{
	ui->setupUi(this);
}

AGVpath::~AGVpath()
{
	delete ui;
}
