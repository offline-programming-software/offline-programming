#include "path_partion.h"

path_partion::path_partion(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::path_partionClass())
{
	ui->setupUi(this);
}

path_partion::~path_partion()
{
	delete ui;
}
