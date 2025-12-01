#include "path_edit.h"

path_edit::path_edit(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::path_editClass())
{
	ui->setupUi(this);
}

path_edit::~path_edit()
{
	delete ui;
}
