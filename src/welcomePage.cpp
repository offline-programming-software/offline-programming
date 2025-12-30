#include "welcomePage.h"
#include"MainWindow.h"
#include <QFileDialog>
#include<QListView>


WelcomePage::WelcomePage(MainWindow* w, QWidget *parent)
	: QWidget(parent),
	paint(w),
	ui(new Ui::welcomePageClass())
{
	//初始化UI
	ui->setupUi(this);
	ui->btnGrinding->setIcon(QIcon(":/welcomePage/resource/welcomePage/grinding.png"));
	ui->btnGrinding->setIconSize(QSize(48, 48));
	ui->btnPainting->setIcon(QIcon(":/welcomePage/resource/welcomePage/painting.png"));
	ui->btnPainting->setIconSize(QSize(48, 48));
	this->setWindowTitle("welcome");
	ui->listRecentWrok->setIconSize(QSize(24, 24));



	connectSet();
	
	//配置文件中读取最近工程和目录
	mruDir = new MRU("RecentDir", 10);
	mruWork = new MRU("RecentWork", 10);
	recentWorks = mruWork->loadRecent();
	recentDirs = mruDir->loadRecent();
	//初始化最近
	for(const QString& work : recentWorks)
	{
		QListWidgetItem* item = new QListWidgetItem(ui->listRecentWrok);
		recentWorkItems.append(item);
		item->setText(QFileInfo(work).fileName());
	}
	for (const QString& dir : recentDirs)
	{
		QListWidgetItem* item = new QListWidgetItem(ui->listRecentDir);
		recentDirItems.append(item);
		item->setText(dir);
	}

	//如果没有最近工程或目录，显示提示信息和图片
	if (recentWorkItems.isEmpty()) 
	{
		ui->listRecentWrok->hide();
		QLabel* noRecentWorkLabel = new QLabel("无最近工程");
		QLabel* image = new QLabel;
		noRecentWorkLabel->setStyleSheet("font-size:16px; color:#555555;");
		QPixmap px(":/welcomePage/resource/welcomePage/emptyProj.png");
		image->setPixmap(px.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		image->setAlignment(Qt::AlignCenter);
		noRecentWorkLabel->setAlignment(Qt::AlignCenter);
		ui->layoutRecentWork->addWidget(image);
		ui->layoutRecentWork->addWidget(noRecentWorkLabel);
		QSize size = noRecentWorkLabel->size();
	}
	if (recentDirItems.isEmpty()) 
	{
		ui->listRecentDir->hide();
		QLabel* image = new QLabel;
		QPixmap px(":/welcomePage/resource/welcomePage/emptyDir.png");
		image->setPixmap(px.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		image->setAlignment(Qt::AlignCenter);
		ui->layoutRecentDir->addWidget(image);
	}

}

WelcomePage::~WelcomePage()
{

}


void WelcomePage::connectSet()
{
	connect(ui->btnPainting, &QPushButton::clicked, this, &WelcomePage::on_btnPainting_clicked);
}


void WelcomePage::openWork(const QString& filePath)
{
	QString qFilePath = filePath;
	QString fileDir = QFileInfo(qFilePath).absolutePath();
	//判断是打磨文件还是喷涂文件
	if (qFilePath.endsWith(".robx") == true)
	{
		this->hide();
		paint->openRobxFile(qFilePath);
		paint->show();
		paint->showMaximized();
		paint->setWindowTitle("自动喷涂离线编程与仿真软件");
		// ? 当主窗口关闭时，关闭欢迎页，程序结束
		connect(paint, &MainWindow::destroyed, this, &WelcomePage::close);
		mruDir->addToRecent(fileDir);
		mruWork->addToRecent(filePath);
	}
	else if (qFilePath.endsWith(".gnd") == true)
	{
		//打开打磨文件
	}
	else
	{
		//文件格式不正确，弹出提示
	}
}

void WelcomePage::on_btnGrinding_clicked()
{

}

void WelcomePage::on_btnPainting_clicked()
{
	this->hide();
	paint->show();
	paint->showMaximized();
	paint->setWindowTitle("自动喷涂离线编程与仿真软件");
	//window关闭时，关闭welcomePage，主程序结束
	connect(paint, &MainWindow::destroyed, this, &WelcomePage::close);
}

void WelcomePage::on_btnOpen_clicked()
{
	std::wstring filePath;
	//打开文件选择对话框，把选择的路径传给filePath, 取消则return返回
	QString qFilePath = QFileDialog::getOpenFileName(
			this, 
		"打开工程文件",
		QDir::homePath(),
		"Painting Projects(*.robx);;Grinding Projects(*.gnd)"
	);

	if(qFilePath.isEmpty()) {
		return;
	}
	//获取文件名和目录路径
	//QString fileName = QFileInfo(qFilePath).fileName();
	QString fileName = qFilePath;
	QString dirPath = QFileInfo(qFilePath).absolutePath();

	//判断是打磨文件还是喷涂文件
	if(qFilePath.endsWith(".robx") == true)
	{
		this->hide();
		paint->openRobxFile(qFilePath);
		paint->show();
		paint->showMaximized();
		paint->setWindowTitle("自动喷涂离线编程与仿真软件");
		// ? 当主窗口关闭时，关闭欢迎页，程序结束
		connect(paint, &MainWindow::destroyed, this, &WelcomePage::close);
		mruDir->addToRecent(dirPath);
		mruWork->addToRecent(fileName);
	}
	else if(qFilePath.endsWith(".gnd") == true) 
	{
		//打开打磨文件
	}
	else
	{
		//文件格式不正确，弹出提示
	}
}

void WelcomePage::on_listRecentWrok_itemClicked(QListWidgetItem* item)
{
	//点击item之后，在QStringList recentWorks中找到对应路径，调用openWork打开工程
	int row = ui->listRecentWrok->row(item);
	if (row >= 0 && row < recentWorks.size()) 
	{
		QString filePath = recentWorks.at(row);
		openWork(filePath);
	}

}

void WelcomePage::on_listRecentDir_itemClicked(QListWidgetItem* item)
{
	//点击item之后，打开QT文件选择对话框，初始目录为该item路径
	int row = ui->listRecentDir->row(item);
	if (row >= 0 && row < recentDirs.size())
	{
		QString dirPath = recentDirs.at(row);
		QString qFilePath = QFileDialog::getOpenFileName(
			this,
			"打开工程文件",
			dirPath,
			"Painting Projects(*.robx);;Grinding Projects(*.gnd)"
		);
		if (qFilePath.isEmpty()) {
			return;
		}
		//获取文件名和目录路径
		//QString fileName = QFileInfo(qFilePath).fileName();
		QString fileName = qFilePath;
		QString dirPathNew = QFileInfo(qFilePath).absolutePath();
		//判断是打磨文件还是喷涂文件
		if (qFilePath.endsWith(".robx") == true)
		{
			this->hide();
			paint->openRobxFile(qFilePath);
			paint->show();
			paint->showMaximized();
			paint->setWindowTitle("自动喷涂离线编程与仿真软件");
			// ? 当主窗口关闭时，关闭欢迎页，程序结束
			connect(paint, &MainWindow::destroyed, this, &WelcomePage::close);
			mruDir->addToRecent(dirPathNew);
			mruWork->addToRecent(fileName);
		}
		else if (qFilePath.endsWith(".gnd") == true)
		{
			//打开打磨文件
		}
		else
		{
			//文件格式不正确，弹出提示
		}
	}
}
