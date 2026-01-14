#include "welcomePage.h"
#include"MainWindow.h"
#include <QFileDialog>
#include<QListView>
#include "newFileDlg.h"
#include "UserManageDlg.h"
#include "About.h"
#include "myProfile.h"

WelcomePage::WelcomePage(QString ID, MainWindow* w, QWidget *parent)
	: QMainWindow(parent),
	m_UserID(ID),
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
	QListWidget* list = ui->listRecentWrok;

	// 大图标模式
	list->setViewMode(QListView::IconMode);

	// 图标大小
	list->setIconSize(QSize(96, 96));

	// 每个 item 的方形网格大小（一定要 ≥ iconSize）
	list->setGridSize(QSize(128, 150));

	// 左→右排列，自动换行
	list->setFlow(QListView::LeftToRight);
	list->setWrapping(true);
	list->setResizeMode(QListView::Adjust);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	list->setWordWrap(true);



	connectSet();
	//配置文件中读取最近工程和目录
	mruDir = new MRU("RecentDir", 10);
	mruWork = new MRU("RecentWork", 10);
	recentWorks = mruWork->loadRecent();
	recentDirs = mruDir->loadRecent();
	//初始化最近
	ui->cmbRecentDir->addItems(recentDirs);

	

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


	initMenu();
	
	
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

void WelcomePage::initMenu()
{
	menuBar = QMainWindow::menuBar();
	toolBar = QMainWindow::addToolBar("Main Toolbar");
	this->setMenuBar(menuBar);

	//初始化菜单栏，目前有：文件、设备、用户、帮助
	QMenu* menuFile = menuBar->addMenu("文件(&F)");
	QMenu* menuDevice = menuBar->addMenu("设备(&D)");
	QMenu* menuUser = menuBar->addMenu("用户(&U)");
	QMenu* menuHelp = menuBar->addMenu("帮助(&H)");

	//文件菜单
	QAction* actionOpen = menuFile->addAction(QIcon(":/welcomePage/resource/welcomePage/open.png"),"打开工程(&O)");
	
	QAction* actionNew = menuFile->addAction(QIcon(":/welcomePage/resource/welcomePage/new.png"), "新建工程(&N)");
	menuFile->addSeparator();
	QAction* actionExit = menuFile->addAction(QIcon(":/welcomePage/resource/welcomePage/exit.png"), "退出(&X)");

	//设备菜单
	QAction* actionDevice = menuDevice->addAction("设备信息");


	//用户菜单
	QAction* actionChangeUser = menuUser->addAction("切换账号");
	menuUser->addSeparator();
	QAction* actionMyProfile = menuUser->addAction("我的资料");
	QAction* actionChangePWD = menuUser->addAction(QIcon(":/welcomePage/resource/welcomePage/changePWD.png"), "修改密码");
	menuUser->addSeparator();
	QAction* actionUserManage = menuUser->addAction(QIcon(":/welcomePage/resource/welcomePage/myProfile.png"), "用户管理(&M)");
	menuUser->addSeparator();
	QAction* actionLog = menuUser->addAction(QIcon(":/welcomePage/resource/welcomePage/log.png"), "操作日志");


	//帮助菜单
	QAction* actionAbout = menuHelp->addAction(QIcon(":/welcomePage/resource/welcomePage/about.png"), "关于");
	menuHelp->addSeparator();
	QAction* actionHelpDoc = menuHelp->addAction( "操作说明文档(&f1)");
	QAction* actionDevDoc = menuHelp->addAction("开发者文档");


	

	toolBar->addAction(actionOpen);
	toolBar->addAction(actionNew);
	toolBar->addAction(actionExit);

	toolBar->addSeparator();

	toolBar->addAction(actionUserManage);

	toolBar->addSeparator();

	toolBar->addAction(actionAbout);


	connect(actionOpen, &QAction::triggered, this, &WelcomePage::on_btnOpen_clicked);
	connect(actionNew, &QAction::triggered, this, &WelcomePage::on_btnPainting_clicked);
	connect(actionExit, &QAction::triggered, this, &QWidget::close);
	connect(actionMyProfile, &QAction::triggered, this, [this]() 
		{
			myProfile pfl(m_UserID);
			pfl.setWindowTitle(u8"当前用户信息");
			pfl.exec();
		}
	);
	connect(actionUserManage, &QAction::triggered, this, []() {
		UserManageDlg dlg;
		dlg.setWindowTitle("用户管理");
		dlg.exec();  //在dlg里面处理保存读取
		});
	menuFile->addAction(actionNew);
	connect(actionAbout, &QAction::triggered, this, []() {
		About dlg;
		dlg.setWindowTitle("关于");
		dlg.exec();
		});
	menuFile->addAction(actionOpen);
	
}



void WelcomePage::setThumbNails()
{
	// root/thumbNail文件夹有若干projName.png文件，检查listRecentWrok上的item文件
	// 同名的（忽略后缀）则把图片文件附给item

	QString thumbNailDir = QDir::currentPath() + "/thumbNail";
	QDir directory(thumbNailDir);

	// 检查thumbNail文件夹是否存在
	if (!directory.exists()) {
		return;
	}

	// 获取thumbNail文件夹中的所有.png文件
	QStringList filters;
	filters << "*.png";
	QFileInfoList thumbNailFiles = directory.entryInfoList(filters, QDir::Files);

	// 为每个item查找对应的缩略图
	for (int i = 0; i < ui->listRecentWrok->count(); ++i) {
		QListWidgetItem* item = ui->listRecentWrok->item(i);
		QString itemName = item->text(); // 获取item的文本（不带后缀的文件名）

		// 在thumbNail文件夹中查找同名的png文件
		for (const QFileInfo& fileInfo : thumbNailFiles) {
			QString thumbNailName = fileInfo.completeBaseName(); // 获取不带后缀的文件名

			if (thumbNailName == itemName) {
				// 找到匹配的缩略图，设置给item
				QPixmap pixmap(fileInfo.absoluteFilePath());
				if (!pixmap.isNull()) {
					// 按照listRecentWrok的iconSize缩放图片
					QPixmap scaledPixmap = pixmap.scaledToWidth(
						ui->listRecentWrok->iconSize().width(),
						Qt::SmoothTransformation
					);
					QIcon icon(scaledPixmap);
					item->setIcon(icon);
				}
				break;
			}
		}
	}
}

void WelcomePage::on_btnPainting_clicked()
{
	//
	newFileDlg dlg(this);
	dlg.setWindowTitle("新建喷涂工程");
	dlg.exec();

	if(dlg.result() == QDialog::Rejected) {
		return;
	}

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
//主控件，最近工程列表事件
void WelcomePage::on_listRecentWrok_itemDoubleClicked(QListWidgetItem* item)
{
	//点击item之后，拼接出对应路径
	bool isPainting = item->data(Qt::UserRole + 1).toBool();
	QString dir = ui->cmbRecentDir->currentText();
	QString fileName = item->text();
	QString filePath = dir + "/" + fileName;
	if(isPainting) {
		filePath += ".robx";
	}
	else {
		filePath += ".gnd";
	}
	openWork(filePath);
}

void WelcomePage::on_listRecentWrok_itemClicked(QListWidgetItem* item)
{
	//单击某个工程文件，右侧grpDetail显示工程详细信息
	//包括：文件名、创建时间、修改时间、创建者、描述
	//grpDetail中有一个空的loutDetail布局，用于动态添加标签显示信息
	int row = ui->listRecentWrok->row(item);
	Details detail = currentDir_detailList.at(row);
	//清空布局
	QLayoutItem* child;
	while ((child = ui->loutDetail->takeAt(0)) != nullptr) 
	{
		delete child->widget();
		delete child;
	}
	//添加标签显示信息
	QLabel* typeLabel = new QLabel("工程类型: " + QString(detail.isPainting ? "喷涂工程" : "打磨工程"));
	QLabel* nameLabel = new QLabel("文件名: " + detail.name);
	QLabel* creationTimeLabel = new QLabel("创建时间: " + detail.creationTime);
	QLabel* modificationTimeLabel = new QLabel("修改时间: " + detail.modificationTime);
	ui->loutDetail->addWidget(typeLabel);
	ui->loutDetail->addWidget(nameLabel);
	ui->loutDetail->addWidget(creationTimeLabel);
	ui->loutDetail->addWidget(modificationTimeLabel);
	


	
}


void WelcomePage::on_cmbRecentDir_currentIndexChanged(const QString& dir)
{
	//搜索目录下的.robx和.gnd文件，更新最近工程列表
	ui->listRecentWrok->clear();
	QDir directory(dir);
	QStringList filters;
	filters << "*.robx" << "*.gnd";
	QFileInfoList fileList = directory.entryInfoList(filters, QDir::Files);
	ui->listRecentWrok->clear();
	recentWorkItems.clear();
	for (const QFileInfo& fileInfo : fileList) 
	{
		QListWidgetItem* item = new QListWidgetItem(ui->listRecentWrok);
		item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
		recentWorkItems.append(item);
        item->setText(fileInfo.completeBaseName()); //设置不带后缀的文件名
		//判断如果是喷涂文件, 设置喷涂图标，否则设置打磨图标
		if (fileInfo.suffix() == "robx")
		{
			item->setIcon(QIcon(":/welcomePage/resource/welcomePage/PaintingProj.png"));
			item->setData(Qt::UserRole + 1, true);
		}
		else if (fileInfo.suffix() == "gnd")
		{
			item->setIcon(QIcon(":/welcomePage/resource/welcomePage/Grinding.png"));
			item->setData(Qt::UserRole + 1, false);
		}
		else
		recentWorks.append(fileInfo.absoluteFilePath());
	}


	//初始化currentDir_detailList
	currentDir_detailList.clear();
	for(const QFileInfo& fileInfo : fileList) 
	{
		Details detail;
		detail.isPainting = (fileInfo.suffix() == "robx");
		detail.name = fileInfo.fileName();
		detail.filePath = fileInfo.absoluteFilePath();
		detail.creationTime = fileInfo.created().toString("yyyy-MM-dd hh:mm:ss");
		detail.modificationTime = fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
		//detail.creator = ... //需要从文件中读取创建者信息
		//detail.description = ... //需要从文件中读取描述信息
		currentDir_detailList.append(detail);
	}

	setThumbNails();
}

void WelcomePage::on_actionNew_triggered()
{
	newFileDlg dlg(this);
	dlg.setWindowTitle("新建喷涂工程");
	dlg.exec();
	if(dlg.result() == QDialog::Rejected) {
		return;
	}
	QString name = dlg.name();
	QString creator = dlg.creator();
	QString description = dlg.description();
	bool isPainting = dlg.isPainting();
	if(isPainting) {
		this->hide();
		paint->show();
		paint->showMaximized();
		paint->setWindowTitle("自动喷涂离线编程与仿真软件");
		//window关闭时，关闭welcomePage，主程序结束
		connect(paint, &MainWindow::destroyed, this, &WelcomePage::close);
	}
	else
	{
		//新建一个打磨工程
	}
}
