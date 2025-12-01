#include "MainWindow.h"
#include "loginwindow.h"
#include <QTextCodec>
#include <QtWidgets/QApplication>
#include <QString>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QFont f = a.font();
	f.setFamily(u8"微软雅黑");
	a.setFont(f);

	MainWindow w;
		
	w.show();  // 显示主窗口
	w.showMaximized();
	w.setWindowTitle("自动喷涂离线编程与仿真软件");
	return a.exec();
}
