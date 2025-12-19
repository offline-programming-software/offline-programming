#include "effectiveness_analysis.h"

effectiveness_analysis::effectiveness_analysis(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::effectiveness_analysisClass())
{
	ui->setupUi(this);

	chart_init();
}

effectiveness_analysis::~effectiveness_analysis()
{
	delete ui;
}

void effectiveness_analysis::chart_init()
{
	//创建图表
	QChart *chart = new QChart;

	//创建QStackedBarSeries,堆柱状图
	QStackedBarSeries *series = new QStackedBarSeries;

	//    barSeries->setBarWidth(5);//设置数据棒的宽度

	//创建QBarSet并添加数据
	QBarSet *set1 = new QBarSet("data1");
	QBarSet *set2 = new QBarSet("data2");
	QBarSet *set3 = new QBarSet("data3");

	*set1 << QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100);

	*set2 << QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100);

	*set3 << QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100)
		<< QRandomGenerator::global()->bounded(0, 100);

	//用于横坐标的字符串列表
	QStringList categories;
	categories << QString::fromLocal8Bit("机背喷涂单元") << QString::fromLocal8Bit("左机侧喷涂单元") << QString::fromLocal8Bit("右机侧喷涂单元");

	//用于柱状图的横坐标
	QBarCategoryAxis *axisX = new QBarCategoryAxis;
	axisX->append(categories);
	axisX->setRange(categories[0], categories[categories.length() - 1]);

	//数值型坐标作为纵坐标
	QValueAxis *axisY = new QValueAxis;
	axisY->setRange(0, 300);
	axisY->setTitleText(QString::fromLocal8Bit("时间"));
	axisY->setTickCount(11);
	axisY->setMinorTickCount(10);
	axisY->setLabelFormat("%d");

	//将QBarSet添加到QBarSeries中
	series->append(set1);
	series->append(set2);
	series->append(set3);
	series->setLabelsVisible(true);

	//将QBarSeries添加到图表中
	chart->addSeries(series);

	//将散点的图表放在自定义坐标中
	//chart->setAxisX(axisX, series);
	//chart->setAxisY(axisY, series);

	//将图表加入画布中
	ui->graphicsView->setChart(chart);
}
