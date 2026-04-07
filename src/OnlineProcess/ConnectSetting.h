#pragma once
#include <QWidget>
#include "ui_ConnectSetting.h"
#include <zmq.hpp>
#include <memory>
#include <qthread.h>
#include <QTreeWidgetItem> // 新增
#include "OnlineProcess/model/clientModel.h"
#include "OnlineProcess/client.h"

class ZmqWorker;

QT_BEGIN_NAMESPACE
namespace Ui { class ConnectSettingClass; };
QT_END_NAMESPACE

class ConnectSetting : public QWidget
{
	Q_OBJECT

public:
	ConnectSetting(zmq::context_t &ctx,QWidget* parent = nullptr);
	~ConnectSetting();

private:
	Ui::ConnectSettingClass* ui;
	void initUI();
	void initTree();
	void updateDeviceTree(const QString& deviceType, const QString& deviceName, const QString& status); // 新增树更新函数
	QThread m_workerThread;
	ZmqWorker* m_worker;
	ClientModel* m_clientModel;
	zmq::context_t& m_zmqContext;   // ZeroMQ上下文对象

private slots:
	void on_chkLocal_toggled(bool checked);
	void on_btnConnect_clicked();
	void on_btnAdd_clicked();
	void on_connect_result(bool flag);

};
