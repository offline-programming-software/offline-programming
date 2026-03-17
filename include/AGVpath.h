#pragma once

#pragma execution_character_set("utf-8")
#define NOMINMAX
#include <QDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <limits>
#include "robxFileIO.h"
#include "boundBox.h"
#include "ui_AGVpath.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types


QT_BEGIN_NAMESPACE
namespace Ui { class AGVpathClass; };
QT_END_NAMESPACE

class AGVpath : public QDialog
{
	Q_OBJECT

public:
	AGVpath(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);

	~AGVpath();

private slots:

	void onCalculate();
	void onConfirm();
	void onCancel();
	void onInsertRow();
	void onDeleteRow();
	void onMoveRowUp();
	void onMoveRowDown();
	void onAddSimulationEvent();

	// AGV选择变化槽函数
	void onAGVSelectionChanged(int index);
	// 轨迹组选择变化槽函数  
	void onComboBox2CurrentIndexChanged(int index);
	void onComboBox4CurrentIndexChanged(int index);
	void onComboBox3CurrentIndexChanged(int index);
	void onComboBox5CurrentIndexChanged(int index);

private:
	Ui::AGVpathClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	QMap<ULONG, QString> m_robotMap;
	QString m_tempDir = "./temp/jsons/";
	std::map<std::string, std::pair<std::string, std::string>> relationsMap;//存储关系
	QVector<AgvStationInfo> agvStations; // 存储AGV站位信息;
	QMap<QString, ULONG> m_pathIdCache;

	void init();

	//初始化界面数据
	void initTable();

	//获取机器人、导轨、AGV的关联关系
	std::map<std::string, std::pair<std::string, std::string>> loadRobotRelations(const std::string& filePath = "relations.json");

	void swapTableRows(int firstRow, int secondRow);
	void updateAgvNameDisplay(const QString& robotName);
	QString findStationNames(
		const QString& robotName,
		const QString& groupName,
		const QString& pathName) const;

	bool tryGetStationInfo(
		const QString& robotName,
		const QString& groupName,
		const QString& pathName,
		const QString& stationName,
		AgvStationInfo& info) const;
	int insertTransitionRow(
		int rowIndex,
		const QString& groupName,
		const QString& pathName,
		const QString& stationLabel,
		const QVector3D& position,
		double theta);

	std::vector<QVector3D> calculateTransitionPoints(
		ULONG startPointID, ULONG endPointID,
		double* startPosture, double* endPosture,
		const QVector3D& dir, double boundaryValue);

	double calculateBoundaryValue(const AABB& bbox, const QVector3D& dir);

	QVector3D getDirectionVector(const QString& dirName);

	QVector<AgvStationInfo> loadAgvStationInfos() const;
	void refreshAgvStationTable();
	void updateVisibleStationRange();
	void syncTableToAgvStations();
	void persistAgvStations();
	void commitAgvStations();
	QString nextTransitionLabel() const;
	int findSortedInsertRow(const QString& groupName,
		const QString& pathName,
		const QString& stationLabel) const;

	QString buildPathCacheKey(ULONG robotID, const QString& groupName,
		const QString& pathName) const;
	bool tryGetPathId(ULONG robotID, const QString& groupName,
		const QString& pathName, ULONG& pathId);

	AABB creatAABB(ULONG uID, ULONG uCoordinate);

	// 根据类型获取对象列表
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);

	// 从VARIANT中提取字符串数组
	QStringList extractStringArrayFromVariant(const VARIANT& variant);

	// 从VARIANT中提取长整型数组
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);

	//获取机器人列表
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);

	// 获取轨迹组名称
	QStringList getPathGroupNames(ULONG robotID);

	// 获取轨迹名称
	QStringList getPathNames(ULONG robotID, const QString& groupName);

	//获取id
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);
};
