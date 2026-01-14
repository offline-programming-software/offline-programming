#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QMap>
#include <QString>
#include "ui_UserManageDlg.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserManageDlgClass; };
QT_END_NAMESPACE

enum class level
{
	Admin,
	User,
	Visitor
};

struct UserInfo {
	QString id;
	QString pwd;
	QString name;
	QString apart;
	QString contact;
	QString workID;
	level userLevel;
	
};

/**
 * @brief 管理登录账号信息的对话框，
 * 
 * @detials 
 * 可以添加、删除、修改用户信息,主要的控件有左侧的listUsers和右侧的grpEditArea组成
 * 操作说明：
 * 点击btnNew，新建用户，右侧编辑区清空，输入右侧所有属性，点击btnEditOK保存
 * 点击btnDelete，删除选中的用户
 * 点击右下角buttonBox的OK，保存所有修改并关闭对话框，点击Cancel，放弃所有修改并关闭对话框
 * 保存位置：
 * 通过QSetting类保存为配置文件
 */
class UserManageDlg : public QDialog
{
	Q_OBJECT

public:
	UserManageDlg(QWidget *parent = nullptr);
	~UserManageDlg();

private slots:
	void onBtnNewClicked();
	void onBtnDeleteClicked();
	void onBtnEditOKClicked();
	void onListUsersSelectionChanged();
	void onOKClicked();
	void onCancelClicked();

private:
	void loadUsersFromSettings();
	void saveUsersToSettings();
	void clearEditFields();
	void updateUserListDisplay();

	Ui::UserManageDlgClass *ui;
	QMap<QString, UserInfo> m_users;
	QString m_currentSelectedUser;
	bool m_isEditingNew;
};

