#pragma once

#include <QDialog>
#include "ui_posCal.h"

QT_BEGIN_NAMESPACE
namespace Ui { class posCalClass; };
QT_END_NAMESPACE

class posCal : public QDialog
{
	Q_OBJECT

public:
	posCal(QWidget *parent = nullptr);
	~posCal();

private:
	Ui::posCalClass *ui;
};
