#pragma once

#include <QDialog>
#include "ui_AGVpath.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AGVpathClass; };
QT_END_NAMESPACE

class AGVpath : public QDialog
{
	Q_OBJECT

public:
	AGVpath(QWidget *parent = nullptr);
	~AGVpath();

private:
	Ui::AGVpathClass *ui;
};
