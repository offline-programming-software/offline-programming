#include "ui\BendingManagerWidget.h"
#include<qcheckbox.h>
#include<iostream>
#include<qdebug.h>
#include<utils/utils.h>
#include"RobxFileIO.h"
#include<qlayout.h>
#include<qlabel.h>
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
	connect(ui->testBtn1,&QPushButton::clicked,this,&BendingManagerWidget::testSlots);
	//initTree();
	//initStyle();
	setWindowTitle(QString::fromLocal8Bit("对象变形修正"));
	setWindowFlags(Qt::Window
		| Qt::WindowContextHelpButtonHint   // 问号
		| Qt::WindowCloseButtonHint);       // 关闭
	ui->listCorrections->setModel(m_model);

	setConnections();
}

BendingManagerWidget::~BendingManagerWidget()
{
	delete ui;
}


void BendingManagerWidget::initTree()
{
	ui->treeWidget->setColumnCount(3);
	BSTR sName;
	BSTR sIDs;
	QStringList trajList;
	m_ptrKit->pq_GetAllDataObjectsByType(PQ_PATH, &sName, &sIDs);
	trajList = utils::BSTR2QStringList(sName);
	int trajCount = 0;
	trajCount = trajList.count();
	QList<QTreeWidgetItem*> trajItemList;
	QList<QCheckBox*> chkList;
	QTreeWidgetItem *group = new QTreeWidgetItem(ui->treeWidget);
	group->setText(0, "group");
	QWidget *groupCheck = new QWidget(ui->treeWidget);
	QHBoxLayout *layout1 = new QHBoxLayout(groupCheck);
	QCheckBox *chk0 = new QCheckBox(ui->treeWidget);
	layout1->addWidget(chk0);
	layout1->setAlignment(Qt::AlignCenter);
	groupCheck->setLayout(layout1);
	ui->treeWidget->setItemWidget(group, 2, groupCheck);
	

	for (int i = 0; i < trajCount; i++)
	{
		QTreeWidgetItem *trajItem = new QTreeWidgetItem(group);
		QCheckBox *chkBox = new QCheckBox(ui->treeWidget);
		trajItem->setIcon(0, QIcon(":/Images/getpathpoint.png"));
		trajItem->setText(0, trajList.at(i));
		trajItemList.append(trajItem);
		chkList.append(chkBox);

		QWidget *widget = new QWidget(ui->treeWidget);
		QHBoxLayout *layout = new QHBoxLayout(widget);
		layout->addWidget(chkBox);
		layout->setAlignment(Qt::AlignCenter);     // 居中对齐（水平 + 垂直）
		layout->setContentsMargins(0, 0, 0, 0);    // 去除边距
		widget->setLayout(layout);
		ui->treeWidget->setItemWidget(trajItem, 2, widget);
	}
	ui->treeWidget->setIconSize(QSize(16, 16));
	QLabel *label = new QLabel();
	label->setText(QString::fromLocal8Bit("  correction1"));
	QLabel *label2 = new QLabel();
	label2->setText(QString::fromLocal8Bit("  correction1,correction2"));
	ui->treeWidget->setItemWidget(trajItemList[0], 1, label);
	ui->treeWidget->setItemWidget(trajItemList[1], 1, label2);

	QList<QString> heads;
	heads <<QString::fromLocal8Bit("轨迹") << QString::fromLocal8Bit("修正函数") << QString::fromLocal8Bit("是否修正");
	ui->treeWidget->setHeaderLabels(heads);
	ui->treeWidget->header()->setDefaultAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	
}

void BendingManagerWidget::initStyle()
{
	ui->treeWidget->setColumnWidth(0, 100); 
	ui->treeWidget->setColumnWidth(1, 250); 
	ui->treeWidget->setColumnWidth(2, 100);
	ui->treeWidget->setIndentation(8); //子节点缩进
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

void BendingManagerWidget::testSlots() {
	qDebug() << "test";
	QLabel *label = new QLabel();
	label->setText(QString::fromLocal8Bit("correction1"));
	QLabel *label2 = new QLabel();
	label2->setText(QString::fromLocal8Bit("correction1,correction2"));
}

void BendingManagerWidget::on_listCorrections_clicked(const QModelIndex& index)
{
	//当打开item被点击时：
	//获取当前选中的变形的range
	//在3D视图中高亮显示该范围内的点
	qDebug() << "clicked item row:" << index.row();

	// ? 保存当前选中行到成员变量
	m_currentDrawRow = index.row();

	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	m_ptrKit->Doc_start_module(cmd);
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
