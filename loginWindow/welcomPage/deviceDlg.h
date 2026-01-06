#pragma once

#include <QDialog>
#include "ui_deviceDlg.h"

QT_BEGIN_NAMESPACE
namespace Ui { class deviceDlgClass; };
QT_END_NAMESPACE

class deviceDlg : public QDialog
{
	Q_OBJECT

public:
	deviceDlg(QWidget *parent = nullptr);
	~deviceDlg();

private:
	Ui::deviceDlgClass *ui;
};

