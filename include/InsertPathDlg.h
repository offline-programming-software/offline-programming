#pragma once

#include <QDialog>
#include "ui_InsertPathDlg.h"

class InsertPathDlg : public QDialog
{
	Q_OBJECT

public:
	InsertPathDlg(QWidget *parent = Q_NULLPTR);
	~InsertPathDlg();

	void GetPosture(double& o_dX, double& o_dY, double& o_dZ, double& o_dA, double& o_dB, double& o_dC, int& o_nType);
private:
	Ui::InsertPathDlg ui;
};
