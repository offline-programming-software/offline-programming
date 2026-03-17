#ifndef POSCAL_H
#define POSCAL_H

#pragma execution_character_set("utf-8")
#define NOMINMAX
#include <QDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QSignalBlocker>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string.h>
#include "ui_posCal.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class posCalClass; };
QT_END_NAMESPACE

class posCal : public QDialog
{
	Q_OBJECT

public:
	posCal(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~posCal();

private slots:
	// 机器人选择变化槽函数
	void onComboBox1CurrentIndexChanged(int index);
	// 轨迹组选择变化槽函数  
	void onComboBox2CurrentIndexChanged(int index);
	// 选择是否需要AGV移动来实现站位计算的槽函数
	void onComboBox4CurrentIndexChanged(int index);
	
	void onCalculate();
	void onShow();
	void onConfirm();
	void onCancel();
	

private:
	Ui::posCalClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	QString m_tempDir = "./temp/jsons/";
	QMap<ULONG, QString> m_robotMap;// 机器人名称到ID的映射
	std::map<std::string, std::pair<std::string, std::string>> relationsMap;//存储关系

	// 初始化函数
	void init();
	
	std::map<std::string, std::pair<std::string, std::string>> loadRobotRelations(const std::string& filePath = "relations.json");



	//计算出平均法向
	std::vector<double> calculateAverageNormal(ULONG pathID);

	//计算出机器人在零点时，末端到达机器人基座标系的值
	std::vector<double> calculataRelativePos(ULONG robotID, std::vector<double> robotJoints);

	//计算机器人站位
	std::vector<double> adjustRobotBasePosition(ULONG coordinateID,const std::vector<double>& pointWorld,
		const std::vector<double>& pointRobot);

	//计算出AGV旋转角度
	double calculateAGVJoint(std::vector<double>robotDir, std::vector<double>pathDir, ULONG baseCoordinate);


	std::vector<double> extractZAxisFromQuaternion(const std::vector<double>& quat);

	double calculateRotationToOpposite(const std::vector<double>& v1,
		const std::vector<double>& v2,
		const std::vector<double>& rotationAxis);
	
	
	//计算得到导轨的运动方向向量
	std::vector<std::vector<double>> calculateJointMovementDir(ULONG guideID);

	//通过移动方向和移动距离计算出导轨的关节值
	std::vector<double> calculateJointValues(const std::vector<double>& move,
		const std::vector<std::vector<double>>& guideDir);

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
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG& o_uID);

};


#endif // POSCAL_H