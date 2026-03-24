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
	BendingManager* m_manager;
	int m_currentDrawRow = -1;

	void setConnections();
	void OnDraw();
	void closeEvent(QCloseEvent* event) override;
	void initTreeWidget();

private slots:
	void on_btnOK_clicked();
	//修改对应修正对象的isApply属性, 同时如果他有父对象，也把父对象的isApply属性修改为true，并且ui同步

	/**
	 * \brief 当树形视图中的某个修正对象被选中或取消选中时：
	 * 0. 勾选时，检查该对象是否有父对象，取消勾选时，检查该对象是否有子对象
	 * 1. 更新对应修正对象的isApply属性
	 * 2. 发送信号，调用对应接口通知bendingmanager和对应correction执行更新
	 * 
	 * -----
	 * #status
	 * 测试通过
	 * -----
	 */
	void on_treeCorrection_itemChanged(QTreeWidgetItem* item, int column);

	/**
	 * \brief 当树形视图选中某个对象时,界面显示其作用域.
	 * 
	 * -----
	 * #status
	 * 待实现
	 * -----
	 */
	void on_treeCorrection_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

	/**
	 * \brief bendingmanager函数入口，接受 itemChanged发出的信号 
	 */
	void applyBendingCorrection(Correction& cor);

	void on_btnAppendLog_clicked();

	void on_chkViewRange_stateChanged(int arg1);

signals:
	void correctionApplyStateChanged(Correction& cor);
	

};
