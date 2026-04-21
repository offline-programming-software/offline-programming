#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QHeaderView>
#include "robxFileIO.h"
#include "ui_path_sort.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class path_sortClass; };
QT_END_NAMESPACE

class path_sort : public QDialog
{
	Q_OBJECT

public:
	path_sort(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~path_sort();

private:
	Ui::path_sortClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	QMap<ULONG, QString> m_robotMap;

	RobxIO* m_io;

	QVector<std::tuple<QString, QString, QString>> m_list;

private:
	void init();
	void initTable();
	void updateTable();
	bool isRelationExists(const QString& robotName, const QString& guideName, const QString& agvName);

	// 根据类型获取对象列表
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);

	// 从VARIANT中提取字符串数组
	QStringList extractStringArrayFromVariant(const VARIANT& variant);

	// 从VARIANT中提取长整型数组
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);

	//获取机器人列表
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);

	//获取id
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);

private slots:
	void onConfirm();
	void onCancel();
	void onAdd();
	void onDelete();

};