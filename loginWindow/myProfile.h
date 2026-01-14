#pragma once

#include <QDialog>
#include "ui_myProfile.h"
#include "UserManageDlg.h"

QT_BEGIN_NAMESPACE
namespace Ui { class myProfileClass; };
QT_END_NAMESPACE

class myProfile : public QDialog
{
	Q_OBJECT

public:
	myProfile(QString ID, QWidget *parent = nullptr);
	~myProfile();

private:
	Ui::myProfileClass *ui;
	QString m_UserID;
	QMap<QString, UserInfo> m_users;

	void loadUsersFromSettings();
	void flushUserInfoToUI();

private slots:
	void on_btnOK_clicked() { this->accept();  }
};

