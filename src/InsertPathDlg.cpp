#include "InsertPathDlg.h"

InsertPathDlg::InsertPathDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	//添加顺序需要匹配api手册9.2
	QStringList strList;
	strList << "Quaternion" << "Euler angles XYZ" << "Euler angles ZYX" << "Euler angles ZXZ" << "Euler angles ZYZ";
	ui.comboBox->addItems(strList);
	
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

InsertPathDlg::~InsertPathDlg()
{
}

void InsertPathDlg::GetPosture(double& o_dX, double& o_dY, double& o_dZ, double& o_dA, double& o_dB, double& o_dC, int& o_nType)
{
	o_dX = ui.lineEditPtX->text().toDouble();
	o_dY = ui.lineEditPtY->text().toDouble();
	o_dZ = ui.lineEditPtZ->text().toDouble();
	o_dA = ui.lineEditPtA->text().toDouble();
	o_dB = ui.lineEditPtB->text().toDouble();
	o_dC = ui.lineEditPtC->text().toDouble();

	o_nType = ui.comboBox->currentIndex();
}
