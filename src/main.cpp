#include "MainWindow.h"
#include "loginwindow.h"
#include <QTextCodec>
#include <QtWidgets/QApplication>
#include <QString>
#include "welcomePage.h"
#include"loginDlg.h"


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QFont f = a.font();
	f.setFamily(u8"微软雅黑");
	a.setFont(f);

	loginDlg login;
	login.show();
	if(login.exec() != QDialog::Accepted) {
		return 0; // 如果登录未成功，退出应用程序
	}
	MainWindow* painting = new MainWindow();
	WelcomePage welcome(painting);
	welcome.show();

	return a.exec();
}
