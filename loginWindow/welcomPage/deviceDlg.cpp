#include "deviceDlg.h"

deviceDlg::deviceDlg(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::deviceDlgClass())
{
	ui->setupUi(this);
}

deviceDlg::~deviceDlg()
{
	delete ui;
}

