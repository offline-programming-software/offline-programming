#pragma once

#include <QDialog>
#include "ui_path_optimize.h"

QT_BEGIN_NAMESPACE
namespace Ui { class path_optimizeClass; };
QT_END_NAMESPACE

class path_optimize : public QDialog
{
	Q_OBJECT

public:
	path_optimize(QWidget *parent = nullptr);
	~path_optimize();

private:
	Ui::path_optimizeClass *ui;
};
