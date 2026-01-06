#include "UserManageDlg.h"
#include <QSettings>
#include <QListWidgetItem>
#include <QMessageBox>

UserManageDlg::UserManageDlg(QWidget *parent)
	: QDialog(parent),
	ui(new Ui::UserManageDlgClass()),
	m_isEditingNew(false)
{
	ui->setupUi(this);
	this->setWindowTitle("用户管理");
	
	loadUsersFromSettings();
	updateUserListDisplay();
	clearEditFields();
	
	// Connect signals and slots
	connect(ui->btnNew, &QPushButton::clicked, this, &UserManageDlg::onBtnNewClicked);
	connect(ui->btnDelete, &QPushButton::clicked, this, &UserManageDlg::onBtnDeleteClicked);
	connect(ui->btnEditOK, &QPushButton::clicked, this, &UserManageDlg::onBtnEditOKClicked);
	connect(ui->listUsers, &QListWidget::itemSelectionChanged, this, &UserManageDlg::onListUsersSelectionChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &UserManageDlg::onOKClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &UserManageDlg::onCancelClicked);
}

UserManageDlg::~UserManageDlg()
{
	delete ui;
}

void UserManageDlg::onBtnNewClicked()
{
	m_isEditingNew = true;
	m_currentSelectedUser.clear();
	ui->listUsers->clearSelection();
	clearEditFields();
}

void UserManageDlg::onBtnDeleteClicked()
{
	QListWidgetItem *currentItem = ui->listUsers->currentItem();
	if (!currentItem) {
		QMessageBox::warning(this, "警告", "请先选择要删除的用户");
		return;
	}
	
	QString userID = currentItem->text();
	if (QMessageBox::question(this, "确认删除", 
		QString("确定要删除用户 %1 吗？").arg(userID)) == QMessageBox::Yes) {
		m_users.remove(userID);
		m_currentSelectedUser.clear();
		updateUserListDisplay();
		clearEditFields();
	}
}

void UserManageDlg::onBtnEditOKClicked()
{
	QString userID = ui->edtID->text().trimmed();
	if (userID.isEmpty()) {
		QMessageBox::warning(this, "警告", "登陆账号不能为空");
		return;
	}
	
	UserInfo info;
	info.id = userID;
	info.pwd = ui->edtPWD->text();
	info.name = ui->edtName->text();
	info.apart = ui->edtApart->text();
	info.contact = ui->edtContact->text();
	info.workID = ui->edtWorkID->text();
	
	m_users[userID] = info;
	m_isEditingNew = false;
	m_currentSelectedUser = userID;
	
	updateUserListDisplay();
	clearEditFields();
	
	// Select the edited/added user in the list
	for (int i = 0; i < ui->listUsers->count(); ++i) {
		if (ui->listUsers->item(i)->text() == userID) {
			ui->listUsers->setCurrentRow(i);
			break;
		}
	}
}

void UserManageDlg::onListUsersSelectionChanged()
{
	QListWidgetItem *currentItem = ui->listUsers->currentItem();
	if (!currentItem) {
		clearEditFields();
		m_currentSelectedUser.clear();
		return;
	}
	
	QString userID = currentItem->text();
	m_currentSelectedUser = userID;
	m_isEditingNew = false;
	
	if (m_users.contains(userID)) {
		const UserInfo &info = m_users[userID];
		ui->edtID->setText(info.id);
		ui->edtPWD->setText(info.pwd);
		ui->edtName->setText(info.name);
		ui->edtApart->setText(info.apart);
		ui->edtContact->setText(info.contact);
		ui->edtWorkID->setText(info.workID);
	}
}

void UserManageDlg::onOKClicked()
{
	saveUsersToSettings();
	accept();
}

void UserManageDlg::onCancelClicked()
{
	reject();
}

void UserManageDlg::loadUsersFromSettings()
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
		
		if (!info.id.isEmpty()) {
			m_users[info.id] = info;
		}
	}
	settings.endArray();
}

void UserManageDlg::saveUsersToSettings()
{
	QSettings settings("offline-programming", "robot2022");
	settings.remove("Users");
	
	settings.beginWriteArray("Users", m_users.size());
	int index = 0;
	for (auto it = m_users.begin(); it != m_users.end(); ++it, ++index) {
		settings.setArrayIndex(index);
		const UserInfo &info = it.value();
		settings.setValue("id", info.id);
		settings.setValue("pwd", info.pwd);
		settings.setValue("name", info.name);
		settings.setValue("apart", info.apart);
		settings.setValue("contact", info.contact);
		settings.setValue("workID", info.workID);
	}
	settings.endArray();
}

void UserManageDlg::clearEditFields()
{
	ui->edtID->clear();
	ui->edtPWD->clear();
	ui->edtName->clear();
	ui->edtApart->clear();
	ui->edtContact->clear();
	ui->edtWorkID->clear();
}

void UserManageDlg::updateUserListDisplay()
{
	ui->listUsers->clear();
	for (auto it = m_users.begin(); it != m_users.end(); ++it) {
		ui->listUsers->addItem(it.key());
	}
}
