#include "myProfile.h"
#include<QSettings>

myProfile::myProfile(QString ID,QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::myProfileClass())
	, m_UserID(ID)
{
	ui->setupUi(this);
	loadUsersFromSettings();
	flushUserInfoToUI();
	

}

myProfile::~myProfile()
{
	delete ui;
}

void myProfile::loadUsersFromSettings()
{
	m_users.clear();
	QSettings settings("offline-programming", "robot2022");

	int size = settings.beginReadArray("Users");
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		UserInfo info;
		info.id = settings.value("id", "").toString();
		info.pwd = settings.value("pwd", "").toString();
		info.name = settings.value("name", "").toString();
		info.apart = settings.value("apart", "").toString();
		info.contact = settings.value("contact", "").toString();
		info.workID = settings.value("workID", "").toString();
		info.userLevel = static_cast<level>(settings.value("userLevel", "").toInt());

		if (!info.id.isEmpty()) {
			m_users[info.id] = info;
		}
	}
	settings.endArray();
}

void myProfile::flushUserInfoToUI()
{
	UserInfo info;
	info = m_users[m_UserID];
	QString myID;
	int myLevel;
	myLevel = static_cast<int>(info.userLevel);
	myID = info.id;
	ui->lblID->setText(myID);
	ui->lblApart->setText(info.apart);
	ui->lblName->setText(info.name);
	if (myLevel == 0)
		ui->lblLevel->setText("Admin");
	else if (myLevel == 1)
		ui->lblLevel->setText("User");
	else if (myLevel == 2)
		ui->lblLevel->setText("visitor");


}
