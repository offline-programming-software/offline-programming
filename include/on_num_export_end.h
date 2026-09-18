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
// 按CATIA APT格式（GOTO / X,Y,Z,I,J,K）将全部轨迹点合并输出到同一个文件
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
	};

private:
	void initUI();
	void loadCoordinates();

	// 生成APT文件头（$$注释 + PARTNO + 换刀工序，到LOADTL/1,1,1为止）
	QStringList buildAptHeader(const QString& partName);

	// 向内容追加一个轨迹工序块（工序注释 + FEDRAT/SPINDL + 该路径全部GOTO点）
	void appendAptOperation(QStringList& lines, const std::vector<AptPoint>& points,
		const QString& operationName);

	// 采集单条路径的全部轨迹点（坐标系变换 + 相邻点距离检查与5mm插补）
	// targetCoordID为用户指定的输出坐标系ID，0=自动（路径关联）
	bool collectPathPoints(ULONG pathID, ULONG targetCoordID, std::vector<AptPoint>& points);

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
