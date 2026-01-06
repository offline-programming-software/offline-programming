#include "newFileDlg.h"

newFileDlg::newFileDlg(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::newFileDlgClass())
{
	ui->setupUi(this);
}

newFileDlg::~newFileDlg()
{
	delete ui;
}

