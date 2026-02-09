
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
	//在构造 TrajCorrectDock 对象时，同时也 new 了一个 Ui::DockContent 类的实例，并把这个指针赋值给成员变量 ui。
{
	QVBoxLayout *layoutForScroll = new QVBoxLayout();
	layoutForScroll->setContentsMargins(0, 0, 0, 0);
	QWidget *contentForScroll = new QWidget(this);
	contentForScroll->setLayout(layoutForScroll);
	QScrollArea *scroll = new QScrollArea();
	scroll->setFixedWidth(330);
	layoutForScroll->addWidget(scroll);
	m_io = new RobxIO();
	m_io->updateData(m_correctionList, "correctionList.json");
	for (Correction* ptr : m_correction) {
		delete ptr;  // 避免内存泄漏
	}
	m_correction.clear();

	// 从 QVector<Correction> 创建新的指针对象
	for (const Correction& c : m_correctionList) {
		m_correction.push_back(new Correction(c));  // 拷贝构造生成新对象
	}
	QWidget *contentWidget = new QWidget();
	ui->setupUi(contentWidget);
	contentWidget->setFixedWidth(300);
	pickBox = new pickWidget(contentWidget);  //pickbox的父对象被设置为contentWidget，在QT中的父子关系负责内存管理，删除父对象时，它会自动删除所有子对象（不需要你手动 delete）
	xMinspin = new PickSpinBox(contentWidget);
	xMaxspin = new PickSpinBox(contentWidget);
	yMinspin = new PickSpinBox(contentWidget);
	yMaxspin = new PickSpinBox(contentWidget);
	zMinspin = new PickSpinBox(contentWidget);
	zMaxspin = new PickSpinBox(contentWidget);
	spinBoxes = { xMinspin, xMaxspin, yMinspin, yMaxspin, zMinspin, zMaxspin };
	InitCustomWidget();
	ui->verticalLayout->addWidget(pickBox);
	scroll->setWidget(contentWidget);
	setWidget(contentForScroll);
	//setWidget(contentWidget);
	InitLists();
	connect(ui->btnNewCorrection, SIGNAL(clicked()), this, SLOT(on_btnNewCorrection_clicked()));

	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	connect(pickBox, &pickWidget::blankAreaClicked, this, &TrajCorrectDock::on_pickBox_blankAreaClicked);
	connect(pickBox, &pickWidget::deleteSignal, this, &TrajCorrectDock::testSignal);
	connect(pickBox, &pickWidget::clearSignal, this, &TrajCorrectDock::on_pickBox_clear);
	

	connect(this, &TrajCorrectDock::blankAreaClicked, this, &TrajCorrectDock::on_this_blankAreaClicked);
	connect(ui->btnDeleteCorrection, SIGNAL(clicked()), this, SLOT(on_btnDeleteCorrection_clicked()));
	connect(xMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(xMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(yMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(yMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	connect(zMinspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	for (size_t i = 0; i < 6; i++)
	{
		connect(spinBoxes[i], &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
		connect(spinBoxes[i], QOverload<int>::of(&PickSpinBox::valueChanged),
			this, &TrajCorrectDock::onPickSpinBoxValueChanged);
		
	}
	connect(zMaxspin, &PickSpinBox::lineEditClicked, this, &TrajCorrectDock::pickRange);
	
	connect(ui->btnAttributeSetOK, &QPushButton::clicked, this,&TrajCorrectDock::on_btnAttributeSetOK_clicked);
	connect(ui->btnFlagPointsImport, &QPushButton::clicked, this, &TrajCorrectDock::onImportFlagPointsClicked);
	connect(ui->btnMeasurePtsExport, &QPushButton::clicked, this, &TrajCorrectDock::onExportMeasurePointsClicked);
	pickBox->setStyleSheet("border: 1px solid black");
	connect(ui->listCorrections, &QListWidget::itemClicked, this, &TrajCorrectDock::on_listCorrection_slectedItem);
	connect(ui->comboCorType, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxIndexChanged(int)));
	connect(ui->chkApplyCor, SIGNAL(toggled(bool)), this, SLOT(on_chkApplyCor_toggled(bool)));
	initDock();
	connect(ui->btnSave, &QPushButton::clicked, this, &TrajCorrectDock::on_btnSave_clicked);
	//---------------pq回调信号--------------------------
	connect(m_ptrKitCallback2, &CPQKitCallback::signalElementPickup, this, &TrajCorrectDock::OnPickup);
	//connect(m_ptrKitCallback2, &CPQKitCallback::signalElementSelection, this, &TrajCorrectDock::OnElementSelection);
	connect(m_ptrKitCallback2, &CPQKitCallback::signalDraw, this, &TrajCorrectDock::OnDraw);
	


	//-------------test-----------------
	connect(ui->btnTest, &QPushButton::clicked, this, &TrajCorrectDock::on_btnTest_clicked);
	connect(ui->btnTest_2, &QPushButton::clicked, this, &TrajCorrectDock::on_btnTest_2_clicked);

	//---------init------------

	setWindowTitle(QString::fromLocal8Bit("弯曲变形配置"));
	
	
}

TrajCorrectDock::~TrajCorrectDock()
{
	delete ui;
}

void TrajCorrectDock::importCsvPoints(const QString & filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开文件:" << filePath;
		return;
	}

	m_vFlagPoints.clear();  // 清空旧数据

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

	qDebug() << "成功导入点数量：" << m_vFlagPoints.size() / 3;
}

void TrajCorrectDock::importCsvPointsMeasure(const QString & filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开文件:" << filePath;
		return;
	}

	m_vMeasurePoints.clear();  // 清空旧数据
	
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

	// 2?? 遍历 QVector<Correction>
	for (int i = 0; i < m_correctionList.size(); ++i)
	{
		const Correction& c = m_correctionList[i]; // 安全访问

		// 3?? 构建显示文本，例如显示名称 + 是否应用
		QString displayText = c.m_name;
		if (c.m_isApply)
			displayText += " [已应用]";

		// 4?? 添加到 QListWidget
		ui->listCorrections->addItem(displayText);
	}

	// 5?? 可选：默认选中第一个
	if (ui->listCorrections->count() > 0)
		ui->listCorrections->setCurrentRow(0);
}

void TrajCorrectDock::initPointLists()
{
	int myCurrentRow = ui->listCorrections->currentRow();
	Correction* myCor = m_correction[myCurrentRow];
	std::vector<double> my_vFlagPoints = myCor->flagPoints();
	std::vector<double> my_vMeasurePoints = myCor->measurePoints();
	pickBox->clear();
	ui->listMeasurePoints->clear();
	if (my_vFlagPoints.size()/3 >0)
	{
		for (size_t i = 0; i < my_vFlagPoints.size()/3; i++)
		{
			pickBox->addItem(QStringLiteral("点%1").arg(i + 1));
		}
	}
	if (my_vMeasurePoints.size()>0)
	{
		for (size_t i = 0; i < my_vMeasurePoints.size()/3; i++)
		{
			ui->listMeasurePoints->addItem(QStringLiteral("点%1").arg(i + 1));

		}
	}
	
}

void TrajCorrectDock::initGroupBox_AttributeDefine(QListWidgetItem * item)
{
	int corIndex = ui->listCorrections->row(item);
	Correction *myCor = m_correction[corIndex];
	ui->editCorName->setText(myCor->name());
	on_chkApplyCor_toggled(myCor->isApplied());
	ui->chkApplyCor->setChecked(myCor->isApplied());
	if (myCor->type() == Correction::interpolationType::Liner)
	{
		ui->comboCorType->setCurrentIndex(0);
		ui->stackedWidget->setCurrentIndex(0);
	}
		
	else if (myCor->type() == Correction::interpolationType::Liner)
	{
		ui->comboCorType->setCurrentIndex(1);
		ui->stackedWidget->setCurrentIndex(1);
	}
	else
	{
		ui->comboCorType->setCurrentIndex(2);
		ui->stackedWidget->setCurrentIndex(2);
	}
	for (size_t i = 0; i < 6; i++)
	{
		spinBoxes[i]->setValue(myCor->rang(i));
	}
	ui->chkIsPosCorrect->setChecked(myCor->isPosCorrect());
	
	
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
}







void TrajCorrectDock::testSignal(int num)
{
	std::cout << "SignalEmit" << std::endl;
	std::cout << "num = "<<num << std::endl;

}

void TrajCorrectDock::on_this_blankAreaClicked()
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	pickBox->setStyleSheet("border: 1px solid black");
	for (size_t i = 0; i < 6; i++)
	{
		spinBoxes[i]->setStyleSheet("PickSpinBox{border: 1px solid black}");
	}
	m_drawSource = DrawSource::None;
}



void TrajCorrectDock::on_btnNewCorrection_clicked()
{
	QListWidgetItem* item = new QListWidgetItem(ui->listCorrections);
	ui->btnNewCorrection->setEnabled(false);
	QString itemName = QString::fromLocal8Bit("新建修正对象");
	for (QListWidgetItem* it : m_correctionItems) {
		const int suffix = 1;
		if (itemName == it->text()) {
			itemName = QString::fromLocal8Bit("新建修正对象%1").arg(suffix);
		}
	}
	item->setText(itemName);
	ui->groupBox_2->setEnabled(true);	
	


	//m_correctCounter++;
	//QString name = QString("correct%1").arg(m_correctCounter);
	//m_correction.push_back(new Correction());
	//	ui->listCorrections->clear();
	//for (size_t i = 0; i < m_correctCounter; i++)
	//{
	//	QListWidgetItem* item = new QListWidgetItem(m_correction[i]->name());
	//	item->setForeground(Qt::black);   // 设置字体颜色为灰色
	//	item->setBackground(Qt::white);       // 设置背景为白色
	//	ui->listCorrections->addItem(item);   // 添加到列表中
	//	
	//}
}

void TrajCorrectDock::on_btnDeleteCorrection_clicked()
{
	
	QListWidgetItem *item = ui->listCorrections->currentItem();
	int row = ui->listCorrections->row(item);
	if (item) {
		// 从列表中删除该项（会自动释放内存）
		delete ui->listCorrections->takeItem(ui->listCorrections->row(item));
		delete m_correction[row];
		m_correction.erase(m_correction.begin() + row);
	}
}

void TrajCorrectDock::on_btnAttributeSetOK_clicked()
{

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

	GetPoints2Correct(my_range);

	int myRow = ui->listCorrections->currentRow();
	Correction *myCor = m_correction[myRow];
	bool myIsApply = ui->chkApplyCor->isChecked();

	myCor->setName(myName);
	myCor->setIsPosCorrect(my_isPosCor);
	myCor->setRange(my_range);
	myCor->setFlagPoints(m_vFlagPoints);
	myCor->set_m_v2dTrajPointsToCorrect(m_v2dPointsToCorrect);
	myCor->setIsApply(myIsApply);
	qDebug() << "auto connected!";
	QListWidgetItem *myItem = ui->listCorrections->currentItem();
	myItem->setText(ui->editCorName->text());

	
}

void TrajCorrectDock::on_btnSave_clicked()
{
	m_correctionList.clear();
	for (Correction* cptr : m_correction) {
		if (cptr) {
			m_correctionList.push_back(*cptr); // 将对象值拷贝到 QVector
		}
	}
	m_io->writeData(m_correctionList, "correctionList.json");
	this->close();
}

void TrajCorrectDock::on_listCorrection_slectedItem(QListWidgetItem *item)      
{
	ui->groupBox_2->setEnabled(true);
	ui->btnDeleteCorrection->setEnabled(true);
	int row = ui->listCorrections->row(item);
	
	initGroupBox_AttributeDefine(item);
	
	initPointLists();
}

void TrajCorrectDock::on_pickBox_blankAreaClicked()
{
	m_pickSource = PickSource::FromListFlags;
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_start_module(cmd);
	pickBox->setStyleSheet("border: 3px solid blue");
	m_drawSource = DrawSource::FromFlagPoints;
}

void TrajCorrectDock::on_pickBox_delet()
{
}

void TrajCorrectDock::on_pickBox_clear()
{
	m_vFlagPoints.clear();
	pickBox->clear();
}

void TrajCorrectDock::onImportFlagPointsClicked()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("导入标记点"),
		"",
		tr("CSV 文件 (*.csv);;Excel 文件 (*.xlsx *.xls)")
	);

	if (fileName.isEmpty())
		return;

		importCsvPoints(fileName);
		pickBox->clear();
		int pointNum = m_vFlagPoints.size() / 3;
		for (size_t i = 0; i < pointNum; i++)
		{
			pickBox->addItem(QStringLiteral("点%1").arg(i + 1));
		}
}

void TrajCorrectDock::on_btnMeasurePtsInport_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("导入标记点"),
		"",
		tr("CSV 文件 (*.csv);;Excel 文件 (*.xlsx *.xls)")
	);

	if (fileName.isEmpty())
		return;

	importCsvPoints(fileName);

	ui->listMeasurePoints->clear();
	int myPointNum = m_vMeasurePoints.size() / 3;
	for (size_t i = 0; i < myPointNum; i++)
	{
		pickBox->addItem(QStringLiteral("点%1").arg(i + 1));
	}
}

void TrajCorrectDock::on_btnFlagPointsExport_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("导出标记点"),
		"",
		tr("CSV 文件 (*.csv)")
	);

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".csv"))
		fileName += ".csv";  // 自动补全后缀

	exportCsvFlagPoints(fileName);
}

void TrajCorrectDock::onExportMeasurePointsClicked()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("导出测试点"),
		"",
		tr("CSV 文件 (*.csv)")
	);

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".csv"))
		fileName += ".csv";  // 自动补全后缀

	exportCsvMeasurePoints(fileName);
}

void TrajCorrectDock::onComboBoxIndexChanged(int index)
{
	ui->stackedWidget->setCurrentIndex(index);
}



void TrajCorrectDock::text()
{
	qDebug() << " spinBox clicked";

}

void TrajCorrectDock::pickRange()
{
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

void TrajCorrectDock::onPickSpinBoxValueChanged(int a)
{
	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	m_ptrKit->Doc_end_module(cmd);
	m_ptrKit->Doc_start_module(cmd);
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
		pickBox->clear();
		for (size_t i = 0; i < myPointCounter; i++)
		{
			pickBox->addItem(QStringLiteral("点%1").arg(i+1));
		}
		
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

void TrajCorrectDock::OnElementSelection(LPWSTR i_wObjNames, LPWSTR i_wFaceNames, double * i_dPointXYZ, int i_nSize)
{
	qDebug() << "It's OnElementSelection from class TrajCorrectDock";

}

void TrajCorrectDock::OnDraw()
{
	CComBSTR strText = "PtText";
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
	case TrajCorrectDock::DrawSource::FromRangePoints:
#pragma region 绘制作用域函数
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
			std::vector<std::pair<int, int>> edges = {
				{0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
				{4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
				{0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
			};
			for (const auto& edge : edges) {
				auto p1 = vertices[edge.first];
				auto p2 = vertices[edge.second];

				for (int i = 0; i <= segments; ++i) {
					double t = static_cast<double>(i) / segments;
					double dPos[3] = {
						(1 - t) * p1[0] + t * p2[0],
						(1 - t) * p1[1] + t * p2[1],
						(1 - t) * p1[2] + t * p2[2]
					};

					CComBSTR emptyText = L"";
					m_ptrKit->View_draw_point(dPos, 0, 3, RGB(0, 255, 255), emptyText, RGB(0, 255, 255)); // 青色细点
				}
			}

			//画线函数 输入2个dPos 

		}
#pragma endregion 绘制作用域
	}
}





void TrajCorrectDock::mousePressEvent(QMouseEvent * event)
{
	QWidget *clickedChild = childAt(event->pos());

	// 如果点击的控件，不是 pickBox，也不是它的子控件
	if (!clickedChild || (clickedChild != pickBox && !pickBox->isAncestorOf(clickedChild))) {
		emit blankAreaClicked();
	}
	else {
		// 点到 pickBox 或其子控件了，正常传递
		QWidget::mousePressEvent(event);
	}
}

void TrajCorrectDock::testSlot()
{
	if (pickBox->isPickModeOn()) {

		return;
	}
	else
	{
		pickBox->pickModeTurnOn();
		std::cout << "testSlot()" << std::endl;
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		m_ptrKit->Doc_start_module(cmd);
		std::cout << "mode: pick points" << std::endl;
		pickBox->setStyleSheet("border: 3px solid blue");
	}
	
}

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
	BSTR trajNames;
	BSTR trajIDs;
	
	m_ptrKit->pq_GetAllDataObjectsByType(PQ_PATH, &trajNames, &trajIDs);
	std::string sNames = (const char*)trajNames;
	std::stringstream ss(sNames);
	std::string item;
	unsigned long id;
	m_vAllPathNames.clear();
	while (std::getline(ss, item, '#')) {
		if (!item.empty()) {
			m_vAllPathNames.push_back(item);
		}
	}
	std::string sIDs = (const char*)trajIDs;
	ss.clear();
	ss.str(sIDs);
	m_vAllPathIDs.clear();
	while (std::getline(ss,item,'#'))
	{
		if (!item.empty()) {
			id = std::stoul(item);
			m_vAllPathIDs.push_back(id);
		}
	}

	ULONG *bufID = nullptr;
	long lenID;
	VARIANT varTrajIDArray;
	m_vAllPointIDs.clear();
	for (size_t j = 0; j < m_vAllPathIDs.size(); j++)
	{
		m_ptrKit->PQAPIGetPointsID(m_vAllPathIDs[j], &varTrajIDArray);

		lenID = varTrajIDArray.parray->rgsabound[0].cElements; //元素个数
		SafeArrayAccessData(varTrajIDArray.parray, (void**)&bufID);  //把地址赋给buffID
		for (size_t i = 0; i < lenID; i++)
		{
			m_vAllPointIDs.push_back(bufID[i]);
			m_mapAllPointIDs[m_vAllPathIDs[j]].push_back(bufID[i]);
		}
		SafeArrayUnaccessData(varTrajIDArray.parray);//释放
	}
	//GetPointInfo();
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
	double xMin, xMax, yMin, yMax, zMin, zMax;
	xMin = (std::min)(range[0], range[1]);
	xMax = (std::max)(range[0], range[1]);
	yMin = (std::min)(range[2], range[3]);
	yMax = (std::max)(range[2], range[3]);
	zMin = (std::min)(range[4], range[5]);
	zMax = (std::max)(range[4], range[5]);
	double x, y, z;
	
	//m_v2dPointsToCorrect.clear();
	//for (const auto& point : m_v2dAllPointsPositions) {
	//	x = point[0];
	//	y = point[1];
	//	z = point[2];

	//	if (x >= xMin && x <= xMax
	//		&& y >= yMin && y <= yMax
	//		&& z >= zMin && z <= zMax)
	//	{
	//		m_v2dPointsToCorrect.push_back(point);
	//	}

	//}
	m_v2dPointsToCorrect.clear();
	m_vPointsToCorrectID.clear();
	for (size_t i = 0; i < m_v2dAllPointsPositions.size(); i++)
	{
		const auto& point = m_v2dAllPointsPositions[i];
		double x = point[0];
		double y = point[1];
		double z = point[2];

		if (x >= xMin && x <= xMax &&
			y >= yMin && y <= yMax &&
			z >= zMin && z <= zMax)
		{
			m_v2dPointsToCorrect.push_back(point);
			m_vPointsToCorrectID.push_back(m_vAllPointIDs[i]);
		}
	}
	GetParentPath();
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

#pragma endregion



#pragma region 测试函数
void TrajCorrectDock::on_btnTest_2_clicked()
{
	modifyPointsPoses(m_vPointsToCorrectID, m_v2dPointsToCorrect);

}

void TrajCorrectDock::on_btnTest_clicked()
{
	double a[6] = { 0,0,0,0,0,0 };
	qDebug() << "btnTest clicked!";
}



#pragma endregion


#pragma region SLOTSs
void TrajCorrectDock::on_chkApplyCor_toggled(bool checked)
{
	bool isChecked = ui->chkApplyCor->isChecked();
	int row = ui->listCorrections->currentRow();
	QListWidgetItem* item = ui->listCorrections->item(row);
	if (!item) return;
	if (isChecked)
	{
		item->setBackground(Qt::white);
		item->setForeground(Qt::black);
	}
	else
	{
		item->setBackground(Qt::white);      // 恢复默认背景
		item->setForeground(Qt::black);  // 显示灰色字体
	}
	
}
#pragma endregion