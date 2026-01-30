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

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class cursePartClass; };
QT_END_NAMESPACE

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
	void on_comboBox_currentTextChanged(const QString& text);  // 机器人组合框变化
	void on_previewButton_clicked();  // 预览按钮
	void on_spaceSettingButton_clicked();  // 文本确认按钮
	void on_textEdit_4_textChanged();  // 修改为无参数
	void on_horizontalSlider_valueChanged(int value);//修改划分起点的长度方向位置
	void on_verticalSlider_valueChanged(int value);//修改划分起点的宽度方向位置
	void on_coordanateTextChanged(); //选择主划分方向 - 移除参数
	void on_confirm_clicked(); //最后确认按钮
	void on_calculate_workspace();//获取机器人工作空间
	void OnDraw();
	void OnElementPickup(ULONG i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);

	void onDialogFinished(int result);

private:
	Ui::cursePartClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	bool isPickupActive = false;       // 使用成员变量而不是局部变量
	bool isPreview = false;
	bool isPoint = false;

	//包围盒
	AABB box;
	std::vector<double> m_vPosition;
	std::vector<double> ABBPosition;//包围盒八个角点
	std::vector<double> points;

	//实现曲面的选取
	std::map<ULONG, std::vector<std::wstring>> pickupMap;//用于记录选取的曲面
	QMap<ULONG, QString> CoodernateMap; //记录坐标系名称和ID
	int indx = 0;

	double x_value;
	double y_value;
	double z_value;

	// 新增：包围盒在选择方向上的厚度
	double m_thickness = 0.0;



private:

	void setupGraphicsScenes(); // 初始化图形场景
	void setStepsExplanation();
	void init();
	void updateRailOptions(const QString & robotName, const QMap<ULONG, QString>& robotMap);
	void addItemToListView(const QString& item);
	std::vector<double> calculateAABBCornersFromPickupMap(const std::map<unsigned long,
		std::vector<std::wstring>>&pickupMap);//设置工作空间八个角点
	void CreateBoundingBox();
	std::vector<double> getAxisVector(const std::vector<std::vector<double>>& axis, const QString& name);
	std::vector<std::vector<double>> getCoordinateAxesFromEuler(double* eulerAngles);
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);
	QStringList extractStringArrayFromVariant(const VARIANT& variant);
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);

	// 保存机器人映射以便在槽函数中使用
	QMap<ULONG, QString> m_robotMap;
};


