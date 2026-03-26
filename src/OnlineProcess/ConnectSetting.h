#pragma once



//检查架次信息是否同步


#include <QWidget>
#include "ui_ConnectSetting.h"
#include <zmq.hpp>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class ConnectSettingClass; };
QT_END_NAMESPACE

class ConnectSetting : public QWidget
{
	Q_OBJECT

public:
	ConnectSetting(QWidget *parent = nullptr);
	~ConnectSetting();

private:
	Ui::ConnectSettingClass *ui;
	void initUI();
	void initTree();
	
	std::unique_ptr<zmq::context_t> m_zmqContext;
	std::unique_ptr<zmq::socket_t> m_zmqRepSocket;

private slots:
	void on_chkLocal_toggled(bool checked);
	void on_btnConnect_clicked();
};
