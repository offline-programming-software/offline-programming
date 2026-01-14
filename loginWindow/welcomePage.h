#pragma  once
#pragma execution_character_set("utf-8")
#include "ui_welcomePage.h"
#include "QWidget"
#include<QListWidgetItem>
#include<QSettings>
#include<QDir>

class MRU;
struct Details;
QT_BEGIN_NAMESPACE
namespace Ui { class welcomePageClass; };
QT_END_NAMESPACE

class MainWindow;

class WelcomePage: public QMainWindow
{
	Q_OBJECT

public:
	WelcomePage(QString ID, MainWindow* w, QWidget* parent = nullptr);
	~WelcomePage();
	
private:
	Ui::welcomePageClass *ui;
	
	QVector<QListWidgetItem*> recentWorkItems;
	QVector<QListWidgetItem*> recentDirItems;
	QStringList recentWorks;
	QStringList recentDirs;
	MainWindow* paint;

private:
	MRU* mruDir;
	MRU* mruWork;
	MRU* mruWorkPath;
	void connectSet();
	void openWork(const QString& filePath);
	void initMenu();
	void setThumbNails();
	QMenuBar* menuBar;
	QToolBar* toolBar;
	QVector<Details> currentDir_detailList;
	QString m_UserID;

private slots:
	void on_btnPainting_clicked();
	void on_btnOpen_clicked();
	void on_listRecentWrok_itemDoubleClicked(QListWidgetItem* item);
	void on_listRecentWrok_itemClicked(QListWidgetItem* item);
	void on_cmbRecentDir_currentIndexChanged(const QString& dir);

	//菜单栏
	void on_actionNew_triggered();

};


class MRU
{
public:
	MRU(const QString& key, int maxItem = 10)
		:m_key(key),
		m_maxItem(maxItem) {}

	// 加载 MRU 列表
	QStringList loadRecent() {
		QSettings settings(
			"PaintingRobot",  // 组织名称
			"PaintingRobot"   // 应用名称
		);
		return settings.value(m_key, QStringList()).toStringList();
	}
	// 保存 MRU 列表
	void saveRecent(const QStringList& list) {
		QSettings settings(
			"PaintingRobot",  // 组织名称
			"PaintingRobot"   // 应用名称
		);
		settings.setValue(m_key, list);
		settings.sync();  // 立即同步到磁盘
	}

	// 将文件添加到 MRU 列表
	void addToRecent(const QString& file) {
		QStringList recent = loadRecent();

		// 如果文件已经存在，则移除它并将它放到最前面
		recent.removeAll(file);
		recent.prepend(file);

		// 如果列表超过最大数量，移除最后的项
		if (recent.size() > m_maxItem) {
			recent.removeLast();
		}

		saveRecent(recent);
	}

	//清空mru列表
	void clearRecent() {
		QSettings settings(
			"PaintingRobot",  // 组织名称
			"PaintingRobot"   // 应用名称
		);
		settings.remove(m_key);
		settings.sync();  // 立即同步到磁盘
	}

private:
	int m_maxItem;
	QString m_key;

};


struct Details
{
	bool isPainting;
	QString name;
	QString filePath;
	QString creationTime;
	QString modificationTime;
};


