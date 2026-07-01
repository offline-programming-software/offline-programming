#include "testWidget.h"

testWidget::testWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::testWidgetClass())
{
	ui->setupUi(this);
}

testWidget::~testWidget()
{
	delete ui;
}

