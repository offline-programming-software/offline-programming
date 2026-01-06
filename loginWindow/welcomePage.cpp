#include "welcomePage.h"
#include"MainWindow.h"
#include <QFileDialog>
#include<QListView>
#include "newFileDlg.h"
#include "UserManageDlg.h"

WelcomePage::WelcomePage(MainWindow* w, QWidget *parent)
	: QMainWindow(parent),
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


	//初始化菜单栏
	menuBar = QMainWindow::menuBar();
	this->setMenuBar(menuBar);
	QMenu* menuFile = menuBar->addMenu("文件(&F)");
	QMenu* menuDevice = menuBar->addMenu("设备(&D)");
	QMenu* menuUser = menuBar->addMenu("用户(&U)");
	QMenu* menuHelp = menuBar->addMenu("帮助(&H)");
	QAction* actionOpen = menuFile->addAction("打开工程(&O)");
	QAction* actionNew = menuFile->addAction("新建工程(&N)");
	QAction* actionUserManage = menuUser->addAction("用户管理(&M)");
	connect(actionOpen, &QAction::triggered, this, &WelcomePage::on_btnOpen_clicked);
	connect(actionNew, &QAction::triggered, this, &WelcomePage::on_btnPainting_clicked);
	connect(actionUserManage, &QAction::triggered, []() {
		UserManageDlg dlg;
		dlg.setWindowTitle("用户管理");
		dlg.exec();  //在dlg里面处理保存读取
		});
	menuFile->addAction(actionNew);
	menuFile->addAction(actionOpen);
	
	
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
	//初始化菜单栏，目前有：文件、设备、用户、帮助
	
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
