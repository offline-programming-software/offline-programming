#include "pickWidget.h"
#include <QContextMenuEvent> //右键菜单事件
#include <QMessageBox>
#include <QMenu>
#include<iostream>
pickWidget::pickWidget(QWidget *parent)
	: QListWidget(parent)
{}

pickWidget::~pickWidget()
{}

bool pickWidget::isPickModeOn()
{
	return isPickModeOnFlag;
}

void pickWidget::pickModeTurnOn()
{
	isPickModeOnFlag = true;
}

void pickWidget::pickModeTurnOff()
{
	isPickModeOnFlag = false;
}

int pickWidget::itemCount()
{
	return itemCounts;
}

void pickWidget::initItemCounts()
{
	itemCounts = 0 ;
}

void pickWidget::addItemCounts()
{
	itemCounts += 1;
}




void pickWidget::setPoints(const std::vector<double>& points)
{
	//清空现有items
	this->clear();
	//循环添加item
	int numPoints = points.size() / 3;
	for (int i = 0; i < numPoints; ++i) {
		this->addItem(QStringLiteral("点%1").arg(i + 1));
	}

}

void pickWidget::contextMenuEvent(QContextMenuEvent * event)
{
	m_currentItem = itemAt(event->pos());

	// 创建菜单
	QMenu menu(this);
	if (m_currentItem)
	{
		QAction *deleteAction = menu.addAction("delete");
		QAction *clearAction = menu.addAction("clear");
		connect(deleteAction, &QAction::triggered, this, &pickWidget::deleteItem);
		connect(clearAction, &QAction::triggered, this, &pickWidget::clearAll);
	}
	menu.exec(event->globalPos());   //globalPos表示在当前处
}

void pickWidget::clearAll()
{
	
	emit clearSignal();
}

void pickWidget::deleteItem()
{
	
}