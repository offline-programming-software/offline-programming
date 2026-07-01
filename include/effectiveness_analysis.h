#pragma once

#pragma execution_character_set("utf-8")

#include <QtCharts>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QChartView>

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>
#include <vector>
#include <QString>
#include <unordered_map>

#include "ui_effectiveness_analysis.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class effectiveness_analysisClass; };
QT_END_NAMESPACE

// 轨迹点信息存储结构体
struct pointInformation {
	double x;                      // X坐标
	double y;                      // Y坐标
	double z;                      // Z坐标
	double vel;                    // 速度
	PQPointInstruction moveType;   // 运动类型

	// 默认构造函数 - 初始化为默认值
	pointInformation()
		: x(0.0), y(0.0), z(0.0), vel(0.0), moveType(PQ_LINE) {}

	// 带参数的构造函数
	pointInformation(double posX, double posY, double posZ, double velocity, PQPointInstruction type)
		: x(posX), y(posY), z(posZ), vel(velocity), moveType(type) {}

	// 重置为默认值的函数
	void reset() {
		x = 0.0;
		y = 0.0;
		z = 0.0;
		vel = 0.0;
		moveType = PQ_LINE;
	}

	// 设置值的函数
	void setValues(double posX, double posY, double posZ, double velocity, PQPointInstruction type) {
		x = posX;
		y = posY;
		z = posZ;
		vel = velocity;
		moveType = type;
	}
};

class effectiveness_analysis : public QDialog
{
	Q_OBJECT

public:
	effectiveness_analysis(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~effectiveness_analysis();

private:
	Ui::effectiveness_analysisClass *ui;

	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	// 图表相关
	QChart *barChart;
	QChart *pieChart;
	QChartView *columnGraph;
	QChartView *pieGraph;

	// 数据存储
	QList<QBarSet*> barSets;
	QStringList categories;

	//机器人轨迹点信息 - 使用新的数据结构
	std::unordered_map<ULONG, std::vector<pointInformation>> robotPointsMap;  // 机器人ID -> 点信息列表
	std::unordered_map<ULONG, std::vector<std::pair<ULONG, std::vector<pointInformation>>>> robotPathPointsMap; // 机器人ID -> (路径ID, 点信息列表)的列表

	// AGV轨迹点信息 - 独立的数据结构
	QMap<ULONG, std::vector<pointInformation>> AGVPointMap;  // AGV ID -> 点信息列表

private:
	void initData();                 // 初始化数据
	void createBarChart();           // 创建柱状图
	void createPieChart();           // 创建扇形图

	// 获取机器人名称
	QString getRobotName(ULONG robotID);
	// 获取AGV名称
	QString getAGVName(ULONG agvID);

	//轨迹点信息计算
	void calculatePointInformation();

	// AGV轨迹点信息计算
	void calculateAGVInformation();

	// 机器人时间计算函数
	double calculatePathTime(ULONG robotID, ULONG pathID);      // 计算单个路径的时间
	double calculateTotalTime();                                // 计算所有路径的总时间
	double calculateRobotTime(ULONG robotID);                   // 计算单个机器人的总时间
	QMap<ULONG, double> calculateRobotTimePercentage();         // 计算各机器人时间占比

	// 机器人喷涂距离计算函数
	double calculatePathSprayDistance(ULONG robotID, ULONG pathID);      // 计算单个路径的喷涂距离
	double calculateTotalSprayDistance();                                // 计算所有路径的总喷涂距离
	double calculateRobotSprayDistance(ULONG robotID);                   // 计算单个机器人的总喷涂距离
	QMap<ULONG, double> calculateRobotSprayDistancePercentage();         // 计算各机器人喷涂距离占比

	// 机器人喷涂效率计算函数
	double calculateSprayEfficiency(ULONG robotID, ULONG pathID);        // 计算单个路径的喷涂效率
	double calculateAverageSprayEfficiency();                            // 计算平均喷涂效率

	// AGV时间计算函数
	double calculateAGVTotalTime(ULONG agvID);                    // 计算单个AGV的总时间

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

private slots:
	void chart_init();
};