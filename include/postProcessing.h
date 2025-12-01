#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include "ui_postProcessing.h"

QT_BEGIN_NAMESPACE
namespace Ui { class postProcessingClass; };
QT_END_NAMESPACE

class postProcessing : public QDialog
{
	Q_OBJECT

public:
	postProcessing(QWidget *parent = nullptr);
	~postProcessing();

	// 设置机器人名称
	void setRobotName(const QString &name) { robotName = name; };
	// 设置保存路径
	void setSavePath(const QString &path) { savePath = path; };

	// 添加父节点（机器人节点）
	int addParentNode(const QString &name, int id);
	// 添加子节点（组节点）到指定的父节点
	int addChildNode(int parentId, const QString &name, int childId, const QString &content);
	// 添加子子节点（位置节点）到指定的子节点
	int addSubChildNode(int parentChildId, const QString &name, int subChildId, const QString &content);
	// 添加子节点内容
	void addChildContent(int childId, const QString &content);
	// 清空所有节点
	void clearAllNodes();
	// 通过ID获取节点内容
	QString getNodeContent(int nodeId) const;
	// 通过ID获取节点名称
	QString getNodeName(int nodeId) const;

	// 设置路径组名称
	void setPathGroupName(const QString &name) { pathGroupName = name; };

	// 新增方法：获取机器人节点下的所有组信息
	QMap<int, QString> getRobotGroups(int robotNodeId) const;
	// 新增方法：生成整个机器人的后置文件
	bool generateRobotPostFiles(int robotNodeId);

signals:
	// 新增信号：节点选择信号
	void nodeSelected(int nodeId, const QString& nodeName, const QString& content);

private slots:
	// 修改生成按钮的槽函数
	void onGenerateRobotFilesClicked();
	void onSelectSavePathClicked();
	void onTreeItemClicked(QTreeWidgetItem *item, int column);

private:
	Ui::postProcessingClass *ui;
	QSplitter *mainSplitter;      // 主分割窗口
	QTreeWidget *treeWidget;      // 树控件
	QTextEdit *textEdit;          // 文本编辑器
	QString robotName;            // 机器人名称
	QString pathGroupName;        // 路径组名称
	QString savePath;             // 保存路径

	QPushButton *generateButton;
	QPushButton *selectPathButton; // 选择路径按钮

	// 存储节点ID与内容的映射
	QMap<int, QString> nodeIdToContentMap;
	// 存储节点ID与名称的映射
	QMap<int, QString> nodeIdToNameMap;
	// 存储父节点ID与项的映射
	QMap<int, QTreeWidgetItem*> parentIdToItemMap;
	// 存储子节点ID与项的映射
	QMap<int, QTreeWidgetItem*> childIdToItemMap;
	// 存储子子节点ID与项的映射
	QMap<int, QTreeWidgetItem*> subChildIdToItemMap;

	// 初始化UI
	void initUI();
	// 保存后置文件到指定目录结构
	bool savePostFile(const QString &trajectoryContent, const QString &positionContent);

	// 新增私有方法：为组生成轨迹内容
	QString generateTrajectoryContentForGroup(int groupNodeId);
	// 新增私有方法：为组生成位置内容
	QString generatePositionContentForGroup(int groupNodeId);
};