#pragma once

#include <QDialog>
#include <QMap>
#include <QString>
#include "ui_loginDlg.h"

QT_BEGIN_NAMESPACE
namespace Ui { class loginDlgClass; };
QT_END_NAMESPACE

class loginDlg : public QDialog
{
	Q_OBJECT

public:
	loginDlg(QWidget *parent = nullptr);
	~loginDlg();

	QString getID() { return m_userID; }

private slots:
	void on_btnLogin_clicked();

private:
	void loadUsersFromSettings();
	QString m_userID;

	Ui::loginDlgClass *ui;
	QMap<QString, QString> m_userCredentials;
};

