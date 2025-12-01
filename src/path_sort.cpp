#include "path_sort.h"

path_sort::path_sort(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::path_sortClass())
{
	ui->setupUi(this);
}

path_sort::~path_sort()
{
	delete ui;
}
