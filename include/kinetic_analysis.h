#pragma once


#include <QtCharts>
#include <QDialog>
#include "ui_kinetic_analysis.h"

#include <Qtcharts/QLineSeries>
#include <QtCharts/QValueAxis>


QT_BEGIN_NAMESPACE
namespace Ui { class kinetic_analysisClass; };
QT_END_NAMESPACE

class kinetic_analysis : public QDialog
{
	Q_OBJECT

public:
	kinetic_analysis(QWidget *parent = nullptr);
	~kinetic_analysis();

private:
	Ui::kinetic_analysisClass *ui;
	QChart *chart1;
	QChart *chart2;
	QChart *chart3;
	QSplineSeries *series1;
	QSplineSeries *series2;
	QSplineSeries *series3;
	QChartView *chartview1;
	QChartView *chartview2;
	QChartView *chartview3;

private slots:
	void chart_init();
};
