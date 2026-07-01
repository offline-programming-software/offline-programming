#pragma once

#include <QWidget>
#include "ui_loginwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class loginwindowClass; };
QT_END_NAMESPACE

class loginwindow : public QWidget
{
	Q_OBJECT

public:
	loginwindow(QWidget *parent = nullptr);
	~loginwindow();

signals:
	void loginSuccess(); // 信号：表示登录成功

protected:
	void closeEvent(QCloseEvent* event) override; // 重写关闭事件

private slots:
	void onLoginClicked(); // 槽：处理登录按钮点击

private:
	Ui::loginwindowClass *ui;
};
