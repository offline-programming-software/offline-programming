#pragma once

#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>

#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_native_types, raw_interfaces_only

// 批量轨迹点输出对话框：遍历所有喷涂机器人的所有路径组/路径，
// 按CATIA APT格式（GOTO / X,Y,Z,I,J,K）逐路径导出全部轨迹点
class num_export_end : public QDialog
{
	Q_OBJECT

public:
	num_export_end(QWidget* parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~num_export_end();

private:
	// 界面控件
	QLabel* pathLabel;
	QPushButton* browseBtn;
	QPushButton* exportBtn;
	QPlainTextEdit* logEdit;

	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	// 保存路径
	QString savePath;

	// APT轨迹点：笛卡尔坐标 + 刀轴方向单位矢量
	struct AptPoint {
		double x, y, z;
		double i, j, k;
		double velocity;
	};

private:
	void initUI();

	// 生成APT文件头（$$注释 + PARTNO + 换刀工序，到LOADTL/1,1,1为止）
	QStringList buildAptHeader(const QString& partName);

	// 向内容追加一个轨迹工序块（工序注释 + FEDRAT/SPINDL + 该路径全部GOTO点）
	void appendAptOperation(QStringList& lines, const std::vector<AptPoint>& points,
		const QString& operationName);

	// 采集单条路径的全部轨迹点（含相邻点距离检查与5mm插补）
	bool collectPathPoints(ULONG pathID, std::vector<AptPoint>& points);

	// 相邻点距离检查：超过5mm时按5mm步长线性插补（位置与刀轴矢量同步插值）
	void interpolatePoints(std::vector<AptPoint>& points);

	// 枚举辅助（与 export_end 同源）
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);
	QStringList getPathGroupNames(ULONG robotID);
	QStringList getPathNames(ULONG robotID, const QString& groupName);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG& o_uID);

private slots:
	void onSelectSavePath();
	void onExportAll();
};
