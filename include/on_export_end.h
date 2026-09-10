#pragma once

#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

// 轨迹点输出对话框：选择 机器人->路径组->路径，输出该路径上所有点的坐标与位姿
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
	QLabel* pointCountLabel;
	QPlainTextEdit* resultEdit;
	QPushButton* outputBtn;
	QPushButton* saveBtn;

	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	// 机器人ID -> 名称
	QMap<ULONG, QString> m_robotMap;

	// APT轨迹点：笛卡尔坐标 + 刀轴方向单位矢量
	struct AptPoint {
		double x, y, z;
		double i, j, k;
		double velocity;
	};

	// 缓存最近一次输出的轨迹点
	std::vector<AptPoint> m_lastPoints;

	// 相邻点距离检查：超过5mm时按5mm步长线性插补（位置与刀轴矢量同步插值）
	void interpolatePoints(std::vector<AptPoint>& points);

	// 按CATIA APT格式（GOTO / X,Y,Z,I,J,K）生成完整轨迹文件文本
	QString buildAptContent(const QString& partName, const QString& operationName);

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
