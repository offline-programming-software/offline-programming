#include "posCal.h"

posCal::posCal(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::posCalClass())
{
	ui->setupUi(this);
}

posCal::~posCal()
{
	delete ui;
}
