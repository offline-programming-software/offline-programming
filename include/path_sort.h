#pragma once

#include <QDialog>
#include "ui_path_sort.h"

QT_BEGIN_NAMESPACE
namespace Ui { class path_sortClass; };
QT_END_NAMESPACE

class path_sort : public QDialog
{
	Q_OBJECT

public:
	path_sort(QWidget *parent = nullptr);
	~path_sort();

private:
	Ui::path_sortClass *ui;
};
