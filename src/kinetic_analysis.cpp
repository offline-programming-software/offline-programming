#include "kinetic_analysis.h"


kinetic_analysis::kinetic_analysis(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::kinetic_analysisClass())
{
	ui->setupUi(this);
	connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(chart_init()));
}

kinetic_analysis::~kinetic_analysis()
{
	delete ui;
}

void kinetic_analysis::chart_init()
{
	chart1 = new QChart();
	series1 = new QSplineSeries();
	chartview1 = new QChartView();

	//设置图标标题
	chart1->setTitle(QString::fromLocal8Bit("角度随时间变化图"));

	//曲线属性
	series1->setName(QString::fromLocal8Bit("θ"));
	series1->setColor(Qt::red);
	series1->setPen(QPen(Qt::red, 2));
	chart1->addSeries(series1);


	//设置X轴属性
	QValueAxis *axisX1 = new QValueAxis;
	chart1->addAxis(axisX1, Qt::AlignBottom);
	//axisX->setTickCount(5);
	axisX1->setRange(0, 30);
	axisX1->setTitleText("Time(s)");
	series1->attachAxis(axisX1);

	//设置Y轴属性
	QValueAxis *axisY1 = new QValueAxis;
	chart1->addAxis(axisY1, Qt::AlignLeft);
	axisY1->setRange(0, 90);
	axisY1->setTitleText(QString::fromLocal8Bit("角度"));
	series1->attachAxis(axisY1);
	QList<double> data = { 1.2,2.5,5.0,8.3,11.9,13.9,22.9,28.9 };
	QList<double> data1 = { 12.76, 45.8,64.9,11.9,36.5,74.9,23.9,55.4 };
	for (int i = 0; i < data.size(); i++)
	{
		series1->append(data.at(i), data1.at(i));
	}

	//将图标添加到chartview中
	ui->angle->setChart(chart1);

	chart2 = new QChart();
	series2 = new QSplineSeries();
	chartview2 = new QChartView();

	//设置图标标题
	chart2->setTitle(QString::fromLocal8Bit("角速度随时间变化图"));

	//曲线属性
	series2->setName(QString::fromLocal8Bit("ω"));
	series2->setColor(Qt::red);
	series2->setPen(QPen(Qt::red, 2));
	chart2->addSeries(series2);


	//设置X轴属性
	QValueAxis *axisX2 = new QValueAxis;
	chart2->addAxis(axisX2, Qt::AlignBottom);
	//axisX->setTickCount(5);
	axisX2->setRange(0, 30);
	axisX2->setTitleText("Time(s)");
	series2->attachAxis(axisX2);

	//设置Y轴属性
	QValueAxis *axisY2 = new QValueAxis;
	chart2->addAxis(axisY2, Qt::AlignLeft);
	axisY2->setRange(0, 90);
	axisY2->setTitleText(QString::fromLocal8Bit("角速度"));
	series2->attachAxis(axisY2);
	QList<double> data2 = { 1.2,2.5,5.0,8.3,11.9,13.9,22.9,28.9 };
	QList<double> data3 = { 12.76, 45.8,64.9,11.9,36.5,74.9,23.9,55.4 };
	for (int i = 0; i < data.size(); i++)
	{
		series2->append(data.at(i), data1.at(i));
	}
	ui->speed->setChart(chart2);

	chart3 = new QChart();
	series3 = new QSplineSeries();
	chartview3 = new QChartView();

	//设置图标标题
	chart3->setTitle(QString::fromLocal8Bit("角加速度随时间变化图"));

	//曲线属性
	series3->setName("a");
	series3->setColor(Qt::red);
	series3->setPen(QPen(Qt::red, 2));
	chart3->addSeries(series3);


	//设置X轴属性
	QValueAxis *axisX = new QValueAxis;
	chart3->addAxis(axisX, Qt::AlignBottom);
	//axisX->setTickCount(5);
	axisX->setRange(0, 30);
	axisX->setTitleText("Time(s)");
	series3->attachAxis(axisX);

	//设置Y轴属性
	QValueAxis *axisY = new QValueAxis;
	chart3->addAxis(axisY, Qt::AlignLeft);
	axisY->setRange(0, 90);
	axisY->setTitleText(QString::fromLocal8Bit("角加速度"));
	series3->attachAxis(axisY);
	QList<double> data4 = { 1.2,2.5,5.0,8.3,11.9,13.9,22.9,28.9 };
	QList<double> data5 = { 12.76, 45.8,64.9,11.9,36.5,74.9,23.9,55.4 };
	for (int i = 0; i < data.size(); i++)
	{
		series3->append(data.at(i), data1.at(i));
	}
	ui->acceleration->setChart(chart3);
}
