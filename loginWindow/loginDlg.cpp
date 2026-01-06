#include "loginDlg.h"
#include <QSettings>
#include "QMessageBox"
#include<QStringList>

loginDlg::loginDlg(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::loginDlgClass())
{
	ui->setupUi(this);
	loadUsersFromSettings();
}

loginDlg::~loginDlg()
{
	delete ui;
}

void loginDlg::loadUsersFromSettings()
{
	m_userCredentials.clear();
	QSettings settings("offline-programming", "robot2022");
	
	int size = settings.beginReadArray("Users");
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		QString id = settings.value("id", "").toString();
		QString pwd = settings.value("pwd", "").toString();
		
		if (!id.isEmpty()) {
			m_userCredentials[id] = pwd;
		}
	}
	settings.endArray();
	
	// If no users loaded from settings, add default admin user
	if (m_userCredentials.isEmpty()) {
		m_userCredentials["admin"] = "123456";
	}
}

void loginDlg::on_btnLogin_clicked() {
	QString username = ui->edtID->text();
	QString password = ui->edtPWD->text();
	if (m_userCredentials.contains(username))
	{
		if (m_userCredentials[username] == password)
		{
			this->accept();
		}
		else
		{
			QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("密码错误！"));
		}
	}
	else
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("用户名不存在！"));
	}
}

