#pragma once

#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <algorithm>
#include <limits>
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
	void onAGVSelectionChanged(int index);


private:
	Ui::AGVpathClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	QMap<ULONG, QString> m_AGVMap;

	void init();

	std::vector<QVector3D> calculateTransitionPoints(
		ULONG startPointID, ULONG endPointID,
		double* startPosture, double* endPosture,
		const QVector3D& dir, double boundaryValue);

	double calculateBoundaryValue(const AABB& bbox, const QVector3D& dir);

	QVector3D getDirectionVector(const QString& dirName);

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
