#pragma once

#include <QWidget>
#include "ui_PositionCorrectWidget.h"
#include "pickWidget.h"
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include <vector>
#include <Windows.h>
#include <array>
	
QT_BEGIN_NAMESPACE
namespace Ui { class PositionCorrectClass; };
QT_END_NAMESPACE

class PositionCorrectWidget : public QWidget
{
	Q_OBJECT

public:
	PositionCorrectWidget(CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback* ptrKitCallback, QWidget* parent = nullptr);
	~PositionCorrectWidget();

private:
	// 获取对象ID和名称
	void getOBJID();
	
	// 获取坐标系ID和名称
	void getCoordID();

	//初始化到ui
	void initCmbBox();

	//设置当前的ID
	void setCurrentID();

	//应用位置修正
	void setObjPos(const ULONG& objID,const ULONG& coordID, std::array<double, 6> correctionValues);

	// pqObj - 对象ID列表和名称列表
	std::vector<ULONG> m_objIDs;
	std::vector<QString> m_objNames;
	
	// pqCoord - 坐标系ID列表和名称列表
	std::vector<ULONG> m_coordIDs;
	std::vector<QString> m_coordNames;

	//当前选中的对象和坐标系
	ULONG m_currentObjID = 0;
	ULONG m_currentCoordID = 0;

	//修正量
	std::array<double, 6> m_correctionValues = { 0 };

	
private:
	Ui::PositionCorrectWidgetClass *ui;
	pickWidget* pickObj;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

private slots:
	void cmbObj_currentIndexChanged(int index);
	void cmbCoord_currentIndexChanged(int index);
	void on_btnApply_clicked();

};
//需要用到的接口说明：
// 获取对象ID
// HRESULT pq_GetAllDataObjectsByType (PQDataType i_eObjType,BSTR* o_sNames,BSTR* o_sIDs)  SDK:9.2.0.6420(函数本体最低支持版本) 11.1.0.7310(PQDataType 最低支持版本)  Parameters:  i_eObjType 指定对象类型,枚举类型值详见 9.1 数据类型定义  o_sNames 所有指定类型对象的名字,以"#"分割  o_sIDs 所有指定类型对象的 ID,以"#"分割
// 修改对象位姿
//HRESULT Doc_set_obj_posture(ULONG i_ulID, DOUBLE *i_dPosture, INT i_nPostureArraySize, PQPostureType i_ePostureType,ULONG i_ulCoorID)  SDK:9.2.0.6420(函数本体最低支持版本) 11.1.0.7310(PQPostureType 最低支持版本)  Parameters:  i_ulID 欲设置位姿的对象 ID  i_dPosture 位姿数据  i_nPostureArraySize 位姿数据数组大小  i_ePostureType 指定姿态表示方式:欧拉角、四元素,详见 9.2 位姿描述定义  i_ulCoorID 位姿参考坐标系,若为世界坐标系传 0
//pq枚举类型
//机器人 PQ_ROBOT  工件 PQ_WORKINGPART  工具 PQ_TOOL 坐标系 PQ_COORD
