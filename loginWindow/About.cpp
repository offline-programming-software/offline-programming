#include "About.h"

About::About(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::AboutClass())
{
	ui->setupUi(this);
}

About::~About()
{
	delete ui;
}

