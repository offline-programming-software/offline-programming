#ifndef ROBOTSPACEDEFINE_H
#define ROBOTSPACEDEFINE_H
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QStandardItemModel>
#include "ui_robotSpaceDefine.h"
#include "addRobotSpace.h"
#include <QDebug>
#include <QMessageBox>
#include <QHeaderView>
#include <QString.h>
#include <QComboBox>
#include "robxFileIO.h"
#include "spaceCalculate.h"
#include "robxFileIO.h"
#include "PQKitCallback.h"
#include <qvector.h>
#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

struct workSpace;
struct workSpaceInformation;

QT_BEGIN_NAMESPACE
namespace Ui { class robotSpaceDefineClass; };
QT_END_NAMESPACE

class robotSpaceDefine : public QDialog
{
	Q_OBJECT

public:
	robotSpaceDefine(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);

	~robotSpaceDefine();

	void addAxisInfo(int number, const QString& axisName, const QString& mainNormalVector);

	QString getRobotName();//获取机器人名称

private slots:
	void onAddAxis();  // 添加坐标轴槽函数
	void onDeleteAxis(); // 删除坐标轴槽函数
	void onConfirm();
	void onClose();
	void onRobotChanged(int index); // 新增：机器人切换槽函数

private:
	void setupTableView(); // 初始化表格视图
	void updateTableView(); // 更新表格显示
	void loadRobotData(const QString& robotName); // 新增：加载特定机器人的数据

	//获取主法矢量
	std::vector<double>getDir(ULONG coordinateID, QString mainDir);
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);


	//获取机器人列表
	QStringList getSpraydRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);

	// 获取轨迹组名称
	QStringList getPathGroupNames(ULONG robotID);

	// 获取轨迹名称
	QStringList getPathNames(ULONG robotID, const QString& groupName);


private:
	Ui::robotSpaceDefineClass *ui;
	QStandardItemModel *axisModel; // 表格数据模型
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	RobxIO *m_io;
	//QVector<RobotWorkspaceBoundary> m_list;
	QMap<QString, QVector<RobotWorkspaceBoundary>> m_list;
	// 修改：使用 QMap 按机器人名称存储工作空间信息
	QMap<QString, QVector<workSpaceInformation>> m_spaceInformation;
	

	// 存储坐标轴数据
	struct AxisData {
		int number;
		QString axisName;
		QString mainNormalVector;
	};

	QList<AxisData> axisList;

};
#endif