#pragma once

#include <QDialog>
#include "ui_addRobotSpace.h"

QT_BEGIN_NAMESPACE
namespace Ui { class addRobotSpaceClass; };
QT_END_NAMESPACE

class addRobotSpace : public QDialog
{
	Q_OBJECT

public:
	addRobotSpace(QWidget *parent = nullptr);
	~addRobotSpace();

	void setRobotName(const QString &robotName);
	void setCoordinate(const QStringList &coordList);
	void setRail(const QStringList &railList);

	QString getCoordinate();
	QString getMainDir();
	QString getRail();

	bool isLink();

private slots:
	void onLinkRail(int state);
	void onCancelButtonClicked();
	void onCalculateButtonClicked();  // 添加计算按钮的槽函数

signals:
	void calculateRequested();  // 添加计算信号声明

private:
	Ui::addRobotSpaceClass *ui;
};
