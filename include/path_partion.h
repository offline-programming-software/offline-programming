#pragma once

#include <QDialog>
#include "ui_path_partion.h"

QT_BEGIN_NAMESPACE
namespace Ui { class path_partionClass; };
QT_END_NAMESPACE

class path_partion : public QDialog
{
	Q_OBJECT

public:
	path_partion(QWidget *parent = nullptr);
	~path_partion();

private:
	Ui::path_partionClass *ui;
};
