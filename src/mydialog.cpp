#include "mydialog.h"

mydialog::mydialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::mydialogClass())
{
	ui->setupUi(this);
}

mydialog::~mydialog()
{
	delete ui;
}
