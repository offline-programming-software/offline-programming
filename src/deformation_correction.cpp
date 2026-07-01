#include "deformation_correction.h"

deformation_correction::deformation_correction(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::deformation_correctionClass())
{
	ui->setupUi(this);
}

deformation_correction::~deformation_correction()
{
	delete ui;
}
