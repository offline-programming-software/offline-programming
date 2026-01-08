
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
#include "robxFileIO.h"

TrajCorrectDock::TrajCorrectDock(
    CComPtr<IPQPlatformComponent> ptrKit, 
	CPQKitCallback *ptrKitCallback, 
	QWidget *parent
)
	: QDockWidget("mywidget",parent), 
	  m_ptrKit(ptrKit),
	  m_ptrKitCallback2(ptrKitCallback), 
	  ui(new Ui::DockContent())
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

	//初始化一些自定义控件
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
	ui->loutOriginPt->addWidget(spnOriginx);
	ui->loutOriginPt->addWidget(spnOriginy);
	ui->loutOriginPt->addWidget(spnOriginz);
	InitCustomWidget();
	ui->verticalLayout->addWidget(listFlagPoints);
	scroll->setWidget(contentWidget);
	setWidget(contentForScroll);
	//setWidget(contentWidget);
	connect(ui->btnNewCorrection, SIGNAL(clicked()), this, SLOT(on_btnNewCorrection_clicked()));

	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setupConnections();
	connect(listFlagPoints, &pickWidget::blankAreaClicked, this, &TrajCorrectDock::on_pickBox_blankAreaClicked);
	connect(listFlagPoints, &pickWidget::deleteSignal, this, &TrajCorrectDock::testSignal);
	connect(this, &TrajCorrectDock::blankAreaClicked, this, &TrajCorrectDock::on_this_blankAreaClicked);
	connect(ui->btnDeleteCorrection, SIGNAL(clicked()), this, SLOT(on_btnDeleteCorrection_clicked()));
	connect(xMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(xMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(yMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(yMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(zMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(spnOriginx, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	connect(spnOriginy, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	connect(spnOriginz, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickOriginPoint);
	for (size_t i = 0; i < 6; i++)
	{
		connect(spinBoxes[i], &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
		connect(spinBoxes[i], QOverload<int>::of(&PickSpinBox::valueChanged),
			this, &TrajCorrectDock::onPickSpinBoxValueChanged);
		
	}
	connect(spnOriginx, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(spnOriginy, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(spnOriginz, QOverload<int>::of(&PickSpinBox::valueChanged), this, &TrajCorrectDock::on_spnOrigin_valueChanged);
	connect(zMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(ui->btnMeasurePtsInport, &QPushButton::clicked, this, &TrajCorrectDock::on_btnImportMeasurePoints_clicked);
	connect(ui->btnImportFlag, &QPushButton::clicked, this, &TrajCorrectDock::on_btnImportFlag_clicked);
	connect(ui->btnFlagPointsExport, &QPushButton::clicked, this, &TrajCorrectDock::on_btnExportFlagPoints_clicked);
	connect(ui->btnAttributeSetOK, &QPushButton::clicked, this,&TrajCorrectDock::on_btnAttributeSetOK_clicked);
	listFlagPoints->setStyleSheet("border: 1px solid black");
	connect(ui->btnRefresh, &QPushButton::clicked, this, &TrajCorrectDock::on_btnRefresh_clicked);
	connect(ui->listCorrections, &QListWidget::itemClicked, this, &TrajCorrectDock::on_listCorrection_slectedItem);
	connect(ui->comboCorType, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxIndexChanged(int)));
	initDock();
	connect(ui->btnSave, &QPushButton::clicked, this, &TrajCorrectDock::on_btnSave_clicked);
	//---------------pq回调信号--------------------------
	connect(m_ptrKitCallback2, &CPQKitCallback::signalElementPickup, this, &TrajCorrectDock::OnPickup);
	//connect(m_ptrKitCallback2, &CPQKitCallback::signalElementSelection, this, &TrajCorrectDock::OnElementSelection);
	connect(m_ptrKitCallback2, &CPQKitCallback::signalDraw, this, &TrajCorrectDock::OnDraw);
	


	//-------------test-----------------

	//---------init------------
		//从json读入数据，初始化数据列表, 初始化变形列表控件
	m_io = new RobxIO();
	m_io->updateData(m_correctionList, "correctionList.json");
	for (const Correction& corr : m_correctionList)
	{
		QListWidgetItem* item = new QListWidgetItem(corr.name());
		m_correctionItems.push_back(item);
	}
	InitLists();
	GetAllPathID();   //获取所有轨迹ID,名称,轨迹点ID
	GetPointInfo();	  //获取所有轨迹点位姿
	setWindowTitle(QString::fromLocal8Bit("弯曲变形配置"));
	
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

	for (size_t i = 0; i + 2 < m_vFlagPoints.size(); i += 3) {
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

	for (size_t i = 0; i + 2 < m_vMeasurePoints.size(); i += 3) {
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
void TrajCorrectDock::initDock()
{
	
	ui->btnDeleteCorrection->setEnabled(false);
	ui->groupBox_2->setEnabled(false);
	for (size_t i = 0; i < 6; i++)
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

	// 5?? 可选：默认选中第一个
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
	listFlagPoints->clear();
	ui->listMeasurePoints->clear();
	ui->cmbFittingType->setCurrentIndex(0);

}
void TrajCorrectDock::initGroupBox_AttributeDefine(QListWidgetItem * item)
{
	int corIndex = ui->listCorrections->row(item);
	const Correction &cor = m_correctionList[corIndex];

	// 将cor的属性值设置到界面控件上
	ui->editCorName->setText(cor.name());
	ui->chkApplyCor->setChecked(cor.isApplied());
	ui->comboCorType->setCurrentIndex(static_cast<int>(cor.m_interType));
	for (size_t i = 0; i < 6; i++)
		spinBoxes[i]->setValue(cor.rang(i));
	ui->chkIsPosCorrect->setChecked(cor.isPosCorrect());
	spnOriginx->setValue(cor.vBeamOrigin[0]);
	spnOriginy->setValue(cor.vBeamOrigin[1]);
	spnOriginz->setValue(cor.vBeamOrigin[2]);
	if(cor.m_beamModelType == Correction::beamModel::Euler)
		ui->chkIsTwisted->setChecked(false);
	if(cor.m_beamModelType == Correction::beamModel::Timoshenko)
		ui->chkIsTwisted->setChecked(true);
	//这里是初始化两个列表控件
	ui->listMeasurePoints->clear();
	size_t pointCount = cor.m_measurePoints.size() / 3;  
	for (size_t i = 0; i < pointCount; i++)
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
	//同步初始化列表控件背后的成员变量
	m_vFlagPoints = cor.m_flagPoints;
	m_vMeasurePoints = cor.m_measurePoints;
	

	//日志信息
	ui->edtLog->appendPlainText(QString::fromLocal8Bit("切换到item,correction %1，属性同步设置到页面").arg(corIndex));
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

	//devPage
	connect(ui->btnDevPage, &QPushButton::clicked, this, &TrajCorrectDock::on_btnDevPage_clicked);
	connect(ui->btnCal, &QPushButton::clicked, this, &TrajCorrectDock::on_btnCal_clicked);

}

#pragma endregion regionName

#pragma region 拾取相关函数


void TrajCorrectDock::OnDraw()
{
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

		
		for (size_t i = 0; i < m_vFlagPoints.size(); i++)
		{
			dPos[counter++] = m_vFlagPoints[i];
			if ((counter % 3) == 0)
			{
				wss.str(L"");
				wss.clear();
				wss << L"点" << (i/3)+1;
				strText = wss.str().c_str();
				m_ptrKit->View_draw_point(dPos, 0, 15, RGB(10, 100, 200), strText, RGB(255, 140, 0));
				counter = 0;
				
			}

		}
		break;
	case TrajCorrectDock::DrawSource::FromMeasurePoints:
			
		break;

		case TrajCorrectDock::DrawSource::FromOriginPoint:
			

			dPos0[0] = spnOriginx->value();
			dPos0[1] = spnOriginy->value();
			dPos0[2] = spnOriginz->value();
			m_ptrKit->View_draw_point(dPos0, 0, 20, RGB(255, 0, 0), bstr, RGB(255, 0, 0));
			break;
	case TrajCorrectDock::DrawSource::FromRangePoints:
		for (size_t i = 0; i < 6; i++)
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
			int segments = 10; // 
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

				for (int i = 0; i <= segments; ++i)
				{
					double t = static_cast<double>(i) / segments;
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
	default:
		break;
	}
}





void TrajCorrectDock::OnPickup(unsigned long i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType, double i_dPointX, double i_dPointY, double i_dPointZ)
{
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
		for (size_t i = 0; i < myPointCounter; i++)
		{
			listFlagPoints->addItem(QStringLiteral("点%1").arg(i+1));
		}
		
		break;
	case TrajCorrectDock::PickSource::FromOriginPoint:
		spnOriginx->setValue(i_dPointX);
		spnOriginy->setValue(i_dPointY);
		spnOriginz->setValue(i_dPointZ);
		m_ptrKit->Doc_end_module(cmd);
		break;
	case TrajCorrectDock::PickSource::FromSpinRanges:
		qDebug() << "It's OnElementPickup from class TrajCorrectDock";
		
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
		for (size_t i = 0; i < 6; i++)
		{
			if (spinBoxes[i]->value() == 0)
			{
				isFull = false;
				break;
			}
		}
		if (isFull)
		{
			break;
		}
		else
		{
			m_ptrKit->Doc_end_module(cmd);
			break;
		}
		

		
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

void TrajCorrectDock::pickRange()
{
	//#拾取相关：
	//点击范围SpinBox时，进入拾取状态，从轨迹中拾取范围值
	if(m_pickSource == PickSource::FromSpinRanges &&
		m_drawSource == DrawSource::FromRangePoints)
	{
		//已经在拾取状态，避免重复触发
		return;
	}
	m_pickSource = PickSource::FromSpinRanges;
	m_drawSource = DrawSource::FromRangePoints;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	for (size_t i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
	QObject *obj = sender();
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
	if(m_pickSource == PickSource::FromOriginPoint &&
		m_drawSource == DrawSource::FromOriginPoint)
	{
		//已经在拾取状态，避免重复触发
		return;
	}
	m_pickSource = PickSource::FromOriginPoint;
	m_drawSource = DrawSource::FromOriginPoint;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	//视觉提示
	spnOriginx->setStyleSheet("PickSpinBox{border: 3px solid blue}");
	spnOriginy->setStyleSheet("PickSpinBox{border: 3px solid blue}");
	spnOriginz->setStyleSheet("PickSpinBox{border: 3px solid blue}");


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
	for (size_t i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
	m_drawSource = DrawSource::None;
	spnOriginx->setStyleSheet("PickSpinBox{border: 1px solid black}");
	spnOriginy->setStyleSheet("PickSpinBox{border: 1px solid black}");
	spnOriginz->setStyleSheet("PickSpinBox{border: 1px solid black}");
}



#pragma endregion 拾取相关函数

#pragma region pq函数

void TrajCorrectDock::getTrajPoints(double range[6], std::vector<std::vector<double>>& trajPointsToCorrect)
{
	////获取所有轨迹id
	//BSTR sName;  
	//BSTR sIDs;  
	//m_ptrKit->pq_GetAllDataObjectsByType(80, &sName, &sIDs);
	//ULONG ulPathID = 0;     
	//INT nStartIndex = 0;      //轨迹点起始索引
	//INT nCount = 5;           //批量获取轨迹点的数目
	//INT nPostureType = 0;         //姿态表示方式 四元数0   欧拉角：1-4
	//ULONG uCoordinateID = 0;       //在坐标系ID下表示
	//m_ptrKit->pq_GetAllDataObjectsByType(80, &sName, &sIDs);
	ULONG uID = 0;
	getObjIdByName(PQ_PATH, L"", uID);
	VARIANT varIDArray;
	m_ptrKit->PQAPIGetPointsID(uID, &varIDArray);
	GetAllPathID();
}


void TrajCorrectDock::getObjIdByName(PQDataType i_datatype, std::wstring i_wsName, ULONG & o_uID)
{

	VARIANT vNamePara;
	vNamePara.parray = NULL;
	VARIANT vIDPara;
	vIDPara.parray = NULL;
	m_ptrKit->Doc_get_obj_bytype(i_datatype, &vNamePara, &vIDPara);
	LONG type = 80;  
	BSTR sName;  
	BSTR sIDs; 
	m_ptrKit->pq_GetAllDataObjectsByType(i_datatype, &sName, &sIDs);
	

}

void TrajCorrectDock::GetAllPathID()    //得到所有轨迹点ID
{
	//BSTR trajNames;
	//BSTR trajIDs;
	//
	//m_ptrKit->pq_GetAllDataObjectsByType(PQ_PATH, &trajNames, &trajIDs);
	//std::string sNames = (const char*)trajNames;
	//std::stringstream ss(sNames);
	//std::string item;
	//unsigned long id;
	//m_vAllPathNames.clear();
	//while (std::getline(ss, item, '#')) {
	//	if (!item.empty()) {
	//		m_vAllPathNames.push_back(item);
	//	}
	//}
	//std::string sIDs = (const char*)trajIDs;
	//ss.clear();
	//ss.str(sIDs);
	//m_vAllPathIDs.clear();
	//while (std::getline(ss,item,'#'))
	//{
	//	if (!item.empty()) {
	//		id = std::stoul(item);
	//		m_vAllPathIDs.push_back(id);
	//	}
	//}

	//ULONG *bufID = nullptr;
	//long lenID;
	//VARIANT varTrajIDArray;
	//m_vAllPointIDs.clear();
	//for (size_t j = 0; j < m_vAllPathIDs.size(); j++)
	//{
	//	m_ptrKit->PQAPIGetPointsID(m_vAllPathIDs[j], &varTrajIDArray);

	//	lenID = varTrajIDArray.parray->rgsabound[0].cElements; //元素个数
	//	SafeArrayAccessData(varTrajIDArray.parray, (void**)&bufID);  //把地址赋给buffID
	//	for (size_t i = 0; i < lenID; i++)
	//	{
	//		m_vAllPointIDs.push_back(bufID[i]);
	//		m_mapAllPointIDs[m_vAllPathIDs[j]].push_back(bufID[i]);
	//	}
	//	SafeArrayUnaccessData(varTrajIDArray.parray);//释放
	//}
	////GetPointInfo();
}

void TrajCorrectDock::GetPointInfo()
{
	double x,y,z,q1,q2,q3,q4;
	std::vector<double> posture;
	ULONG ulPointID = 0;
	INT nPostureType = 0;
	INT nPostureCount = 0;
	double* dPointPosture = new double;
	double dVelocity = 0;
	double dSpeedPercent = 0;
	PQPointInstruction nInstruct = PQ_LINE;
	INT nApproach = 0;
	m_v2dAllPointsPositions.clear();

	for (size_t i = 0; i < m_vAllPointIDs.size(); i++)
	{
		ulPointID = m_vAllPointIDs[i];
		m_ptrKit->PQAPIGetPointInfo(ulPointID, QUATERNION , &nPostureCount, &dPointPosture,
			&dVelocity, &dSpeedPercent, &nInstruct, &nApproach);
		x = *dPointPosture;
		y = *(dPointPosture + 1);
		z = *(dPointPosture + 2);
		q1 = *(dPointPosture + 3);
		q2 = *(dPointPosture + 4);
		q3 = *(dPointPosture + 5);
		q4 = *(dPointPosture + 6);
		posture.push_back(x); posture.push_back(y); posture.push_back(z);
		posture.push_back(q1); posture.push_back(q2); posture.push_back(q3); posture.push_back(q4);
		m_v2dAllPointsPositions.push_back(posture);
		posture.clear();
		m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
	}
	
}

void TrajCorrectDock::GetPoints2Correct(double range[6])
{
	//double xMin, xMax, yMin, yMax, zMin, zMax;
	//xMin = (std::min)(range[0], range[1]);
	//xMax = (std::max)(range[0], range[1]);
	//yMin = (std::min)(range[2], range[3]);
	//yMax = (std::max)(range[2], range[3]);
	//zMin = (std::min)(range[4], range[5]);
	//zMax = (std::max)(range[4], range[5]);
	//double x, y, z;
	//
	////m_v2dPointsToCorrect.clear();
	////for (const auto& point : m_v2dAllPointsPositions) {
	////	x = point[0];
	////	y = point[1];
	////	z = point[2];

	////	if (x >= xMin && x <= xMax
	////		&& y >= yMin && y <= yMax
	////		&& z >= zMin && z <= zMax)
	////	{
	////		m_v2dPointsToCorrect.push_back(point);
	////	}

	////}
	//m_v2dPointsToCorrect.clear();
	//m_vPointsToCorrectID.clear();
	//for (size_t i = 0; i < m_v2dAllPointsPositions.size(); i++)
	//{
	//	const auto& point = m_v2dAllPointsPositions[i];
	//	double x = point[0];
	//	double y = point[1];
	//	double z = point[2];

	//	if (x >= xMin && x <= xMax &&
	//		y >= yMin && y <= yMax &&
	//		z >= zMin && z <= zMax)
	//	{
	//		m_v2dPointsToCorrect.push_back(point);
	//		m_vPointsToCorrectID.push_back(m_vAllPointIDs[i]);
	//	}
	//}
	//GetParentPath();
}

void TrajCorrectDock::GetParentPath()
{
	ULONG ulPointID = 0;
	ULONG ulPathID = 0;
	m_ptrKit->Point_get_parent_path(ulPointID, &ulPathID);
	
}

void TrajCorrectDock::modifyPointsPoses(const std::vector<unsigned long>& CorrectPointID, const std::vector<std::vector<double>>& newPoints)
{
	int posCount = 7;
	int posType = 0;
	double dPosition[7] = { 0,0,0,0,0,0,0 };
	size_t num = m_vPointsToCorrectID.size();
	for (size_t i = 0; i < num; i++)
	{
		for (size_t j = 0; j < 7; j++)
		{
			dPosition[j] = m_v2dPointsToCorrect[i][j];
		}
		dPosition[1] = dPosition[1] + 100;
		dPosition[2] = dPosition[2] - 100;
		m_ptrKit->PQAPIModifyPointPosture(m_vPointsToCorrectID[i], dPosition, posCount, QUATERNION);
	}
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this,                         // 父窗口
		QStringLiteral("提示"),                       // 标题
		QStringLiteral(" 是否应用此修正 ？").arg(ui->editCorName->text()),   // 正文内容
		QMessageBox::Yes | QMessageBox::No, // 按钮
		QMessageBox::No);             // 默认选中 No
	if (reply == QMessageBox::Yes)
	{
		return;
	}
	if (reply == QMessageBox::No)
	{
		
		for (size_t i = 0; i < num; i++)
		{
			for (size_t j = 0; j < 7; j++)
			{
				dPosition[j] = m_v2dPointsToCorrect[i][j];
			}
			
			m_ptrKit->PQAPIModifyPointPosture(m_vPointsToCorrectID[i], dPosition, posCount, QUATERNION);
		}
	}
}

void TrajCorrectDock::onPickSpinBoxValueChanged(int a)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	m_ptrKit->Doc_start_module(cmd);
}

void TrajCorrectDock::on_spnOrigin_valueChanged(int a)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	m_ptrKit->Doc_start_module(cmd);
}

#pragma endregion






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
	auto state = static_cast<ItemState>(item->data(Qt::UserRole).toInt());
	if (state == ItemState::View)
		return;
	item->setData(Qt::UserRole, static_cast<int>(ItemState::View));
	//去掉item后面的*
	QString itemText = item->text();
	itemText = itemText.replace("*", "");
	item->setText(itemText);
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

void TrajCorrectDock::on_listCorrection_slectedItem(QListWidgetItem *item)      
{
	auto state = static_cast<ItemState>(item->data(Qt::UserRole).toInt());
	
	//当item标签页切换之后:
	// 1.btnAttributeSetOK按钮不可用，直到用户修改Groupbox_2中的属性，才能再次点击保存属性
	// 2. btnNewCorrection按钮可用，但是它永远在列表最下方创建对象而不是在中间插入
	// 3. Groupbox_2中的控件内容显示当前item对应correction的属性
	ui->btnNewCorrection->setEnabled(false);
	ui->groupBox_2->setEnabled(true);
	ui->btnDeleteCorrection->setEnabled(true);

	//item->setData(Qt::UserRole, static_cast<int>(ItemState::Edit));
	initGroupBox_AttributeDefine(item);  //触发emitedit，需保证当前处于edit状态
	
	
	//initPointLists();
	setView();
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
	ui->edtLog->appendPlainText(QStringLiteral(">>新建修正对象：%1").arg(itemName));
	ui->edtLog->appendPlainText(QStringLiteral(">>请在属性定义中设置修正对象属性"));
	ui->edtLog->appendPlainText(QStringLiteral(">>当前的item数目：%1").arg(m_correctionItems.count()));
	ui->edtLog->appendPlainText(QStringLiteral(">>当前的correction数目：%1").arg(m_correctionList.count()));
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
	bool my_isPosCor;
	if (ui->chkIsPosCorrect->isChecked())
		my_isPosCor = true;
	else
		my_isPosCor = false;
	for (size_t i = 0; i < 6; i++)
	{
		my_range[i] = spinBoxes[i]->value();
	}
	if (ui->cmbFittingType->currentIndex() == 0) {
		Correction::interpolationType myIntTypes = Correction::interpolationType::Liner;
	}
	else if (ui->cmbFittingType->currentIndex() == 1)
	{
		Correction::interpolationType myIntTypes = Correction::interpolationType::Other;
	}
	else if (ui->cmbFittingType->currentIndex() == 2)
	{
		Correction::interpolationType myIntTypes = Correction::interpolationType::Poly;
	}
	Correction::beamModel beamType;
	if (ui->chkIsTwisted->isChecked())
		beamType = Correction::beamModel::Timoshenko;
	else
		beamType = Correction::beamModel::Euler;

	GetPoints2Correct(my_range);
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

	//获取当前行correction对象，保存属性
	int myRow = ui->listCorrections->currentRow();
	Correction& myCor = m_correctionList[myRow];
	myCor.setName(myName);
	myCor.setIsPosCorrect(my_isPosCor);
	myCor.setRange(my_range);
	myCor.setFlagPoints(m_vFlagPoints);
	myCor.setMeasurePoints(m_vMeasurePoints);
	myCor.set_m_v2dTrajPointsToCorrect(m_v2dPointsToCorrect);
	myCor.setType(static_cast<Correction::interpolationType>(ui->cmbFittingType->currentIndex()));
	myCor.setBeamModelType(beamType);
	myCor.setBeamOrigin(myOrig);
	myCor.setBeamDir(myDirection);



	
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

void TrajCorrectDock::on_btnDeleteCorrection_clicked()
{
	//#重写
}

void TrajCorrectDock::on_btnSave_clicked()
{
	m_io->writeData(m_correctionList, "correctionList.json");
	this->close();
}

void TrajCorrectDock::on_btnRefresh_clicked()
{
	//根据CurrentRow对应的m_correctionList对象重新初始化GroupBox_2
	int myRow = ui->listCorrections->currentRow();
	const Correction& myCor = m_correctionList[myRow];

	//刷新ui->listMeasurePoints
	ui->listMeasurePoints->clear();
	size_t pointCount = myCor.m_measurePoints.size() / 3;
	for (size_t i = 0; i < pointCount; i++)
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
	for (size_t i = 0; i < pointCount; i++)
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
	cor.m_coeffs = cor.calculate();
}

#pragma endregion

#pragma region 辅助函数

#pragma endregion 辅助函数
