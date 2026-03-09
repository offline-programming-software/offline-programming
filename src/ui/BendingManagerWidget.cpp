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

	//test 将一个修正对象假装成子对象
	auto& items = m_model->getItems();
	items[1].m_parentCorrection = &items[0];
	ui->treeCorrection->blockSignals(false);
	initTreeWidget();
	ui->treeCorrection->blockSignals(false);

}

BendingManagerWidget::~BendingManagerWidget()
{
	delete ui;
}



void BendingManagerWidget::setConnections()
{
	connect(m_ptrKitCallback, &CPQKitCallback::signalDraw, this, &BendingManagerWidget::OnDraw);
}

void BendingManagerWidget::OnDraw()
{
	//在3D视图中高亮显示选中的变形范围内的点
	qDebug() << "OnDraw called";

	if (!m_model || m_model->rowCount() == 0)
	{
		qDebug() << "Model is empty or invalid";
		return;
	}

	// ? 使用成员变量代替 currentIndex()
	if (m_currentDrawRow < 0 || m_currentDrawRow >= m_model->rowCount())
	{
		qDebug() << "No valid draw row:" << m_currentDrawRow;
		return;
	}

	int row = m_currentDrawRow;
	qDebug() << "Drawing for row:" << row;

	// 从模型获取修正范围数组
	std::array<double, 6> range = m_model->correctionRangeArray(row);

	double xMax = (std::max)(range[0], range[1]);
	double xMin = (std::min)(range[0], range[1]);
	double yMax = (std::max)(range[2], range[3]);
	double yMin = (std::min)(range[2], range[3]);
	double zMax = (std::max)(range[4], range[5]);
	double zMin = (std::min)(range[4], range[5]);

	// 检查范围是否有效
	bool isRangeValid = !(xMin == 0 && xMax == 0 && yMin == 0 && yMax == 0 && zMin == 0 && zMax == 0);
	if (!isRangeValid)
	{
		qDebug() << "Invalid range data for row:" << row;
		return;
	}

	double length = xMax - xMin;
	double width = yMax - yMin;
	double height = zMax - zMin;

	qDebug() << "Range - X:[" << xMin << "," << xMax << "] Y:[" << yMin << "," << yMax << "] Z:[" << zMin << "," << zMax << "]";

	// 计算点间距，用于动态调整点的密度
	double pointSpacing = 50.0;  // 点之间的均匀间距

	// 定义立方体的8个顶点
	std::vector<std::array<double, 3>> vertices =
	{
		{xMin, yMin, zMin}, // 0
		{xMax, yMin, zMin}, // 1
		{xMax, yMax, zMin}, // 2
		{xMin, yMax, zMin}, // 3
		{xMin, yMin, zMax}, // 4
		{xMax, yMin, zMax}, // 5
		{xMax, yMax, zMax}, // 6
		{xMin, yMax, zMax}  // 7
	};

	// 定义立方体的12条边
	std::vector<std::pair<int, int>> edges =
	{
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
		{4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
		{0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
	};

	// 绘制修正范围的立方体轮廓
	CComBSTR emptyText = L"";
	int pointCount = 0;
	for (const auto& edge : edges)
	{
		auto p1 = vertices[edge.first];
		auto p2 = vertices[edge.second];

		// 计算这条边的长度
		double edgeLength = std::sqrt(
			(p2[0] - p1[0]) * (p2[0] - p1[0]) +
			(p2[1] - p1[1]) * (p2[1] - p1[1]) +
			(p2[2] - p1[2]) * (p2[2] - p1[2])
		);

		// 根据边长动态计算分段数
		int dynamicSegments = std::max<int>(1, static_cast<int>(std::round(edgeLength / pointSpacing)));
		for (int i = 0; i <= dynamicSegments; ++i)
		{
			double t = static_cast<double>(i) / dynamicSegments;
			double dPos[3] =
			{
				(1 - t) * p1[0] + t * p2[0],
				(1 - t) * p1[1] + t * p2[1],
				(1 - t) * p1[2] + t * p2[2]
			};

			// 使用青色绘制立方体边框
			m_ptrKit->View_draw_point(dPos, 0, 3, RGB(0, 255, 255), emptyText, RGB(0, 255, 255));
			pointCount++;
		}
	}

	qDebug() << "Drew" << pointCount << "points for correction range at row:" << row;
}

void BendingManagerWidget::closeEvent(QCloseEvent* event)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
}

void BendingManagerWidget::initTreeWidget()
{
	ui->treeCorrection->clear();
	ui->treeCorrection->setHeaderLabel(QString::fromLocal8Bit("修正名称"));

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
	items[row].setIsApply(checked);

	ui->treeCorrection->blockSignals(true);

	if (checked)
	{
		// 子节点选中时，自动选中父节点
		QTreeWidgetItem* parentItem = item->parent();
		if (parentItem)
		{
			int parentRow = parentItem->data(0, Qt::UserRole).toInt();
			if (parentRow >= 0 && parentRow < items.size())
			{
				items[parentRow].setIsApply(true);
				parentItem->setCheckState(0, Qt::Checked);
			}
		}
	}
	else
	{
		// 父节点取消选中时，取消所有子节点
		for (int i = 0; i < item->childCount(); ++i)
		{
			QTreeWidgetItem* child = item->child(i);
			int childRow = child->data(0, Qt::UserRole).toInt();
			if (childRow >= 0 && childRow < items.size())
			{
				items[childRow].setIsApply(false);
				child->setCheckState(0, Qt::Unchecked);
			}
		}
	}

	ui->treeCorrection->blockSignals(false);
}

void BendingManagerWidget::on_treeCorrection_currentItemChanded(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
}


void BendingManagerWidget::on_btnOK_clicked()
{
	BendingManager manager(m_ptrKit, m_model);
	std::vector<ULONG> listID;
	manager.getAllPointInfo();
	/**
	 *     **执行弯曲变形应用**
	 * 1. √ 获取全部轨迹点（PQ接口）  
	 * 2. √ 将轨迹点分配给各个启用的变形修正函数（CorrectionModel）
	 * 3. √ 计算每个轨迹点的变形量
	 * 4. 将变形量应用到轨迹点上（PQ接口）
	 * m_bendingManager-> allocatePoints(index);
	 */
	this->close();
}
