#pragma once

#include <QtCharts>
#include <Qtcharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QDialog>
#include "ui_effectiveness_analysis.h"

QT_BEGIN_NAMESPACE
namespace Ui { class effectiveness_analysisClass; };
QT_END_NAMESPACE

class effectiveness_analysis : public QDialog
{
	Q_OBJECT

public:
	effectiveness_analysis(QWidget *parent = nullptr);
	~effectiveness_analysis();

private:
	Ui::effectiveness_analysisClass *ui;
	QChart *chart;
	QSplineSeries *series;
	QChartView *chartview;

private slots:
	void chart_init();
};
