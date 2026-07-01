#pragma once
#pragma execution_character_set("utf-8")

#include <QtCharts>
#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QMap>
#include <QString>
#include <QStringList>
#include <Qtcharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "ui_kinetic_analysis.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class kinetic_analysisClass; };
QT_END_NAMESPACE

class kinetic_analysis : public QDialog
{
	Q_OBJECT

public:
	kinetic_analysis(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);

	~kinetic_analysis();

private:
	Ui::kinetic_analysisClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	QChart *chart1;
	QChart *chart2;
	QChart *chart3;
	QSplineSeries *series1;
	QSplineSeries *series2;
	QSplineSeries *series3;
	QChartView *chartview1;
	QChartView *chartview2;
	QChartView *chartview3;

	// 机器人名称到ID的映射
	QMap<ULONG, QString> m_robotMap;

	std::vector<double> times;
	std::vector<std::vector<double>> angles;  // 存储关节角度数据
	std::vector<std::vector<double>> vels;
	std::vector<std::vector<double>> accs;

	// 初始化函数
	void init();

	// 计算出喷涂时间节点
	std::vector<double> calculateTime(ULONG pathID);

	// 计算出关节角度（弧度制）
	std::vector<std::vector<double>> calculateAngle(ULONG pathID, const std::vector<double>& time);

	// 计算出关节角速度（弧度制）
	std::vector<std::vector<double>> calculateVel(ULONG pathID, const std::vector<double>& time);

	// 计算出关节角加速度（弧度制）
	std::vector<std::vector<double>> calculateAcc(const std::vector<double>& time,
		const std::vector<std::vector<double>>& vel);

	void setupChartView(QChartView* chartView, QChart* chart);
	void configureAxis(QChart* chart, double xMin, double xMax,
		double yMin, double yMax, const QString& xTitle, const QString& yTitle);

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
	void chartDrawing();

	// 机器人选择变化槽函数
	void onComboBox1CurrentIndexChanged(int index);
	// 轨迹组选择变化槽函数  
	void onComboBox2CurrentIndexChanged(int index);

};