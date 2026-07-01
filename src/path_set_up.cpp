#include "path_set_up.h"

path_set_up::path_set_up(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::path_set_upClass())
{
	ui->setupUi(this);
}

path_set_up::~path_set_up()
{
	delete ui;
}
