#pragma once

#include <QListWidget>
#include<QMouseEvent>

class pickWidget  : public QListWidget
{
	Q_OBJECT

public:
	pickWidget(QWidget *parent);
	~pickWidget();
	bool isPickModeOn();
	void pickModeTurnOn();
	void pickModeTurnOff();
	int itemCount();
	void initItemCounts();
	void addItemCounts();



private:
	QListWidgetItem *m_currentItem;
	bool isPickModeOnFlag = false;
	int itemCounts= 0;
signals:
	void blankAreaClicked();
	void clearSignal();
	void deleteSignal(int ItemIndex);
protected:
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override {
		QListWidget::mousePressEvent(event);
		QModelIndex index = indexAt(event->pos());
		if (!index.isValid()) { // 点击的是空白区域
			emit blankAreaClicked(); // 发射信号
		}
		
		
	}

private slots:
	void deleteItem();
	void clearAll();

};


//调用过程
//include "pickWidget.h"
//pickWidget *pickPoints = new pickWidget(this);
//ui->USERLayout->addWidget(pickPoints);
//connect(pickPoints, &pickWidget::blankAreaClicked, this, &Mainwindow::SLOTS);
