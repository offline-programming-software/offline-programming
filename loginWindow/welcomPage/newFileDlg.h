#pragma once

#include <QDialog>
#include "ui_newFileDlg.h"

QT_BEGIN_NAMESPACE
namespace Ui { class newFileDlgClass; };
QT_END_NAMESPACE

class newFileDlg : public QDialog
{
	Q_OBJECT

public:
	newFileDlg(QWidget *parent = nullptr);
	~newFileDlg();

	bool	 isPainting() { return ui->rdoPainting->isChecked(); }
	QString name() { return ui->edtName->text() ; }
	QString creator() { return ui->edtCreator->text(); }
	QString description() { return ui->edtNote->toPlainText(); }

private:
	Ui::newFileDlgClass *ui;

};

