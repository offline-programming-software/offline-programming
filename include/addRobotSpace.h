#pragma once

#include <QDialog>
#include "ui_addRobotSpace.h"

QT_BEGIN_NAMESPACE
namespace Ui { class addRobotSpaceClass; };
QT_END_NAMESPACE

class addRobotSpace : public QDialog
{
	Q_OBJECT

public:
	addRobotSpace(QWidget *parent = nullptr);
	~addRobotSpace();

private:
	Ui::addRobotSpaceClass *ui;
};
