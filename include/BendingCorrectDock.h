#pragma once

#include <QWidget>
#include "ui_BendingCorrectDock.h"
#include "Correction.h"
#include "PQKitCallback.h"
#include <qdockwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui { class BendingCorrectDockClass; };
QT_END_NAMESPACE

class BendingCorrectDock : public QDockWidget
{
	Q_OBJECT

public:
	BendingCorrectDock(CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback* ptrKitCallback, QWidget *parent = nullptr);
	~BendingCorrectDock();

private:
	Ui::BendingCorrectDockClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;
	QVector<QListWidgetItem*> m_itemList;
	QVector<Correction> m_correctionList;

private slots:
	void on_btnOK_clicked();
	void on_btnDelete_clicked();
	void on_btnNew_clicked();
	void on_listWidget_itemChanged(QListWidgetItem* item);
};

