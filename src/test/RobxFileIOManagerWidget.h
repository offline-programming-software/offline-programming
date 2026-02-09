#pragma once

#include <QWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include "ui_RobxFileIOManagerWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RobxFileIOManagerWidgetClass; };
QT_END_NAMESPACE

/**
 * @class RobxFileIOManagerWidget.
 * 
 * @brief robx文件读写管理界面
 * 
 * 用于测试robx文件的读写功能：核心控件是一个TreeWidget，显示robx文件内的json文件列表，支持查看、temp文件夹的内容
 * 具体功能：
 * 1. 读取当前temp文件夹内的所有文件列表，显示在TreeWidget中
 * 2. 点击某个json文件，右侧的文本编辑器显示该json文件的内容
 */
class RobxFileIOManagerWidget : public QWidget
{
	Q_OBJECT

public:
	RobxFileIOManagerWidget(QWidget *parent = nullptr);
	~RobxFileIOManagerWidget();

private:
	Ui::RobxFileIOManagerWidgetClass *ui;
	QMap<QTreeWidgetItem*, QString> m_itemPathMap; // Map item to full file path

	// Private helper methods
	void loadTempFolder();
	void populateTreeWidget(QTreeWidgetItem* parentItem, const QString& folderPath);
	QString readFileContent(const QString& filePath);
	QTreeWidgetItem* findItemByPath(const QString& path);

private slots:
	void onTreeItemSelectionChanged();
	void onDeleteButtonClicked();
	void onRefreshButtonClicked();
};

