#pragma once

#include <QDialog>
#include "ui_deformation_correction.h"

QT_BEGIN_NAMESPACE
namespace Ui { class deformation_correctionClass; };
QT_END_NAMESPACE

class deformation_correction : public QDialog
{
	Q_OBJECT

public:
	deformation_correction(QWidget *parent = nullptr);
	~deformation_correction();

private:
	Ui::deformation_correctionClass *ui;
};
