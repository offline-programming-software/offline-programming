#pragma once

#include <QWidget>
#include "ui_testWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class testWidgetClass; };
QT_END_NAMESPACE

class testWidget : public QWidget
{
	Q_OBJECT

public:
	testWidget(QWidget *parent = nullptr);
	~testWidget();

private:
	Ui::testWidgetClass *ui;
};

