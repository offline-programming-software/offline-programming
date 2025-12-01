#pragma once

#include <QDialog>
#include "ui_mydialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class mydialogClass; };
QT_END_NAMESPACE

class mydialog : public QDialog
{
	Q_OBJECT

public:
	mydialog(QWidget *parent = nullptr);
	~mydialog();

private:
	Ui::mydialogClass *ui;
};
