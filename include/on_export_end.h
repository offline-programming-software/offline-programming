#pragma once

#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QComboBox>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QPushButton>

#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

// 单个轨迹点输出对话框：选择 机器人->路径组->路径->点序号，输出该点的坐标与位姿
class export_end : public QDialog
{
	Q_OBJECT

public:
	export_end(QWidget* parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~export_end();

private:
	// 界面控件
	QComboBox* robotCombo;
	QComboBox* groupCombo;
	QComboBox* pathCombo;
	QSpinBox* pointSpin;
	QPlainTextEdit* resultEdit;
	QPushButton* outputBtn;
	QPushButton* saveBtn;

	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	// 机器人ID -> 名称
	QMap<ULONG, QString> m_robotMap;

private:
	void initUI();
	void loadRobots();

	QString currentRobotName();
	QString currentGroupName();
	QString currentPathName();

	// 枚举辅助（与 effectiveness_analysis 同源）
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);
	QStringList getPathGroupNames(ULONG robotID);
	QStringList getPathNames(ULONG robotID, const QString& groupName);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG& o_uID);

private slots:
	void onRobotChanged();
	void onGroupChanged();
	void onPathChanged();
	void onOutput();
	void onSaveToFile();
};
