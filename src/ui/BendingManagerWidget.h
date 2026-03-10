#pragma once
#include <QWidget>
#include "ui_BendingManagerWidget.h"
#include <comdef.h>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include "model\CorrectionModel.h"
#include "utils\utils.h"
QT_BEGIN_NAMESPACE
namespace Ui { class BendingManagerClass; };
QT_END_NAMESPACE

class BendingManager;

/**
 * @class BendingManager
 * @brief 管理修正对象是否应用.
 * 
 * 包含修正对象的列表视图，用户可以通过勾选或取消勾选来控制每个修正对象是否应用于当前的轨迹修正过程。
 * 过程：
 *  0. 通过pq接口读取所有的轨迹点数据
 *	1. 通过RobxFileIO类加载修正对象数据，并将其显示在树形视图中，参考robxFileIO.h
 *  2. 用户可以在树形视图中选择或取消选择修正对象。
 *  3. 选中对象后，根据对象的**作用域**来取出需要修正的轨迹点数据。
 *  4. 对选中的修正对象，应用其修正算法对对应的轨迹点进行修正。
 * 
 */
class BendingManagerWidget : public QWidget
{
	Q_OBJECT

public:
	BendingManagerWidget(CComPtr<IPQPlatformComponent> ptrKit,
						CPQKitCallback* ptrKitCallback,
						CorrectionModel* model,
						QWidget* parent = nullptr);
	~BendingManagerWidget();

private:
	Ui::BendingManagerWidgetClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	CorrectionModel* m_model;
	int m_currentDrawRow = -1;

	void setConnections();
	void OnDraw();
	void closeEvent(QCloseEvent* event) override;
	void initTreeWidget();

private slots:
	void on_btnOK_clicked();
	//修改对应修正对象的isApply属性, 同时如果他有父对象，也把父对象的isApply属性修改为true，并且ui同步

	void on_treeCorrection_itemChanged(QTreeWidgetItem* item, int column);
	//
	void on_treeCorrection_currentItemChanded(QTreeWidgetItem* current, QTreeWidgetItem* previous);

};
