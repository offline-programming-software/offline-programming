#include<iostream>
#include<qstringliteral.h>
#include"TrajCorrectDock.h"
#include<QMouseEvent>
#include<qdebug.h>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include<qscrollarea.h>
#include<qfiledialog.h>
#include<qmessagebox.h>
#include <sstream>   // 用于数字转字符串
#include <comdef.h>  // 包含 CComBSTR 所需头文件
#include <algorithm>
#include<qscrollbar.h>
#include "robxFileIO.h"

TrajCorrectDock::TrajCorrectDock(
    CComPtr<IPQPlatformComponent> ptrKit, 
	CPQKitCallback *ptrKitCallback, 
	CorrectionModel* model,
    QWidget *parent)

	: QDockWidget(u8"变形修正设置",parent), 
	  m_ptrKit(ptrKit),
	  m_ptrKitCallback2(ptrKitCallback), 
	  ui(new Ui::DockContent()),
	m_model(model)

{
	//初始化布局
	QVBoxLayout *layoutForScroll = new QVBoxLayout();
	layoutForScroll->setContentsMargins(0, 0, 0, 0);
	QWidget *contentForScroll = new QWidget(this);
	contentForScroll->setLayout(layoutForScroll);
	scroll = new QScrollArea();
	scroll->setFixedWidth(350);
	layoutForScroll->addWidget(scroll);
	QWidget *contentWidget = new QWidget();
	ui->setupUi(contentWidget);
	contentWidget->setFixedWidth(800);
	scroll->setWidget(contentWidget);
	setWidget(contentForScroll);
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);

	//初始化dock
	initDock(contentWidget);
	//初始化一些自定义控件
	InitCustomWidget();
	//信号槽链接
	setupConnections();
	
	//设置样式表格式
	listFlagPoints->setStyleSheet("border: 1px solid black");
	QFont font("SimSun", 10);
	ui->groupBox->setFont(font);  // 设置
	ui->groupBox_2->setFont(font);
	ui->spnDeg->setSingleStep(0.05);
	ui->spnMaxDeflection->setDisabled(true);
	
	//从json读入数据，初始化数据列表, 初始化变形列表控件
	m_correctionList = m_model->corrections();
	for (const Correction& corr : m_correctionList)
	{
		QListWidgetItem* item = new QListWidgetItem(corr.name());
		m_correctionItems.push_back(item);
	}
	InitLists();

	setWindowTitle(QString::fromLocal8Bit("变形修正设置"));
	
	//增加一些帮助
	QPixmap pixmap(":/image/resource/QuestionMark.png");  
	ui->label_Question->setFixedSize(20, 20);  // 设置标签大小
	ui->label_Question->setPixmap(pixmap);  // 设置图片
	ui->label_Question->setAlignment(Qt::AlignCenter);  // 图片居中显示
	ui->label_Question->setScaledContents(true);

	//设置提示信息：鼠标悬停在图片上时显示提示文本
	ui->label_Question->setToolTip(QString::fromLocal8Bit
	(
		"导入文件需求：\n"
		"格式：.csv"
		"文件内容：每行一个点，X Y, Z坐标用逗号分隔\n"
		"例如：\n"
		"1.000000, 2.000000, 3.000000\n"
		"4.000000, 5.000000, 6.000000\n"
		"7.000000, 8.000000, 9.000000\n"
	));


}

TrajCorrectDock::~TrajCorrectDock()
{
	delete ui;
}



void TrajCorrectDock::importCsvPointsMeasure(const QString & filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开文件:" << filePath;
		return;
	}
	m_vMeasurePoints.clear();
	
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) continue;

		QStringList parts = line.split(QRegExp("[,;\\s]"), QString::SkipEmptyParts);
		if (parts.size() < 3) continue;

		bool ok1, ok2, ok3;
		double x = parts[0].toDouble(&ok1);
		double y = parts[1].toDouble(&ok2);
		double z = parts[2].toDouble(&ok3);

		if (ok1 && ok2 && ok3) {
			m_vMeasurePoints.push_back(x);
			m_vMeasurePoints.push_back(y);
			m_vMeasurePoints.push_back(z);
		}
	}

	file.close();

	qDebug() << "成功导入点数量：" << m_vMeasurePoints.size() / 3;
}

void TrajCorrectDock::exportCsvFlagPoints(const QString & filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "无法写入文件:" << filePath;
		return;
	}

	QTextStream out(&file);
	out.setRealNumberNotation(QTextStream::FixedNotation);
	out.setRealNumberPrecision(6);

	for (int i = 0; i + 2 < m_vFlagPoints.size(); i += 3) {
		out << m_vFlagPoints[i] << ", "
			<< m_vFlagPoints[i + 1] << ", "
			<< m_vFlagPoints[i + 2] << "\n";
	}

	file.close();
	qDebug() << "导出成功:" << filePath;
}

void TrajCorrectDock::exportCsvMeasurePoints(const QString & filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "无法写入文件:" << filePath;
		return;
	}
	
	QTextStream out(&file);
	out.setRealNumberNotation(QTextStream::FixedNotation);
	out.setRealNumberPrecision(6);

	for (int i = 0; i + 2 < m_vMeasurePoints.size(); i += 3) {
		out << m_vMeasurePoints[i] << ", "
			<< m_vMeasurePoints[i + 1] << ", "
			<< m_vMeasurePoints[i + 2] << "\n";
	}

	file.close();
	qDebug() << "导出成功:" << filePath;
}





void TrajCorrectDock::testSignal(int num)
{
	std::cout << "SignalEmit" << std::endl;
	std::cout << "num = "<<num << std::endl;

}

void TrajCorrectDock::mousePressEvent(QMouseEvent * event)
{
	QWidget *clickedChild = childAt(event->pos());

	// 如果点击的控件，不是 pickBox，也不是它的子控件
	if (!clickedChild || (clickedChild != listFlagPoints && !listFlagPoints->isAncestorOf(clickedChild))) {
		emit blankAreaClicked();
	}
	else {
		// 点到 pickBox 或其子控件了，正常传递
		QWidget::mousePressEvent(event);
	}
}




#pragma region InitializationFunctions
void TrajCorrectDock::initDock(QWidget* contentWidget)
{
	listFlagPoints = new pickWidget(contentWidget);  //pickbox的父对象被设置为contentWidget，在QT中的父子关系负责内存管理，删除父对象时，它会自动删除所有子对象（不需要你手动 delete）
	xMinspin = new PickSpinBox(contentWidget);
	xMaxspin = new PickSpinBox(contentWidget);
	yMinspin = new PickSpinBox(contentWidget);
	yMaxspin = new PickSpinBox(contentWidget);
	zMinspin = new PickSpinBox(contentWidget);
	zMaxspin = new PickSpinBox(contentWidget);
	spinBoxes = { xMinspin, xMaxspin, yMinspin, yMaxspin, zMinspin, zMaxspin };
	spnOriginx = new PickSpinBox(contentWidget);
	spnOriginy = new PickSpinBox(contentWidget);
	spnOriginz = new PickSpinBox(contentWidget);
	spnOriginx->setSingleStep(10.0);
	spnOriginy->setSingleStep(10.0);
	spnOriginz->setSingleStep(10.0);
	for(PickSpinBox* box : spinBoxes)
	{
		box->setSingleStep(10.0);
	}
	ui->loutOriginPt->addWidget(spnOriginx);
	ui->loutOriginPt->addWidget(spnOriginy);
	ui->loutOriginPt->addWidget(spnOriginz);
	ui->btnDelCor->setEnabled(false);
	ui->groupBox_2->setEnabled(false);
	ui->verticalLayout->addWidget(listFlagPoints);
	for (int i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
}

void TrajCorrectDock::InitLists()
{
	// 1?? 先清空列表控件
	ui->listCorrections->clear();

	//2
	for(QListWidgetItem *item: m_correctionItems)
	{
		ui->listCorrections->addItem(item);
	}

	//默认选中第一个
	if (ui->listCorrections->count() > 0)
		ui->listCorrections->setCurrentRow(0);
	setView();
}

void TrajCorrectDock::initPointLists()
{
	//#重写
}

void TrajCorrectDock::initGroupEmpty(const QString name) {
	ui->editCorName->setText(name);
	ui->chkApplyCor->setChecked(false);
	for (PickSpinBox* p : spinBoxes) {
		p->setValue(0);
	}
	spnOriginx->setValue(0);
	spnOriginy->setValue(0);
	spnOriginz->setValue(0);
	ui->spnDeg->setValue(0);
	listFlagPoints->clear();
	ui->listMeasurePoints->clear();
	ui->cmbFittingType->setCurrentIndex(0);

}
void TrajCorrectDock::InitCustomWidget()
{
	ui->gridLayout->addWidget(xMinspin,0,0);
	ui->gridLayout->addWidget(xMaxspin,0,1);
	ui->gridLayout->addWidget(yMinspin,1,0);
	ui->gridLayout->addWidget(yMaxspin,1,1);
	ui->gridLayout->addWidget(zMinspin,2,0);
	ui->gridLayout->addWidget(zMaxspin,2,1);
	for (int i = 0; i < 6; i++) {
		spinBoxes[i]->setRange(-100000,110000);
	}
	spnOriginx->setRange(-100000, 110000);
	spnOriginy->setRange(-100000, 110000);
	spnOriginz->setRange(-100000, 110000);
}








void TrajCorrectDock::setupConnections()
{
	//点击事件
	connect(ui->btnNewCorrection, SIGNAL(clicked()), this, SLOT(on_btnNewCorrection_clicked()));
	connect(ui->btnMeasurePtsInport, &QPushButton::clicked, this, &TrajCorrectDock::on_btnImportMeasurePoints_clicked);
	connect(ui->btnImportFlag, &QPushButton::clicked, this, &TrajCorrectDock::on_btnImportFlag_clicked);
	connect(ui->btnFlagPointsExport, &QPushButton::clicked, this, &TrajCorrectDock::on_btnExportFlagPoints_clicked);
	connect(ui->btnAttributeSetOK, &QPushButton::clicked, this, &TrajCorrectDock::on_btnAttributeSetOK_clicked);
	connect(ui->btnRefresh, &QPushButton::clicked, this, &TrajCorrectDock::on_btnRefresh_clicked);
	connect(ui->listCorrections, &QListWidget::itemClicked, this, &TrajCorrectDock::on_listCorrection_slectedItem);
	connect(ui->btnSave, &QPushButton::clicked, this, &TrajCorrectDock::on_btnSave_clicked);
	connect(listFlagPoints, &pickWidget::blankAreaClicked, this, &TrajCorrectDock::on_pickBox_blankAreaClicked);
	connect(listFlagPoints, &pickWidget::deleteSignal, this, &TrajCorrectDock::testSignal);
	connect(this, &TrajCorrectDock::blankAreaClicked, this, &TrajCorrectDock::on_this_blankAreaClicked);
	connect(ui->btnDelCor, SIGNAL(clicked()), this, SLOT(on_btnDeleteCorrection_clicked()));
	
	connect(ui->cmbBeamDir, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &TrajCorrectDock::on_cmbBeamDir_currentIndexChanged);
	connect(spnOriginx, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	connect(spnOriginy, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	connect(spnOriginz, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	//tap页切换事件
	connect(ui->tabInput, &QTabWidget::currentChanged, this, &TrajCorrectDock::on_tabInput_currentChanged);

	//触发setEdit槽函数，当编辑名称时
	connect(ui->editCorName, &QLineEdit::textEdited, this, &TrajCorrectDock::setEdit);
	for (PickSpinBox* box : spinBoxes)
		connect(box, QOverload<int>::of(&QSpinBox::valueChanged), this, &TrajCorrectDock::setEdit);
	connect(ui->chkIsPosCorrect, &QCheckBox::stateChanged, this, &TrajCorrectDock::setEdit);
	connect(ui->chkViewPoint2Correct, &QCheckBox::stateChanged, this, &TrajCorrectDock::setEdit);
	connect(ui->cmbFittingType, QOverload<int>::of(&QComboBox::currentIndexChanged) , this, &TrajCorrectDock::setEdit);
	connect(ui->listMeasurePoints, &QListWidget::itemChanged, this, &TrajCorrectDock::setEdit);
	connect(listFlagPoints, &QListWidget::itemChanged, this, &TrajCorrectDock::setEdit);
	connect(ui->btnRefreshLog, &QPushButton::clicked, this, &TrajCorrectDock::on_btnRefreshLog_clicked);

	for (int i = 0; i < 6; i++)
	{
		connect(spinBoxes[i], &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
		connect(spinBoxes[i], QOverload<int>::of(&PickSpinBox::valueChanged),
			this, &TrajCorrectDock::onPickSpinBoxValueChanged);
	}
	connect(spnOriginx, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(spnOriginy, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(spnOriginz, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(ui->spnDeg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TrajCorrectDock::on_spnBeamDeg_valueChanged);
	//绘图
	connect(ui->listMeasurePoints, &QListWidget::itemClicked, this, &TrajCorrectDock::on_listMeasurePoints_itemClicked);
	connect(listFlagPoints, &QListWidget::itemClicked, this, &TrajCorrectDock::on_listFlagPoints_itemClicked);

	//devPage
	testConnection();
	//connect(ui->btnDevPage, &QPushButton::clicked, this, &TrajCorrectDock::on_btnDevPage_clicked);
	connect(ui->btnCal, &QPushButton::clicked, this, &TrajCorrectDock::on_btnCal_clicked);
	
	//---------------pq回调信号--------------------------
	connect(m_ptrKitCallback2, &CPQKitCallback::signalElementPickup, this, &TrajCorrectDock::OnPickup);
	//connect(m_ptrKitCallback2, &CPQKitCallback::signalElementSelection, this, &TrajCorrectDock::OnElementSelection);
	connect(m_ptrKitCallback2, &CPQKitCallback::signalDraw, this, &TrajCorrectDock::OnDraw);
}

#pragma endregion regionName

#pragma region 拾取相关函数


void TrajCorrectDock::OnDraw()
{
	if (!this->isVisible()) return;
	CComBSTR strText = "PtText";
	CComBSTR bstr = "Origin";
	CComBSTR emptyText = " ";
	qDebug() << "DrawSignal emited ,It's OnDraw From TrajCorrectDock";
	static double dPos[3] = { 0.0 };
	static int counter = 0;
	double dPos0[3] = { 0,0,0 };
	std::wstringstream wss;
	//----------------------------------
	bool isFull = true;
	double xMax, xMin, yMax, yMin, zMax, zMin = 0;
	double lengh, width, height;
	switch (m_drawSource)
	{

	case TrajCorrectDock::DrawSource::None:
		break;

	case TrajCorrectDock::DrawSource::FromFlagPoints:
	{
		int CurrentPointIndex = listFlagPoints->currentRow();
		for (int i = 0; i < m_vFlagPoints.size(); i += 3)
		{
			// 确保有完整的三个坐标
			if (i + 2 >= m_vFlagPoints.size()) break;

			double currentPoint[3] = {
				m_vFlagPoints[i],
				m_vFlagPoints[i + 1],
				m_vFlagPoints[i + 2]
			};

			// 获取点索引
			int pointIndex = i / 3;

			// 绘制蓝色点（所有点）
			wss.str(L"");
			wss.clear();
			wss << L"点" << pointIndex + 1;
			strText = wss.str().c_str();

			// 如果是当前选中点，绘制绿色大点高亮
			if (pointIndex == CurrentPointIndex)
			{
				m_ptrKit->View_draw_point(currentPoint, 0, 30, RGB(0, 255, 0), strText, RGB(0, 255, 0));
			}
			m_ptrKit->View_draw_point(currentPoint, 0, 15, RGB(10, 100, 200), strText, RGB(255, 140, 0));
		}
		break;
	}

	case TrajCorrectDock::DrawSource::FromMeasurePoints:
	{
		int CurrentPointIndex = ui->listMeasurePoints->currentRow();
		for (int i = 0; i < m_vMeasurePoints.size(); i += 3)
		{
			// 确保有完整的三个坐标
			if (i + 2 >= m_vMeasurePoints.size()) break;

			double currentPoint[3] = {
				m_vMeasurePoints[i],
				m_vMeasurePoints[i + 1],
				m_vMeasurePoints[i + 2]
			};

			// 获取点索引
			int pointIndex = i / 3;

			// 绘制蓝色点（所有点）
			wss.str(L"");
			wss.clear();
			wss << L"点" << pointIndex + 1;
			strText = wss.str().c_str();
			// 如果是当前选中点，绘制绿色大点高亮
			if (pointIndex == CurrentPointIndex)
			{
				m_ptrKit->View_draw_point(currentPoint, 0, 30, RGB(0, 255, 0), strText, RGB(0, 255, 0));
			}
			m_ptrKit->View_draw_point(currentPoint, 0, 15, RGB(217, 83, 79), strText, RGB(255, 140, 0));

		}
		break;
	}
		

	case TrajCorrectDock::DrawSource::FromOriginPoint:
	{
		dPos0[0] = spnOriginx->value();
		dPos0[1] = spnOriginy->value();
		dPos0[2] = spnOriginz->value();

		// ? 原点：红色大点 + "Origin" 标注
		m_ptrKit->View_draw_point(dPos0, 0, 20, RGB(255, 0, 0), bstr, RGB(255, 0, 0));

		// ? 获取方向
		int directionIndex = ui->cmbBeamDir->currentIndex();
		std::vector<double> direction(3, 0.0);

		switch (directionIndex) {
		case 0: direction = { 1.0, 0.0, 0.0 };  break;
		case 1: direction = { -1.0, 0.0, 0.0 }; break;
		case 2: direction = { 0.0, 1.0, 0.0 };  break;
		case 3: direction = { 0.0, -1.0, 0.0 }; break;
		default: direction = { 1.0, 0.0, 0.0 }; break;
		}

		double arrowLength = 500.0;
		double arrowHeadLength = 100.0;
		double arrowHeadWidth = 60.0;

		// ? 箭头末端
		double arrowEnd[3] = {
			dPos0[0] + direction[0] * arrowLength,
			dPos0[1] + direction[1] * arrowLength,
			dPos0[2] + direction[2] * arrowLength
		};

		// ? 绘制箭头主体（红色线）
		int lineSegments = 30;
		for (int i = 1; i <= lineSegments; ++i) {
			double t = static_cast<double>(i) / lineSegments;
			double pointPos[3] = {
				dPos0[0] + direction[0] * arrowLength * t,
				dPos0[1] + direction[1] * arrowLength * t,
				dPos0[2] + direction[2] * arrowLength * t
			};
			m_ptrKit->View_draw_point(pointPos, 0, 6, RGB(255, 0, 0), emptyText, RGB(255, 0, 0));
		}

		// ? 计算两个垂直向量 - 改进算法
		std::vector<double> perp1(3, 0.0), perp2(3, 0.0);

		// 找一个与 direction 不平行的向量
		std::vector<double> helper(3, 0.0);
		if (std::abs(direction[0]) < 0.9) {
			helper = { 1.0, 0.0, 0.0 };  // 如果 direction 不主要沿 X，用 X 轴
		}
		else {
			helper = { 0.0, 1.0, 0.0 };  // 否则用 Y 轴
		}

		// perp1 = direction × helper（叉积）
		perp1[0] = direction[1] * helper[2] - direction[2] * helper[1];
		perp1[1] = direction[2] * helper[0] - direction[0] * helper[2];
		perp1[2] = direction[0] * helper[1] - direction[1] * helper[0];

		// 归一化 perp1
		double perp1_len = std::sqrt(perp1[0] * perp1[0] + perp1[1] * perp1[1] + perp1[2] * perp1[2]);
		if (perp1_len > 1e-6) {
			perp1[0] /= perp1_len;
			perp1[1] /= perp1_len;
			perp1[2] /= perp1_len;
		}

		// perp2 = direction × perp1（叉积）
		perp2[0] = direction[1] * perp1[2] - direction[2] * perp1[1];
		perp2[1] = direction[2] * perp1[0] - direction[0] * perp1[2];
		perp2[2] = direction[0] * perp1[1] - direction[1] * perp1[0];

		// 归一化 perp2
		double perp2_len = std::sqrt(perp2[0] * perp2[0] + perp2[1] * perp2[1] + perp2[2] * perp2[2]);
		if (perp2_len > 1e-6) {
			perp2[0] /= perp2_len;
			perp2[1] /= perp2_len;
			perp2[2] /= perp2_len;
		}

		// ? 圆锥箭头头部
		double arrowBase[3] = {
			arrowEnd[0] - direction[0] * arrowHeadLength,
			arrowEnd[1] - direction[1] * arrowHeadLength,
			arrowEnd[2] - direction[2] * arrowHeadLength
		};

		int coneSegments = 12;
		int coneHeightSegments = 8;

		// 绘制圆锥表面
		for (int i = 0; i < coneSegments; ++i) {
			double angle1 = 2.0 * 3.14159265359 * i / coneSegments;
			double angle2 = 2.0 * 3.14159265359 * (i + 1) / coneSegments;

			double basePoint1[3] = {
				arrowBase[0] + perp1[0] * arrowHeadWidth * std::cos(angle1) + perp2[0] * arrowHeadWidth * std::sin(angle1),
				arrowBase[1] + perp1[1] * arrowHeadWidth * std::cos(angle1) + perp2[1] * arrowHeadWidth * std::sin(angle1),
				arrowBase[2] + perp1[2] * arrowHeadWidth * std::cos(angle1) + perp2[2] * arrowHeadWidth * std::sin(angle1)
			};

			double basePoint2[3] = {
				arrowBase[0] + perp1[0] * arrowHeadWidth * std::cos(angle2) + perp2[0] * arrowHeadWidth * std::sin(angle2),
				arrowBase[1] + perp1[1] * arrowHeadWidth * std::cos(angle2) + perp2[1] * arrowHeadWidth * std::sin(angle2),
				arrowBase[2] + perp1[2] * arrowHeadWidth * std::cos(angle2) + perp2[2] * arrowHeadWidth * std::sin(angle2)
			};

			for (int h = 0; h <= coneHeightSegments; ++h) {
				double t = static_cast<double>(h) / coneHeightSegments;

				double point1[3] = {
					basePoint1[0] * (1 - t) + arrowEnd[0] * t,
					basePoint1[1] * (1 - t) + arrowEnd[1] * t,
					basePoint1[2] * (1 - t) + arrowEnd[2] * t
				};

				double point2[3] = {
					basePoint2[0] * (1 - t) + arrowEnd[0] * t,
					basePoint2[1] * (1 - t) + arrowEnd[1] * t,
					basePoint2[2] * (1 - t) + arrowEnd[2] * t
				};

				m_ptrKit->View_draw_point(point1, 0, 6, RGB(255, 0, 0), emptyText, RGB(255, 0, 0));
				m_ptrKit->View_draw_point(point2, 0, 6, RGB(255, 0, 0), emptyText, RGB(255, 0, 0));
			}
		}

		// 绘制圆锥底部
		for (int i = 0; i < coneSegments; ++i) {
			for (int j = 1; j < coneSegments; ++j) {
				double angle = 2.0 * 3.14159265359 * i / coneSegments;
				double radius = arrowHeadWidth * j / coneSegments;

				double point[3] = {
					arrowBase[0] + perp1[0] * radius * std::cos(angle) + perp2[0] * radius * std::sin(angle),
					arrowBase[1] + perp1[1] * radius * std::cos(angle) + perp2[1] * radius * std::sin(angle),
					arrowBase[2] + perp1[2] * radius * std::cos(angle) + perp2[2] * radius * std::sin(angle)
				};

				m_ptrKit->View_draw_point(point, 0, 5, RGB(255, 0, 0), emptyText, RGB(255, 0, 0));
			}
		}

		break;
	}
				
	case TrajCorrectDock::DrawSource::FromRangePoints:
	{
		for (int i = 0; i < 6; i++)
		{
			if (spinBoxes[i]->value() == 0)
			{
				isFull = false;
				break;
			}
		}
		if (isFull)
		{
			xMax = (std::max)(xMaxspin->value(), xMinspin->value());
			xMin = (std::min)(xMaxspin->value(), xMinspin->value());
			yMax = (std::max)(yMaxspin->value(), yMinspin->value());
			yMin = (std::min)(yMaxspin->value(), yMinspin->value());
			zMax = (std::max)(zMaxspin->value(), zMinspin->value());
			zMin = (std::min)(zMaxspin->value(), zMinspin->value());
			lengh = xMax - xMin;
			width = yMax - yMin;
			height = zMax - zMin;

			// ? 计算最大边长，用于确定点密度
			double maxEdgeLength = (std::max)({ lengh, width, height });
			double pointSpacing = 50.0;  // 点之间的均匀间距（可调整）

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

			std::vector<std::pair<int, int>> edges =
			{
				{0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
				{4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
				{0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
			};

			for (const auto& edge : edges)
			{
				auto p1 = vertices[edge.first];
				auto p2 = vertices[edge.second];

				// ? 计算这条边的长度
				double edgeLength = std::sqrt(
					(p2[0] - p1[0]) * (p2[0] - p1[0]) +
					(p2[1] - p1[1]) * (p2[1] - p1[1]) +
					(p2[2] - p1[2]) * (p2[2] - p1[2])
				);

				// ? 根据边长动态计算分段数
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

					CComBSTR emptyText = L"";
					m_ptrKit->View_draw_point(dPos, 0, 3, RGB(0, 255, 255), emptyText, RGB(0, 255, 255)); // 青色细点
				}
			}
		}
		break;
	}
	
	}
}





void TrajCorrectDock::OnPickup(unsigned long i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType, double i_dPointX, double i_dPointY, double i_dPointZ)
{
	if (!this->isVisible()) return;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	int myPointCounter = 0 ;
	bool isFull = true;
	switch (m_pickSource)
	{
	case TrajCorrectDock::PickSource::None:
		break;
	
	case TrajCorrectDock::PickSource::FromListFlags:
		m_vFlagPoints.push_back(i_dPointX);
		m_vFlagPoints.push_back(i_dPointY);
		m_vFlagPoints.push_back(i_dPointZ);
		myPointCounter = m_vFlagPoints.size() / 3;
		listFlagPoints->clear();
		for (int i = 0; i < myPointCounter; i++)
		{
			listFlagPoints->addItem(QStringLiteral("点%1").arg(i+1));
		}
		
		break;
	case TrajCorrectDock::PickSource::FromOriginPoint:
		spnOriginx->setValue(i_dPointX);
		spnOriginy->setValue(i_dPointY);
		spnOriginz->setValue(i_dPointZ);
		OnDraw();
		
		break;
	case TrajCorrectDock::PickSource::FromSpinRanges:
		qDebug() << "It's OnElementPickup from class TrajCorrectDock";
		pick:
		switch (rangeBoxIndex)
		{
		case 0:
			xMinspin->setValue(i_dPointX);
			break;
		case 1:
			xMaxspin->setValue(i_dPointX);
			break;
		case 2:
			yMinspin->setValue(i_dPointY);
			break;
		case 3:
			yMaxspin->setValue(i_dPointY);
			break;
		case 4:
			zMinspin->setValue(i_dPointZ);
			break;
		case 5:
			zMaxspin->setValue(i_dPointZ);
			break;
		}
		spinBoxes[rangeBoxIndex]->setStyleSheet("PickSpinBox{border: 1px solid black}");
		while(rangeBoxIndex<5)
		{
			rangeBoxIndex++;
			spinBoxes[rangeBoxIndex]->setStyleSheet("border: 3px solid blue");
			return;  //退出函数，等待下一次拾取
		}
		if (rangeBoxIndex == 5)
		{
			spinBoxes[rangeBoxIndex]->setStyleSheet("border: 3px solid blue");
			return;
		}
		//m_ptrKit->Doc_end_module(cmd);

		//OnDraw();

		break;
	
		

		
	default:
		break;
	}
	
}

void TrajCorrectDock::on_pickBox_blankAreaClicked()
{
	//#拾取相关
	//当点击pickBox空白区域时，开始标志点拾取模式
	if(m_drawSource == DrawSource::FromFlagPoints)
		return;
	m_pickSource = PickSource::FromListFlags;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	listFlagPoints->setStyleSheet("border: 3px solid blue");
	m_drawSource = DrawSource::FromFlagPoints;
}

void TrajCorrectDock::on_listMeasurePoints_itemClicked(QListWidgetItem* item)
{
	//当点击measurePoints中的某一项时，进入标记点拾取状态
	m_drawSource = DrawSource::FromMeasurePoints;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	OnDraw();
}

void TrajCorrectDock::on_listFlagPoints_itemClicked(QListWidgetItem* item)
{
	//当点击listFlagPoints中的某一项时，进入标记点拾取状态
	m_drawSource = DrawSource::FromFlagPoints;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	OnDraw();
}

void TrajCorrectDock::pickRange()
{
	//#拾取相关：
	//点击范围SpinBox时，进入拾取状态，从轨迹中拾取范围值
	//if(m_pickSource == PickSource::FromSpinRanges &&
	//	m_drawSource == DrawSource::FromRangePoints)
	//{
	//	//已经在拾取状态，避免重复触发
	//	return;
	//}
	m_pickSource = PickSource::FromSpinRanges;
	m_drawSource = DrawSource::FromRangePoints;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	OnDraw();
	for (int i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
	QObject *obj = sender();    //槽函数专用，获取触发这个槽函数的对象指针
	for (int i = 0; i < spinBoxes.size(); ++i) {
		if (obj == spinBoxes[i]) {
			spinBoxes[i]->setStyleSheet("border: 3px solid blue");
			rangeBoxIndex = i;
			qDebug() << "current pick:" << rangeBoxIndex << "SpinBox";
			break;
		}
	}
}


void TrajCorrectDock::pickOriginPoint()
{
	
	m_pickSource = PickSource::FromOriginPoint;
	m_drawSource = DrawSource::FromOriginPoint;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	OnDraw();
	//视觉提示
	spnOriginx->setStyleSheet("PickSpinBox{border: 3px solid blue}");
	spnOriginy->setStyleSheet("PickSpinBox{border: 3px solid blue}");
	spnOriginz->setStyleSheet("PickSpinBox{border: 3px solid blue}");
}

void TrajCorrectDock::on_cmbBeamDir_currentIndexChanged(int index)
{
	OnDraw();
}

void TrajCorrectDock::on_this_blankAreaClicked()
{
    //#拾取相关：
	//点击dock空白区域，结束拾取状态
	if(m_pickSource == PickSource::None &&
		m_drawSource == DrawSource::None)
	{
		//不在拾取状态，避免重复触发
		return;
	}
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	listFlagPoints->setStyleSheet("border: 1px solid black");
	for (int i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
	m_drawSource = DrawSource::None;
	spnOriginx->setStyleSheet("PickSpinBox{border: 1px solid black}");
	spnOriginy->setStyleSheet("PickSpinBox{border: 1px solid black}");
	spnOriginz->setStyleSheet("PickSpinBox{border: 1px solid black}");
}


void TrajCorrectDock::onPickSpinBoxValueChanged(int a)
{
	//CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	//m_ptrKit->Doc_end_module(cmd);
	//m_ptrKit->Doc_start_module(cmd);
	OnDraw();
}

void TrajCorrectDock::on_spnOrigin_valueChanged(int a)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	m_ptrKit->Doc_start_module(cmd);
}


void TrajCorrectDock::on_spnBeamDeg_valueChanged(double a)
{
	//根据范围的最大值，计算下垂量
	double maxRange = (std::max)({
		abs(xMaxspin->value()),
		abs(xMinspin->value()),
		abs(yMaxspin->value()),
		abs(yMinspin->value()),
		abs(zMaxspin->value()),
		abs(zMinspin->value())
		});
	double deflection = tan(a * 3.1415926 / 180) * maxRange;
	ui->spnMaxDeflection->setValue(deflection);
}

#pragma endregion 拾取相关函数







#pragma region SLOTSs






void TrajCorrectDock::OnElementSelection(LPWSTR i_wObjNames, LPWSTR i_wFaceNames, double * i_dPointXYZ, int i_nSize)
{
	qDebug() << "It's OnElementSelection from class TrajCorrectDock";

}

void TrajCorrectDock::setView()
{
	//触发条件：
	// 1.当listCorrection的currentItemChanged信号触发时
	// 2.当btnAttributeSetOK按钮点击后
	// 表现为itemtext后面无*
	QListWidgetItem* item = ui->listCorrections->currentItem();
	if (!item)
	{
		return;
	}
	auto state = static_cast<ItemState>(item->data(Qt::UserRole).toInt());
	if (state == ItemState::View)
		return;
	item->setData(Qt::UserRole, static_cast<int>(ItemState::View));
	//去掉item后面的*
	QString itemText = item->text();
	itemText = itemText.replace("*", "");
	item->setText(itemText);
}

void TrajCorrectDock::on_tabInput_currentChanged(int index)
{
	int grpHeight = ui->groupBox_2->height();
	int myIndex = index;
	if (myIndex == 1)
	{
		//ui->tabInput->setFixedHeight(100);
		ui->groupBox_2->setFixedHeight(grpHeight + 160);
		//ui->btnSave的位置也要调整，
		int btnSaveY = ui->groupBox_2->pos().y() + ui->groupBox_2->height() + 10;  // 增加10像素间距
		ui->btnSave->move(ui->btnSave->pos().x(), btnSaveY);
	}
	else
	{
		ui->groupBox_2->setFixedHeight(grpHeight - 160);
		// 恢复 btnSave 的位置
		int btnSaveY = ui->groupBox_2->pos().y() + ui->groupBox_2->height() + 10;
		ui->btnSave->move(ui->btnSave->pos().x(), btnSaveY);
	}
		//ui->tabInput->setFixedHeight(260);
}

void TrajCorrectDock::setEdit()
{

	//groupbox_2控件内容改变时，当前item触发编辑模式：表现为itemtext后面加*
	//btnAttributeSetOK按钮可用
	//当listCorrection的itemChanged信号触发时，询问是否放弃更改
	QListWidgetItem* item = ui->listCorrections->currentItem();
	auto state  = static_cast<ItemState>(item->data(Qt::UserRole).toInt());
	if(state == ItemState::Edit)
		return;
	item->setData(Qt::UserRole, static_cast<int>(ItemState::Edit));
	item->setText(item->text() + "*");
	ui->btnAttributeSetOK->setEnabled(true);

}

void TrajCorrectDock::on_btnNewCorrection_clicked()
{
	//btnNewCorrection按钮点击后
	//1. 创建一个新的Correction对象，添加到m_correctionList
	//2. 在listCorrections中添加一个新的QListWidgetItem，设置状态为Edit
	//3. 遍历listCorrections，确保新项名称唯一
	//4. 启用groupBox_2，清空属性定义控件

	Correction newCor;
	QListWidgetItem* item = new QListWidgetItem(ui->listCorrections);
	item->setData(Qt::UserRole, static_cast<int>(ItemState::View));
	

	m_correctionItems.append(item);
	ui->btnNewCorrection->setEnabled(false);
	QString itemName = QString::fromLocal8Bit("新建修正对象");
	int suffix = 1;
	for (QListWidgetItem* it : m_correctionItems) {
		if (itemName == it->text()) {
			itemName = QString::fromLocal8Bit("新建修正对象%1").arg(suffix++);
		}
	}
	item->setText(itemName);
	newCor.setName(itemName);
	
	ui->groupBox_2->setEnabled(true);	
	ui->listCorrections->setCurrentItem(item);
	initGroupEmpty(itemName);
	setView();
	m_correctionList.append(newCor);
	ui->editCorName->setFocus();                    // 设置焦点到 editCorName
	ui->editCorName->selectAll();
	scroll->verticalScrollBar()->setValue(0);
	scroll->horizontalScrollBar()->setValue(0);

	

	
}

void TrajCorrectDock::on_btnDeleteCorrection_clicked()
{
	int row = ui->listCorrections->currentRow();
	m_correctionList.remove(row);
	m_correctionItems.removeAt(row);
	ui->listCorrections->takeItem(row);
	ui->edtLog->appendPlainText(QStringLiteral(">>删除修正对象，当前的item数目：%1").arg(m_correctionItems.count()));
	ui->edtLog->appendPlainText(QStringLiteral(">>当前的correction数目：%1").arg(m_correctionList.count()));

}

void TrajCorrectDock::on_btnSave_clicked()
{
	m_model->setCorrections(m_correctionList);
	this->close();
}

void TrajCorrectDock::on_btnRefresh_clicked()
{
	//根据CurrentRow对应的m_correctionList对象重新初始化GroupBox_2
	int myRow = ui->listCorrections->currentRow();
	const Correction& myCor = m_correctionList[myRow];

	//刷新ui->listMeasurePoints
	ui->listMeasurePoints->clear();
	int pointCount = myCor.m_measurePoints.size() / 3;
	for (int i = 0; i < pointCount; i++)
		ui->listMeasurePoints->addItem(QStringLiteral("点%1").arg(i + 1));
	listFlagPoints->setPoints(myCor.m_flagPoints);

}




void TrajCorrectDock::on_btnExportFlagPoints_clicked()
{
	if(m_vFlagPoints.size() == 0)
	{
		QMessageBox::warning(this,
			QStringLiteral("警告"),
			QStringLiteral("当前无标记点可导出！"),
			QMessageBox::Ok);
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr(u8"导出标记点"),
		"",
		tr(u8"CSV 文件 (*.csv)")
	);

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".csv"))
		fileName += ".csv";  // 自动补全后缀

	exportCsvFlagPoints(fileName);
}

void TrajCorrectDock::on_btnImportMeasurePoints_clicked()
{
	//brief: 从csv文件导入pts
	// 0. 打开文件对话框，选择csv文件
	//1. 初始化到列表控件
	//2. 保存到成员变量m_vMeasurePoints中
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("导入测试点"),
		"",
		tr("CSV 文件 (*.csv)")
	);
	if (fileName.isEmpty())
		return;
	ui->listMeasurePoints->clear();
	m_vMeasurePoints.clear();
	importCsvPointsMeasure(fileName); //数据同步
	int pointCount = m_vMeasurePoints.size() / 3;
	for (int i = 0; i < pointCount; i++)
	{
		ui->listMeasurePoints->addItem(QStringLiteral("点%1").arg(i + 1));
	}//界面同步
}

void TrajCorrectDock::on_btnImportFlag_clicked()
{
	//从外部csv文件导入标记点
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("导入标记点"),
		"",
		tr("CSV 文件 (*.csv)")
	);
	if (fileName.isEmpty())
		return;
	listFlagPoints->clear();
	m_vFlagPoints.clear();
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开文件:" << fileName;
		return;
	}
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) continue;

		QStringList parts = line.split(QRegExp("[,;\\s]"), QString::SkipEmptyParts);
		if (parts.size() < 3) continue;

		bool ok1, ok2, ok3;
		double x = parts[0].toDouble(&ok1);
		double y = parts[1].toDouble(&ok2);
		double z = parts[2].toDouble(&ok3);

		if (ok1 && ok2 && ok3) {
			m_vFlagPoints.push_back(x);
			m_vFlagPoints.push_back(y);
			m_vFlagPoints.push_back(z);
		}
	}

	file.close();
	
	qDebug() << "成功导入点数量：" << m_vMeasurePoints.size() / 3;
	//同步到listFlagPoints
	for(int i = 0; i < m_vFlagPoints.size() / 3; i++)
		listFlagPoints->addItem(QStringLiteral("点%1").arg(i + 1));
}

void TrajCorrectDock::on_btnDevPage_clicked()
{
	double width = scroll->width();
	if(width == 350)
		scroll->setFixedWidth(800);
	else
		scroll->setFixedWidth(350);
}

void TrajCorrectDock::on_btnCal_clicked()
{
	int row = ui->listCorrections->currentRow();
	Correction cor = m_correctionList[row];
	cor.m_coeffs = cor.calCoeffs();
	
}

#pragma endregion

#pragma region 常用


void TrajCorrectDock::initGroupBox_AttributeDefine(QListWidgetItem * item)
{
	int corIndex = ui->listCorrections->row(item);
	const Correction &cor = m_correctionList[corIndex];

	// 将cor的属性值设置到界面控件上
	ui->editCorName->setText(cor.name());
	ui->chkApplyCor->setChecked(cor.isApplied());
	ui->comboCorType->setCurrentIndex(static_cast<int>(cor.m_interType));
	for (int i = 0; i < 6; i++)
		spinBoxes[i]->setValue(cor.rang(i));
	ui->chkIsPosCorrect->setChecked(cor.isPosCorrect());
	spnOriginx->setValue(cor.vBeamOrigin[0]);
	spnOriginy->setValue(cor.vBeamOrigin[1]);
	spnOriginz->setValue(cor.vBeamOrigin[2]);
	ui->spnDeg->setValue(cor.m_bendingDeg);

	//这里是初始化两个列表控件
	ui->listMeasurePoints->clear();
	int pointCount = cor.m_measurePoints.size() / 3;  
	for (int i = 0; i < pointCount; i++)
		ui->listMeasurePoints->addItem(QStringLiteral("点%1").arg(i + 1));
	listFlagPoints->setPoints(cor.m_flagPoints);
	//初始化cmbBeamDir
	std::vector<double> dir_x = {1.0, 0.0, 0.0};
	std::vector<double> dir_neg_x = {-1.0, 0.0, 0.0};
	std::vector<double> dir_y = {0.0, 1.0, 0.0};
	std::vector<double> dir_neg_y = {0.0, -1.0, 0.0};
	
	if (cor.vBeamDirection == dir_x) {
		ui->cmbBeamDir->setCurrentIndex(0); 
	}
	else if (cor.vBeamDirection == dir_neg_x) {
		ui->cmbBeamDir->setCurrentIndex(1); 
	}
	else if (cor.vBeamDirection == dir_y) {
		ui->cmbBeamDir->setCurrentIndex(2); 
	}
	else if (cor.vBeamDirection == dir_neg_y) {
		ui->cmbBeamDir->setCurrentIndex(3); 
	}

	if (cor.m_interType == Correction::interpolationType::Liner)
		ui->cmbFittingType->setCurrentIndex(0);
	else if (cor.m_interType == Correction::interpolationType::Euler)
		ui->cmbFittingType->setCurrentIndex(1);
	else if (cor.m_interType == Correction::interpolationType::Timoshenko)
		ui->cmbFittingType->setCurrentIndex(2);
	//同步初始化列表控件背后的成员变量
	m_vFlagPoints = cor.m_flagPoints;
	m_vMeasurePoints = cor.m_measurePoints;
	

	//日志信息
	ui->edtLog->appendPlainText(QString::fromLocal8Bit("切换到item,correction %1，属性同步设置到页面").arg(corIndex));
}

void TrajCorrectDock::on_btnAttributeSetOK_clicked()
{

	//输入合法性检查：输入的名字不能为空
	if(ui->editCorName->text().isEmpty())
	{
		QMessageBox::warning(this,
			QStringLiteral("警告"),
			QStringLiteral("修正对象名称不能为空！"),
			QMessageBox::Ok);
		return;
	}
	//弹出提示框，确认是否保存属性
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this,
		QStringLiteral("提示"),
		QStringLiteral("保存当前属性至修正对象：%1 ？").arg(ui->editCorName->text()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No);

	if (reply == QMessageBox::No)
	{
		return;
	}

	//保存属性到当前选中correction对象
	QString myName = ui->editCorName->text();
	double my_range[6];
	double my_bendingDeg = ui->spnDeg->value();
	bool my_isPosCor;
	if (ui->chkIsPosCorrect->isChecked())
		my_isPosCor = true;
	else
		my_isPosCor = false;
	for (int i = 0; i < 6; i++)
	{
		my_range[i] = spinBoxes[i]->value();
	}

	Eigen::Vector3d myOrigin = Eigen::Vector3d(
		spnOriginx->value(),
		spnOriginy->value(),
		spnOriginz->value()
	);
	std::vector<double> myDirection,myOrig;
	switch (ui->cmbBeamDir->currentIndex())
	{
		//x轴正方向
		case 0:
			myDirection = { 1.0, 0.0, 0.0 };
			break;
			//x轴负方向
		case 1:
			myDirection = { -1.0, 0.0, 0.0 };
			break;
			//y轴正方向
		case 2:
			myDirection = { 0.0, 1.0, 0.0 };
			break;
			//y轴负方向
		case 3:
			myDirection = { 0.0, -1.0, 0.0 };
			break;
		default:
			break;
	}
	myOrig = {
		static_cast<double>(spnOriginx->value()),
		static_cast <double>(spnOriginy->value()),
		static_cast<double> (spnOriginz->value())
	};
	Correction::interpolationType myType = Correction::interpolationType::Liner;
	if (ui->cmbFittingType->currentIndex() == 0)
		myType = Correction::interpolationType::Liner;
	else if (ui->cmbFittingType->currentIndex() == 1)
		myType = Correction::interpolationType::Euler;
	else if (ui->cmbFittingType->currentIndex() == 2)
		myType = Correction::interpolationType::Timoshenko;

	//获取当前行correction对象，保存属性
	int myRow = ui->listCorrections->currentRow();
	Correction& myCor = m_correctionList[myRow];
	myCor.setName(myName);
	myCor.setIsPosCorrect(my_isPosCor);
	myCor.setRange(my_range);
	myCor.setFlagPoints(m_vFlagPoints);
	myCor.setMeasurePoints(m_vMeasurePoints);
	myCor.setType(static_cast<Correction::interpolationType>(ui->cmbFittingType->currentIndex()));
	myCor.setBeamOrigin(myOrig);
	myCor.setBeamDir(myDirection);
	myCor.setType(myType);
	myCor.setBendingDeg(my_bendingDeg);
	


	
	myCor.setIsApply(false); //未体现在本窗口中，默认为false，这个属性在”修正函数管理“模块控制
	QListWidgetItem *myItem = ui->listCorrections->currentItem();
	myItem->setText(myName);
	

	ui->edtLog->setPlainText(QStringLiteral(">>"));
	ui->edtLog->setPlainText(QStringLiteral(">>属性已保存到修正对象：%1").arg(m_correctionList[myRow].m_name));
	ui->edtLog->setPlainText(QStringLiteral(">>当前item数目：%1").arg(m_correctionItems.count	()));
	ui->edtLog->setPlainText(QStringLiteral(">>当前correction数目：%1").arg(m_correctionList.count()));

	ui->btnNewCorrection->setEnabled(true);
	
	setView();

	//属性完整性检查，若属性不完整，设置为不可用，体现在列表项灰色显示
	//需要的属性有：范围，标记点，测试点
	if(my_range[0] == 0 && my_range[1] == 0 &&
		my_range[2] == 0 && my_range[3] == 0 &&
		my_range[4] == 0 && my_range[5] == 0)
	{
		myItem->setBackground(Qt::lightGray);
		myItem->setForeground(Qt::darkGray);
	}
	else if (m_vFlagPoints.size() == 0)
	{
		myItem->setBackground(Qt::lightGray);
		myItem->setForeground(Qt::darkGray);
	}
	else if (m_vMeasurePoints.size() == 0)
	{
		myItem->setBackground(Qt::lightGray);
		myItem->setForeground(Qt::darkGray);
	}
	else
	{
		myItem->setBackground(Qt::white);
		myItem->setForeground(Qt::black);
	}

}

void TrajCorrectDock::on_listCorrection_slectedItem(QListWidgetItem *item)      
{
	auto state = static_cast<ItemState>(item->data(Qt::UserRole).toInt());
	
	//当item标签页切换之后:
	// 1.btnAttributeSetOK按钮不可用，直到用户修改Groupbox_2中的属性，才能再次点击保存属性
	// 2. btnNewCorrection按钮可用，但是它永远在列表最下方创建对象而不是在中间插入
	// 3. Groupbox_2中的控件内容显示当前item对应correction的属性
	ui->btnNewCorrection->setEnabled(false);
	ui->groupBox_2->setEnabled(true);
	ui->btnDelCor->setEnabled(true);

	//item->setData(Qt::UserRole, static_cast<int>(ItemState::Edit));
	initGroupBox_AttributeDefine(item);  //触发emitedit，需保证当前处于edit状态
	
	
	//initPointLists();
	setView();
}

#pragma endregion 常用

#pragma region 辅助函数



#pragma endregion 辅助函数
