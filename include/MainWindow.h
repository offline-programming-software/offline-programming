#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

#pragma execution_character_set("utf-8")

#include <QMainWindow>
#include "SARibbon.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <iostream>
#include <vector>
#include <array>
#include <Eigen/Dense>


#include "mydialog.h"
#include "kinetic_analysis.h"
#include "InsertPathDlg.h"
#include "effectiveness_analysis.h"
#include "path_partion.h"
#include "path_edit.h"
#include "path_set_up.h"
#include "path_sort.h"
#include "path_optimize.h"
#include "deformation_correction.h"
#include "TrajectoryFileGenerator.h"
#include "postProcessing.h"
#include "AGVpath.h"
#include "cursePart.h"
#include "robotSpaceDefine.h"
#include "posCal.h"
#include "addRobotSpace.h"
#include "spaceCalculate.h"

#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include "boundBox.h"
#include "robxFileIO.h"
#include "TrajCorrectDock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public SARibbonMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	void InitPQKit();
	void ShowPQKitWindown();
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);//获取机构名称
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);//获取机构ID和机构名称
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);//机器人ID和机构名称

private:
	Ui::MainWindow *ui;

	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	PQKitInitThread* m_ptrKitInitThread;

	QWidget* m_pPQPlatformView;
	QWidget* m_pPQModeTreeView;
	QWidget* m_pPQDebugView;

private:
	void closeEvent(QCloseEvent* event);

	//区域划分变量
	cursePart* curseDialog = nullptr;  // 保存对话框指针
	bool isPickupActive = false;       // 使用成员变量而不是局部变量
	bool isPreview = false;
	bool isPoint = false;

	//机器人工作空间定义
	robotSpaceDefine* spaceDialog = nullptr;
	QString axisName;
	int axisIdx;//坐标轴ID

	std::vector<double> axisPosture;

	QStringList extractStringArrayFromVariant(const VARIANT& variant);//提取variant中数据
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	 
	//实现曲面的选取
	std::map<ULONG, std::vector<std::wstring>> pickupMap;//用于记录选取的曲面
	void onDeleteSelectedSurfaces(const QStringList& surfaceNames);//删除选中的曲面
	std::vector<double> calculateAABBCornersFromPickupMap(const std::map<unsigned long, 
		std::vector<std::wstring>>& pickupMap);//设置工作空间八个角点

	//包围盒
	AABB box;
	std::vector<double> m_vPosition;
	std::vector<double> ABBPosition;//包围盒八个角点
	std::vector<double> points;

	//坐标轴方向向量计算
	std::vector<std::vector<double>> getCoordinateAxesFromEuler(double* eulerAngles);

	//利用复选框中的string中选择方向向量
	std::vector<double> getAxisVector(const std::vector<std::vector<double>>& axis, const QString& name);

	QString generatePathGroupPostContent(long robotId, BSTR pathGroupId,
	const QString& robotName, const QString& pathGroupName);//获取轨迹点名字
	void CreateBoundingBox();
	void updateRailOptions(const QString& robotName, const QMap<ULONG, QString>& robotMap);

protected slots:
	//PQKit slots
	void OnInitializeResult(long lResult);
	void OnRunCMDResult(long lResult);
	void OnGetDataResult(long lResult);
	void OnRaiseDockwindow(int i_nType);
	void OnLoginResult(int i_nLoginType);
	void OnPathGenerateResult(long i_bSuccess, int i_nPathCount, int i_nIndex, unsigned long i_ulPathID);
	void OnElementPickup(ULONG i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);
	void OnRButtonUp(long i_lPosX, long i_lPosY);
	void OnLButtonUp(long i_lPosX, long i_lPosY);
	void OnMenuPop(unsigned long i_ulObjID, long i_lPosX, long i_lPosY, int* o_nHandled);
	void OnInitializeKitThread();
	void OnDraw();


	//own slot
	void OnOpenRobx();
	void OnSaveRobx();
	void OnSaveasRobx();
	void on_close_robx();
	void on_input();
	void on_proxy();
	void on_measurement();
	void on_create_frame();
	void on_create_path();
	void OnCompile();
	void on_simulate();
	void on_post();
	void on_numPost();
	void on_defining_institutions();
	void on_import_institutions();
	void on_defining_tool();
	void on_import_tool();
	void on_defining_part();
	void on_import_part();
	void on_create_newsketch();
	void on_edit_wkptsketch();
	void on_sketcher_createpoint();
	void on_sketcher_line();
	void on_sketcher_arc();
	void on_sketcher_circ();
	void on_sketcher_ellipticalarc();
	void on_sketcher_ellipse();
	void on_sketcher_rectangle();
	void on_sketcher_regpolygon();
	void on_sketcher_slot();
	void on_sketcher_polyline();
	void on_sketcher_fillet();
	void on_sketcher_triming();
	void on_choose_robot();
	void on_floor();
	void on_motion_option();
	void on_choosetool();
	void on_campath_flat_surface();
	void on_roobjassistor_manage();
	void on_rocreate_sprayingtool();
	void on_collision_option();
	void on_insert_path();
	void on_kinetic_analysis();
	void on_effectiveness_analysis();
	void on_AlignPart3Point();
	void on_AlignPartPointAxis();
	void on_path_partion();
	void on_path_edit();
	void on_path_optimize();
	void on_path_set_up();
	void on_path_sort();
	void on_addlinearspraypath();
	void on_sync();
	void on_deformation_correction();
	void on_action();
	void on_kinematics();
	void on_linkage();
	void on_animation();
	void on_video();
	void on_define_space();
	void on_curse_part();
	void on_pos_cal();
	void on_AGV_path();
	void on_trajCorrectdock_open();

};
#endif  // MAINWINDOW_H
