#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QStandardItemModel>
#include "ui_robotSpaceDefine.h"
#include "addRobotSpace.h"
#include <QDebug>
#include <QMessageBox>
#include <QHeaderView>

#include "PQKitCallback.h"
#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

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

	void addAxisInfo(int number, const QString& axisName, const QString& mainNormalVector,
		bool hasGuideRail, const QString guideName);

	void setRobotOptions(const QString& robotOption);//设置机器人名称


	QString getRobotName();//获取机器人名称

private slots:
	void onAddAxis();  // 添加坐标轴槽函数
	void onDeleteAxis(); // 删除坐标轴槽函数

private:
	void setupTableView(); // 初始化表格视图
	void updateTableView(); // 更新表格显示

	QMap<ULONG, QString> getObjectsByType(PQDataType objType);
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);

private:
	Ui::robotSpaceDefineClass *ui;
	QStandardItemModel *axisModel; // 表格数据模型
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	
	// 存储坐标轴数据
	struct AxisData {
		int number;
		QString axisName;
		QString mainNormalVector;
		bool hasGuideRail;
		QString guideName;
	};

	QList<AxisData> axisList;

};
