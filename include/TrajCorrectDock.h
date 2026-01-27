#pragma once

#include <QWidget>
#include "ui_TrajCorrectDock.h"
#include"qdockwidget.h"
#include <comdef.h>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include"pickWidget.h"
#include"core\Correction.h"
#include<vector>
#include"PickSpinBox.h"
#include<Eigen/Core>
#include<Eigen/Dense>
#include"robxFileIO.h"
QT_BEGIN_NAMESPACE
namespace Ui { class DockContent; };
QT_END_NAMESPACE

class QScrollArea;

namespace 
{
	struct CorrectionDataModel
			{
		QString name;
		QString fittingType;
		QString beamDir;
		bool isPosCorrect;
		bool isTwisted;
		std::vector<double> flagPoints;
		std::vector<double> measurePoints;
	};
}
class TrajCorrectDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit TrajCorrectDock(CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback *ptrKitCallback, QWidget *parent = nullptr);
	~TrajCorrectDock();

	enum class PickSource {
		None,
		FromListFlags,
		FromSpinRanges,
		FromOriginPoint

	};

	enum class DrawSource
	{
		None,
		FromFlagPoints,
		FromMeasurePoints,
		FromRangePoints,
		FromOriginPoint
	};

	enum ItemState
	{
		Edit,
		View
	};


private:
	
    Ui::DockContent *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback *m_ptrKitCallback2;
	RobxIO* m_io;

	/*-----------------------自定义控件-----------------------------*/
	pickWidget *listFlagPoints;    //拾取点
	PickSpinBox *xMinspin;
	PickSpinBox* xMaxspin;
	PickSpinBox *yMinspin;
	PickSpinBox *yMaxspin;
	PickSpinBox *zMinspin;
	PickSpinBox *zMaxspin;
	PickSpinBox* spnOriginx;
	PickSpinBox* spnOriginy;
	PickSpinBox* spnOriginz;
	
	QVector<PickSpinBox*> spinBoxes;
	PickSource m_pickSource = PickSource::None;
	DrawSource m_drawSource = DrawSource::None;
	QScrollArea* scroll ;
	int pointCount = 0;
	int rangeBoxIndex;
	int m_correctCounter = 0;
	/*-------------------------**成员变量**--------------------------------*/
	
	QVector<Correction> m_correctionList;
	QVector<QListWidgetItem*> m_correctionItems;
	std::vector<double> m_vFlagPoints;
	std::vector<double> m_vMeasurePoints;
	

		/*-------------------------成员方法----------------------------*/

	void importCsvPointsMeasure(const QString& filePath);
	void exportCsvFlagPoints(const QString& filePath);
	void exportCsvMeasurePoints(const QString& filePath);
	void getTrajPoints(double range[6], std::vector<std::vector<double>>& trajPointsToCorrect);
	void initGroupEmpty(const QString name);

	/*-------------------------初始化函数----------------------------*/
	void initDock(QWidget* contentWidget);
	void InitLists();
	void initPointLists();
	void initGroupBox_AttributeDefine(QListWidgetItem *item);
	void InitCustomWidget();
	void setupConnections();


	/*-------------------------辅助函数----------------------------*/
	
public slots:
	void testSignal(int num);
	void on_this_blankAreaClicked();
	void on_btnNewCorrection_clicked();
	void on_btnDeleteCorrection_clicked();
	void on_btnAttributeSetOK_clicked();
	void on_btnSave_clicked();
	void on_btnRefresh_clicked();
	void on_listCorrection_slectedItem(QListWidgetItem *item);
	void on_listMeasurePoints_itemClicked(QListWidgetItem* item);
	void on_listFlagPoints_itemClicked(QListWidgetItem* item);
	void on_pickBox_blankAreaClicked();
	void on_btnExportFlagPoints_clicked();
	void on_btnImportMeasurePoints_clicked();
	void on_btnImportFlag_clicked();
	void on_spnOrigin_valueChanged(int a);
	void on_spnBeamDeg_valueChanged(double a);
	void pickRange();
	void pickOriginPoint();
	void on_cmbBeamDir_currentIndexChanged(int index);
	void onPickSpinBoxValueChanged(int a);
	void OnPickup(unsigned long i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);
	void OnElementSelection(LPWSTR i_wObjNames, LPWSTR i_wFaceNames, double* i_dPointXYZ, int i_nSize);
	void OnDraw();
	void setEdit();
	void setView();
	void on_tabInput_currentChanged(int index);
	/*-------------------------测试槽----------------------------*/
	void testConnection()
	{
		connect(ui->btnDelAll, &QPushButton::clicked, this, &TrajCorrectDock::on_btnDelAll_clicked);
	}
	void on_btnDevPage_clicked();
	void on_btnCal_clicked();
	void on_btnDelAll_clicked() {
		m_correctionList.clear();
		m_correctionItems.clear();
	}
	void on_btnRefreshLog_clicked() 
	{
		//ui->tblLog删除所有行
		ui->tblLog->setRowCount(0);
		//刷新correctItem信息：
		//根据m_correctionItems的数量，添加相应的行数，第一列填写item的text，第二列填写item的状态（编辑/查看）
		//获取currentitem，然后高亮显示
		for (int i = 0; i < m_correctionItems.size(); i++)
		{
			ui->tblLog->insertRow(i);
			ui->tblLog->setItem(i, 0, new QTableWidgetItem(m_correctionItems[i]->text()));
			QString state = (m_correctionItems[i]->data(Qt::UserRole).toInt() == Edit) ? "Edit" : "View";
			ui->tblLog->setItem(i, 1, new QTableWidgetItem(state));
		}
		QListWidgetItem* currentItem = ui->listCorrections->currentItem();
		if (currentItem)
		{
			for (int i = 0; i < ui->tblLog->rowCount(); i++)
			{
				if (ui->tblLog->item(i, 0)->text() == currentItem->text())
				{
					ui->tblLog->selectRow(i);
					break;
				}
			}
		}
		//刷新correctionList信息
		ui->tblLogCor->setRowCount(0);
		for(int i = 0; i < m_correctionList.size(); i++)
		{
			ui->tblLogCor->insertRow(i);
			ui->tblLogCor->setItem(i, 0, new QTableWidgetItem(m_correctionList[i].name()));
			
		}
		
	}
protected:
	void mousePressEvent(QMouseEvent *event) override;

signals:
	void groupAttributeChanged();
	void blankAreaClicked();
};




/**
  * \class beamFrame
  * \brief 描述一个尾梁坐标系的类
  *
  * \details 尾梁坐标系以尾梁起始点截面质心为原点，Z方向竖直向上，X方向沿着尾梁尾部方向
  *          %beamFrame 类可以通过输入一组数据计算坐标系的原点坐标以及X轴方向向量，最终确定坐标系。
  */


