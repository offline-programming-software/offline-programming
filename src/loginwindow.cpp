#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include <QString>

loginwindow::loginwindow(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::loginwindowClass())
{
	ui->setupUi(this);

	// 连接登录按钮
	connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(onLoginClicked()));
}

loginwindow::~loginwindow()
{
	delete ui;
}

void loginwindow::onLoginClicked()
{
	QString username = ui->lineEdit->text();
	QString password = ui->lineEdit_2->text();

	if (username.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("用户名不能为空！"));
		return;
	}

	if (password.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("密码不能为空！"));
		return;
	}

	// 简单验证用户名和密码
	if (username == "admin" && password == "12345") {
		emit loginSuccess(); // 触发登录成功信号
	}
	else {
		QMessageBox::warning(this, QString::fromLocal8Bit("登录失败"), QString::fromLocal8Bit("用户名或密码错误！"));
	}
}

void loginwindow::closeEvent(QCloseEvent* event)
{
	QApplication::quit(); // 在登录窗口关闭时退出应用程序
	QWidget::closeEvent(event); // 调用基类的关闭事件处理
}
