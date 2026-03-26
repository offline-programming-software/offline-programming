#pragma once
#include <QWidget>
#include "ui_ConnectSetting.h"
#include <zmq.hpp>
#include <memory>
#include <qthread.h>
#include <QTreeWidgetItem> // 新增

struct Client
{
	QString Name;
	QString IP = "0.0.0.0";
	QString Port;
	bool status = false;
};
class ZmqWorker;

QT_BEGIN_NAMESPACE
namespace Ui { class ConnectSettingClass; };
QT_END_NAMESPACE

class ConnectSetting : public QWidget
{
	Q_OBJECT

public:
	ConnectSetting(QWidget* parent = nullptr);
	~ConnectSetting();

private:
	Ui::ConnectSettingClass* ui;
	void initUI();
	void initTree();
	void updateDeviceTree(const QString& deviceType, const QString& deviceName, const QString& status); // 新增树更新函数
	QThread m_workerThread;
	ZmqWorker* m_worker;
	

private slots:
	void on_chkLocal_toggled(bool checked);
	void on_btnConnect_clicked();
	void on_benAdd_clicked();
};
