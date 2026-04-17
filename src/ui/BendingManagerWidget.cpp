#include "ui\BendingManagerWidget.h"
#include<qcheckbox.h>
#include<iostream>
#include<qdebug.h>
#include<utils/utils.h>
#include"RobxFileIO.h"
#include<qlayout.h>
#include<qlabel.h>
#include<qmap.h>
#include<core\BendingManager.h>

class CorrectionModel;

BendingManagerWidget::BendingManagerWidget(CComPtr<IPQPlatformComponent> ptrKit, 
										   CPQKitCallback* ptrKitCallback,
										   CorrectionModel *model,
										   QWidget *parent)
	: QWidget(parent),
	m_ptrKit(ptrKit),
	m_ptrKitCallback(ptrKitCallback),
	m_model(model)
	, ui(new Ui::BendingManagerWidgetClass())
{
	ui->setupUi(this);
	setWindowTitle(QString::fromLocal8Bit("对象变形修正"));
	setWindowFlags(Qt::Window
		| Qt::WindowContextHelpButtonHint   // 问号
		| Qt::WindowCloseButtonHint);       // 关闭

	setConnections();
	m_manager = new BendingManager(m_ptrKit, m_model);
	m_manager->initOriginPointsSnapshot();
	m_manager->rebuildParentChildRelation();
	m_manager->initPathIDMap();
	m_manager->initIncludedPathForCorrections();
	auto& items = m_model->getItems();
	ui->treeCorrection->blockSignals(false);
	initTreeWidget();
	ui->treeCorrection->blockSignals(false);

}

BendingManagerWidget::~BendingManagerWidget()
{
	delete ui;
	delete m_manager;
}



void BendingManagerWidget::setConnections()
{
	connect(m_ptrKitCallback, &CPQKitCallback::signalDraw, this, &BendingManagerWidget::OnDraw);
	connect(this, &BendingManagerWidget::correctionApplyStateChanged, this, &BendingManagerWidget::applyBendingCorrection);
}

//刷新一次绘图状态
void BendingManagerWidget::OnDraw()
{
	////在3D视图中高亮显示选中的变形范围内的点
	//qDebug() << "OnDraw called";
	//const auto& item = ui->treeCorrection->currentItem();
	//qDebug() << "Current item:" << (item ? item->text(0) : "None");
	////取出当前correction的对象，取出他的作用域，调用绘图函数
	//if(ui->treeCorrection->currentItem() == nullptr)
	//	return;
	//const Correction& cor = m_model->getItems().at(item->data(0, Qt::UserRole).toInt());
	//Correction *ParentCorrection = cor.findParent();
	//PQUtils utils(m_ptrKit);
	//if (ParentCorrection) {
	//	std::array<edgePoint, 8> eqRange = m_manager->calEqRange(*ParentCorrection, cor);
	//	utils.drawBox(eqRange);
	//}
	//utils.drawBox(cor.m_range);
}

void BendingManagerWidget::closeEvent(QCloseEvent* event)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	event->ignore();
	this->hide();
}

void BendingManagerWidget::showEvent(QShowEvent* event)
{
	m_manager->initPathIDMap();
	m_manager->initIncludedPathForCorrections();
	initTreeWidget();
	ui->treeCorrection->blockSignals(false);
}

void BendingManagerWidget::initTreeWidget()
{
	ui->treeCorrection->clear();
	QStringList headerLabels;
	headerLabels << QString::fromLocal8Bit("修正名称")<< QString::fromLocal8Bit("作用于轨迹...");
	ui->treeCorrection->setHeaderLabels(headerLabels);
	QHeaderView* header = ui->treeCorrection->header();
	header->setSectionResizeMode(0, QHeaderView::Stretch); // 第一列自适应填充
	header->setSectionResizeMode(1, QHeaderView::Fixed);   // 第二列固定宽度
	ui->treeCorrection->setColumnWidth(1, 35);            // 第二列宽度可按需调小

	// 关键：第二列表头右对齐
	ui->treeCorrection->headerItem()->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);


	if (!m_model || m_model->rowCount() == 0)
		return;

	QVector<Correction>& items = m_model->getItems();

	// 建立 Correction* -> QTreeWidgetItem* 的映射，用于挂载子节点
	QMap<Correction*, QTreeWidgetItem*> itemMap;

	// 第一遍：创建所有顶层节点（parentCorrection == nullptr）
	for (int i = 0; i < items.size(); ++i)
	{
		Correction& cor = items[i];
		//顶层节点
		if (cor.m_parentCorrection == nullptr)
		{
			QTreeWidgetItem* treeItem = new QTreeWidgetItem(ui->treeCorrection);
			ui->treeCorrection->blockSignals(true);
			treeItem->setText(0, cor.name());
			treeItem->setCheckState(0, cor.isApplied() ? Qt::Checked : Qt::Unchecked);
			treeItem->setData(0, Qt::UserRole, i); // 存储模型行索引
			treeItem->setText(1, cor.m_includedPathStrList.join(","));
			ui->treeCorrection->blockSignals(true);
			itemMap.insert(&cor, treeItem);
		}
	}

	// 第二遍：创建所有子节点（parentCorrection != nullptr）
	for (int i = 0; i < items.size(); ++i)
	{
		Correction& cor = items[i];
		if (cor.m_parentCorrection != nullptr)
		{
			QTreeWidgetItem* parentItem = itemMap.value(cor.m_parentCorrection, nullptr);
			QTreeWidgetItem* treeItem = nullptr;
			if (parentItem)
			{
				treeItem = new QTreeWidgetItem(parentItem);
			}
			else
			{
				// 找不到父节点时作为顶层节点
				treeItem = new QTreeWidgetItem(ui->treeCorrection);
			}
			ui->treeCorrection->blockSignals(true);
			treeItem->setText(0, cor.name());
			treeItem->setCheckState(0, cor.isApplied() ? Qt::Checked : Qt::Unchecked);
			treeItem->setData(0, Qt::UserRole, i);
			treeItem->setText(1, cor.m_includedPathStrList.join(","));
			ui->treeCorrection->blockSignals(true);
			itemMap.insert(&cor, treeItem);
		}
	}

	ui->treeCorrection->expandAll();

	//设置逻辑：父节点未选中子节点不能被选中，子节点选中父节点自动选中。
}

void BendingManagerWidget::on_treeCorrection_itemChanged(QTreeWidgetItem* item, int column)
{
	if (column != 0 || !m_model)
		return;

	int row = item->data(0, Qt::UserRole).toInt();
	QVector<Correction>& items = m_model->getItems();
	if (row < 0 || row >= items.size())
		return;

	bool checked = (item->checkState(0) == Qt::Checked);
	//items[row].setIsApply(checked);	
	//emit correctionApplyStateChanged(items[row]);
	ui->treeCorrection->blockSignals(true);

	
	if (checked)//检查父节点
	{
		// 子节点选中时，自动选中父节点
		QTreeWidgetItem* parentItem = item->parent();
		if (parentItem)
		{
			int parentRow = parentItem->data(0, Qt::UserRole).toInt();
			if (parentRow >= 0 && parentRow < items.size())
			{
				if(!items[parentRow].isApplied())
				{
					items[parentRow].setIsApply(true);
					emit correctionApplyStateChanged(items[parentRow]);
					parentItem->setCheckState(0, Qt::Checked);
				}
			}
		}
		//再勾选本节点
		items[row].setIsApply(true);
		emit correctionApplyStateChanged(items[row]);
	}
	else//取消勾选，检查子节点
	{
		// 父节点取消选中时，取消所有子节点
		for (int i = 0; i < item->childCount(); ++i)
		{
			QTreeWidgetItem* child = item->child(i);
			int childRow = child->data(0, Qt::UserRole).toInt();
			if (childRow >= 0 && childRow < items.size())
			{
				if (items[childRow].isApplied())
				{
					items[childRow].setIsApply(false);
					emit correctionApplyStateChanged(items[childRow]);
					child->setCheckState(0, Qt::Unchecked);
				}
			}
		}
		items[row].setIsApply(false);
		emit correctionApplyStateChanged(items[row]);
	}

	ui->treeCorrection->blockSignals(false);
}

void BendingManagerWidget::on_treeCorrection_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
}

void BendingManagerWidget::applyBendingCorrection(Correction& cor)
{
	m_manager->rebuildPoints(cor);
}

void BendingManagerWidget::on_btnAppendLog_clicked()
{
	qDebug() << "\n\n\n\n";
}

void BendingManagerWidget::on_chkViewRange_stateChanged(int arg1)
{
	qDebug() << "View range checkbox state changed:" << arg1;
	if(arg1 == Qt::Checked)
	{
		// 开启拾取模式
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		m_ptrKit->Doc_start_module(cmd);
	}
	else
	{
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		m_ptrKit->Doc_end_module(cmd);
		qDebug() << "cancel on draw";
	}
}



void BendingManagerWidget::on_btnOK_clicked()
{
	BendingManager manager(m_ptrKit, m_model);
	std::vector<ULONG> listID;
	this->close();
}
