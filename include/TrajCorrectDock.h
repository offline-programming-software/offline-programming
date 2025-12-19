#pragma once
#pragma once

#include <QWidget>
#include "ui_TrajCorrectDock.h"
#include"qdockwidget.h"
#include <comdef.h>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include"pickWidget.h"
#include"Correction.h"
#include<vector>
#include"PickSpinBox.h"
#include<Eigen/Core>
#include<Eigen/Dense>
#include"robxFileIO.h"
QT_BEGIN_NAMESPACE
namespace Ui { class DockContent; };
QT_END_NAMESPACE

class TrajCorrectDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit TrajCorrectDock(CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback *ptrKitCallback, QWidget *parent = nullptr);
	~TrajCorrectDock();

	enum class PickSource {
		None,
		FromListFlags,
		FromSpinRanges

	};

	enum class DrawSource
	{
		None,
		FromFlagPoints,
		FromMeasurePoints,
		FromRangePoints
	};
signals:
	void blankAreaClicked();
private:
	
    Ui::DockContent *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback *m_ptrKitCallback2;
	RobxIO* m_io;

	/*-----------------------控件-----------------------------*/
	pickWidget *pickBox;    //拾取点
	PickSpinBox *xMinspin;
	PickSpinBox* xMaxspin;
	PickSpinBox *yMinspin;
	PickSpinBox *yMaxspin;
	PickSpinBox *zMinspin;
	PickSpinBox *zMaxspin;
	QVector<PickSpinBox*> spinBoxes;
	PickSource m_pickSource = PickSource::None;
	DrawSource m_drawSource = DrawSource::None;
	int pointCount = 0;
	int rangeBoxIndex;
	int m_correctCounter = 0;
	/*-------------------------成员变量--------------------------------*/
	
	std::vector<Correction*> m_correction;
	QVector<Correction> m_correctionList;
	QVector<QListWidgetItem*> m_correctionItems;
	std::vector<double> m_vFlagPoints;
	std::vector<double> m_vMeasurePoints;

	std::vector<unsigned long> m_vAllPathIDs;      
	std::vector<std::string> m_vAllPathNames;
	std::vector<unsigned long>m_vAllPointIDs;
	std::vector<std::vector<double>> m_v2dAllPointsPositions;
	std::vector<std::vector<double>> m_v2dPointsToCorrect;
	std::vector<unsigned long> m_vPointsToCorrectID;
	std::map<unsigned long, std::vector<unsigned long>> m_mapAllPointIDs;
	std::map<unsigned long, std::vector<std::vector<double>>> m_mapAllPointPositions;
	std::map<unsigned long, std::vector<std::vector<double>>> m_mapPointsToCorrect;
	

		/*-------------------------成员方法----------------------------*/

	void importCsvPoints(const QString& filePath);
	void importCsvPointsMeasure(const QString& filePath);
	void exportCsvFlagPoints(const QString& filePath);
	void exportCsvMeasurePoints(const QString& filePath);
	void getTrajPoints(double range[6], std::vector<std::vector<double>>& trajPointsToCorrect);

	/*-------------------------初始化函数----------------------------*/
	void initDock();
	void InitLists();
	void initPointLists();
	void initGroupBox_AttributeDefine(QListWidgetItem *item);
	void InitCustomWidget();
	
	/*-------------------------pq工具函数----------------------------*/
	void getObjIdByName(PQDataType i_datatype, std::wstring i_wsName, ULONG & o_uID);
	void GetAllPathID();    //读取robx中所有轨迹ID, 名称, 
	void GetPointInfo();    //读取robx中所有轨迹点ID, 位姿
	void GetPoints2Correct(double range[6]);    //读取robx中作用域范围内的轨迹点ID, 位姿
	void GetParentPath();
	void modifyPointsPoses(const std::vector<unsigned long>& CorrectPointID, const std::vector<std::vector<double>>& newPoints);

public slots:
	void testSlot();
	void testSignal(int num);
	void on_this_blankAreaClicked();
	void on_btnNewCorrection_clicked();
	void on_btnDeleteCorrection_clicked();
	void on_btnAttributeSetOK_clicked();
	void on_btnSave_clicked();
	void on_listCorrection_slectedItem(QListWidgetItem *item);
	void on_pickBox_blankAreaClicked();
	void on_pickBox_delet();
	void on_pickBox_clear();
	void onImportFlagPointsClicked();
	void on_btnMeasurePtsInport_clicked();
	void on_btnFlagPointsExport_clicked();
	void onExportMeasurePointsClicked();
	void onComboBoxIndexChanged(int index);
	void text();
	void pickRange();
	void onPickSpinBoxValueChanged(int a);
	void OnPickup(unsigned long i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);
	void OnElementSelection(LPWSTR i_wObjNames, LPWSTR i_wFaceNames, double* i_dPointXYZ, int i_nSize);
	void OnDraw();
	void on_btnTest_2_clicked();

	/*-------------------------测试槽----------------------------*/
	void on_btnTest_clicked();
	void on_chkApplyCor_toggled(bool checked);
protected:
	void mousePressEvent(QMouseEvent *event) override;
};




/**
  * \class beamFrame
  * \brief 描述一个尾梁坐标系的类
  *
  * \details 尾梁坐标系以尾梁起始点截面质心为原点，Z方向竖直向上，X方向沿着尾梁尾部方向
  *          %beamFrame 类可以通过输入一组数据计算坐标系的原点坐标以及X轴方向向量，最终确定坐标系。
  */

class beamFrame
{

public:
	/**
	  * \fun beamFrame
	  * \brief %beamFrame 类的构造函数，用于计算坐标系位置，参考了robotstudio中的三点建立坐标系
	  *
	  * \param[in] flagPoints 在直升机尾梁上分布的一些标志点
	  * \param[in] measuredPoints
	  * \param[in] point1 尾梁根部一点xyz坐标
	  * \param[in] point2 用户选取，尾梁尾部一点xyz坐标，尽量和 %point1 在同一 y 平面（全局坐标系）上，假设整机沿着x轴停靠
	  */
	beamFrame(Eigen::MatrixXd flagPoints, Eigen::MatrixXd measuredPoints, Eigen::Vector3d point1, Eigen::Vector3d point2);
	~beamFrame();
	Eigen::MatrixXd flagPoints() { return m_flagPoints; }
	Eigen::MatrixXd measuredPoints() { return m_measuredPoints; }
	Eigen::Vector3d origin() { return m_originPoint; }


private:

	Eigen::MatrixXd m_flagPoints,
		m_measuredPoints;
	Eigen::Matrix4d m_beamPose;/**坐标系位姿*/
	Eigen::Vector3d m_originPoint;/**坐标系原点*/
	Eigen::Vector3d m_xDir, m_point1, m_point2;//(x0,y0,0)

	void getXDir();
	void getOrigin();
	void getBeamPose();
	void getPoints();
	void transFormMatrix(const Eigen::Matrix4d T, Eigen::MatrixXd& P);
};

