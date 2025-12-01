#pragma once

#include <QDialog>
#include "ui_path_edit.h"

QT_BEGIN_NAMESPACE
namespace Ui { class path_editClass; };
QT_END_NAMESPACE

class path_edit : public QDialog
{
	Q_OBJECT

public:
	path_edit(QWidget *parent = nullptr);
	~path_edit();

private:
	Ui::path_editClass *ui;
};
