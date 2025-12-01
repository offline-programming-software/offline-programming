#include "path_optimize.h"

path_optimize::path_optimize(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::path_optimizeClass())
{
	ui->setupUi(this);
}

path_optimize::~path_optimize()
{
	delete ui;
}
