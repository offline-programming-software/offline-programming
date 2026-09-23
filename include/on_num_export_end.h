#pragma once

#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <vector>

#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_native_types, raw_interfaces_only

// 批量轨迹点输出对话框：遍历所有喷涂机器人的所有路径组/路径，
// 按图示格式（MOVJ起始关节角 + GOTO点列 + GUNT/GUNF + END）将全部轨迹合并输出到同一个文件
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
	QComboBox* coordCombo;
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
		// 喷涂事件：0=无，1=开枪(GUNT)，2=关枪(GUNF)
		int gunEvent = 0;
		// 事件输出时机：true=点前(该点GOTO之前)，false=点后(该点GOTO之后)
		bool gunBeforePoint = true;
	};

private:
	void initUI();
	void loadCoordinates();

	// 向内容追加一个轨迹块：首个块前写FEDRAT/SPINDL（文件级），
	// 块内为 MOVJ(起始关节角) + GOTO点列（带喷涂事件的点按点前/点后输出GUNT/GUNF）+ END + 结束注释
	void appendAptOperation(QStringList& lines, const std::vector<AptPoint>& points,
		const QString& operationName);

	// 采集单条路径的全部轨迹点（坐标系变换 + 相邻点距离检查与5mm插补）
	// targetCoordID为用户指定的输出坐标系ID，0=自动（路径关联）
	// externalMechId为该机器人挂载的AGV/导轨机构ID（用于MOVJ行），0=无
	bool collectPathPoints(ULONG pathID, ULONG targetCoordID, ULONG externalMechId,
		std::vector<AptPoint>& points);

	// 解析机器人挂载的外部轴机构（AGV小车/导轨）ID：
	// 1)命名约定<机器人名>_rail 2)relations.json连接关系 3)场景中唯一的导轨型机构；无则返回0
	ULONG resolveExternalMechId(const QString& robotName);

	// 读路径首点关节角（弧度转角度，Π取3.14）并追加AGV外部轴值（直线轴不换算），生成MOVJ行文本
	void buildMovjText(ULONG firstPointID, ULONG externalMechId);

	// 相邻点距离检查：2.5mm以内合并为一个点，超过5mm按5mm步长线性插补；
	// 插补后的点会再做一次合并检查
	void interpolatePoints(std::vector<AptPoint>& points);

	// 2.5mm以内相邻点合并为一个点（位置中点、刀轴平均归一化、速度取后点）
	void mergeClosePoints(std::vector<AptPoint>& points);

	// 按位姿数组[x,y,z,qw,qx,qy,qz]把点变换到该位姿定义的坐标系（P=R^T(P-t)，矢量只旋转）
	bool applyPostureTransform(std::vector<AptPoint>& points, const double* dPosture);

	// 将点变换到指定坐标系对象（Doc_get_coordinate_posture），失败返回false
	bool transformPointsToCoordinate(std::vector<AptPoint>& points, ULONG targetCoordID);

	// 最近一条路径实际使用的坐标系说明（写入工序注释便于核对）
	QString m_lastCoordInfo;

	// 最近一条路径的起始关节角文本（MOVJ行，取路径首点关节值，逗号分隔）
	QString m_lastMovjText;

	// GUNT/GUNF编号：整个输出文件内递增（1,2,3,...）
	int m_gunCounter = 1;

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
