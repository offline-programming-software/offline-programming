// cursePart.h
#pragma once
#pragma execution_character_set("utf-8")
#define NOMINMAX 

#include <QDialog>
#include <QStringListModel>
#include <QMessageBox>
#include <qDebug>
#include <QTimer>
#include "boundBox.h"
#include "ui_cursePart.h"
#include "PQKitCallback.h"
#include "parseJSON.h"
#include "spaceCalculate.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class cursePartClass; };
QT_END_NAMESPACE

// 机器人工作空间处理器类
class RobotWorkspaceHandler {
private:
	std::string jsonFileName;

public:
	RobotWorkspaceHandler(const std::string& fileName) : jsonFileName(fileName) {}

	// 写入机器人工作空间边界数据到JSON文件
	void writeRobotWorkspaceBoundary(const RobotWorkspaceBoundary& boundary);

	// 根据条件查找并获取最匹配的points值
	std::vector<double> findMatchingPoints(
		ULONG robotID,
		const QString& coordinateName,
		const QString& directionName,
		double targetTheta,
		double targetThickness);

	// 从points中找到最接近给定值的点（向上取整）
	double findClosestPointUp(const std::vector<double>& points, double targetValue);

	// 综合查找方法：根据UI参数查找并计算最接近的points
	std::vector<double> processRobotWorkspaceQuery(
		const QString& robotName,
		ULONG robotID,
		const QString& coordinateName,
		const QString& directionName,
		double theta,
		double thickness);
};


class cursePart : public QDialog
{
	Q_OBJECT

public:
	cursePart(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~cursePart();

signals:


private slots:
	void on_next_page_clicked();  // 下一页
	void on_prev_page_clicked();  // 上一页
	void on_cancel_clicked();     // 取消
	void on_pickUpButton_clicked();    // 信号1
	void on_finishButton_clicked();  // 关闭信号
	void on_deleteButton_clicked();  // 删除选中项
	void on_pickupPoint_clicked(); //拾取点
	void on_previewButton_clicked();  // 预览按钮
	void on_spaceSettingButton_clicked();  // 文本确认按钮
	void on_horizontalSlider_valueChanged(int value);//修改划分起点的长度方向位置
	void on_verticalSlider_valueChanged(int value);//修改划分起点的宽度方向位置
	void on_coordanateTextChanged(); //选择主划分方向 - 移除参数
	void on_confirm_clicked(); //最后确认按钮
	void on_calculate_workspace();//获取机器人工作空间
	void OnDraw();
	void onRobotSelectionChanged(const QString& currentRobotName);
	void OnElementPickup(ULONG i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);

	void onDialogFinished(int result);

private:
	Ui::cursePartClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	bool isPickupActive = false;       // 使用成员变量而不是局部变量
	bool isPreview = false;
	bool isPoint = false;			   //拾取点

	//包围盒
	AABB divideBox; //划分包围盒
	OBB minBox;   //最小包围盒
	

	std::vector<double> OBBPosition;  //最小包围盒八个角点
	std::vector<double> AABBPosition; //划分包围盒八个角点
	std::vector<double> points;      //划分区域角点
	std::vector<double> partPositions; //

	//方向读取
	std::vector<double> mainDirction;//主方向
	std::vector<double> divisionDirection;//划分方向
	std::vector<double> otherDirection;//次要划分方向

	//实现曲面的选取
	std::map<ULONG, std::vector<std::wstring>> pickupMap;//用于记录选取的曲面
	int indx = 0;
	double x_value;
	double y_value;
	double z_value;

	// 新增：包围盒在选择方向上的厚度
	double m_thickness = 0.0;
	double d_thickness = 0.0;
	QString m_tempDir = "./temp/jsons/";

	// 新增：机器人工作空间处理器
	RobotWorkspaceHandler* m_workspaceHandler = nullptr;

	// 保存机器人映射以便在槽函数中使用
	QMap<ULONG, QString> m_robotMap;

	//存储从 relations.json 加载的机器人关系
	std::map<std::string, std::pair<std::string, std::string>> relationsMap;


private:
	void setupGraphicsScenes(); // 初始化图形场景
	void setStepsExplanation();// 设置每个界面的功能介绍
	void init(); //初始化界面
	void addItemToListView(const QString& item);//在曲面控件中添加曲面
	std::vector<double> calculateAABBCornersFromPickupMap(const std::map<unsigned long,
		std::vector<std::wstring>>&pickupMap);//设置工作空间八个角点
	std::vector<double> calculateOBBCornersFromPickupMap(
		const std::map<unsigned long, std::vector<std::wstring>>& pickupMap, std::vector<double> direction);
	std::vector<double> getAxisVector(const std::vector<std::vector<double>>& axis, const QString& name);//获取坐标系某个轴的方向向量
	std::vector<std::vector<double>> getCoordinateAxesFromEuler(double* eulerAngles);//通过坐标系ID获取坐标系位置
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);//通过数据类型获取ID和名称
	std::map<std::string, std::pair<std::string, std::string>> loadRobotRelations(const std::string& filePath = "relations.json");
	void updateLinkedJointCheckBoxes(const std::string& railNameStr);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);//将variant转化为QStringList
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);//将variant转化为QList
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);//获取喷涂机器人列表
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);//通过名称获取ID
	
	//创建坐标系
	std::vector<double> convertToLocalEulerAngles(
		const std::vector<double>& point,
		const std::vector<double>& mainDirection,
		const std::vector<double>& otherDirection);

	//将世界坐标系点转化为局部坐标系
	std::vector<double> transformPointToLocal(
		const std::vector<double>& worldPoint,
		const std::vector<double>& localCoordinateSystem);

	//将局部坐标系点转化为世界坐标系
	std::vector<double> transformPointToWorld(
		const std::vector<double>& localPoint,
		const std::vector<double>& localCoordinateSystem);

	std::vector<double> transformMultiplePointsToLocal(
		const std::vector<double>& worldPoints,
		const std::vector<double>& localCoordinateSystem);

	std::vector<double> transformMultiplePointsToWorld(
		const std::vector<double>& localPoints,
		const std::vector<double>& localCoordinateSystem);

	// 新增：通过叉乘计算除两个方向外的第三个坐标轴方向
	std::vector<double> getThirdAxisVectorFromTwoAxes(
		const std::vector<std::vector<double>>& axisVector,
		const QString& firstAxisName,
		const QString& secondAxisName);

	//计算得到包围盒长宽厚
	bool calculateDimensionsFromCorners(const std::vector<double>& OBBPosition,
		const QString& mainVectorText, double& length, double& width, double& thickness);

	// 新增：保存工作空间数据的方法
	void saveWorkspaceData();
};


