#pragma once

#include <QDialog>
#include "ui_path_set_up.h"

QT_BEGIN_NAMESPACE
namespace Ui { class path_set_upClass; };
QT_END_NAMESPACE

class path_set_up : public QDialog
{
	Q_OBJECT

public:
	path_set_up(QWidget *parent = nullptr);
	~path_set_up();

private:
	Ui::path_set_upClass *ui;
};
