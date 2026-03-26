#include "ConnectSetting.h"
#include <QMessageBox>
#include <QDebug>

ConnectSetting::ConnectSetting(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ConnectSettingClass())
{
	ui->setupUi(this);
	initUI();
	initTree();
	setWindowTitle(QString::fromLocal8Bit("连接配置"));
	setWindowFlags(Qt::Window
		| Qt::WindowContextHelpButtonHint   // 问号
		| Qt::WindowCloseButtonHint);       // 关闭
}

ConnectSetting::~ConnectSetting()
{
	if (m_zmqRepSocket) {
		m_zmqRepSocket->close();
	}
	if (m_zmqContext) {
		m_zmqContext->close();
	}
	delete ui;
}

void ConnectSetting::initUI()
{
	QList<QString> itemList;
	ui->edtIP->setInputMask("000.000.000.000;_");
	QIntValidator* validator = new QIntValidator(5000, 65536, this);
	ui->edtPort->setValidator(validator);

}

void ConnectSetting::initTree()
{
	QList<QString> head;
	head << QString::fromLocal8Bit("设备类型") << QString::fromLocal8Bit("设备名") << QString::fromLocal8Bit("连接状态");
	
	ui->treeDeviceList->setHeaderLabels(head);
}

void ConnectSetting::on_chkLocal_toggled(bool checked)
{
	if (checked)
	{
		ui->edtIP->setEnabled(false);
		ui->edtIP->setText("");
		ui->edtIP->setInputMask("000.000.000.000;_");
	}
	else
	{
		ui->edtIP->setEnabled(true);
		ui->edtIP->setInputMask("000.000.000.000;_");
	}
}

void ConnectSetting::on_btnConnect_clicked()
{
	QString Port = ui->edtPort->text();
	QString IP = ui->edtIP->text();
	if (ui->chkLocal->isChecked()) {
		IP = "*";
	} else if (IP.isEmpty() || IP == "...") {
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请输入有效的IP地址或勾选本地"));
		return;
	}

	if (Port.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请输入端口号"));
		return;
	}

	try {
		if (!m_zmqContext) {
			m_zmqContext = std::make_unique<zmq::context_t>(1);
		}
		if (!m_zmqRepSocket) {
			m_zmqRepSocket = std::make_unique<zmq::socket_t>(*m_zmqContext, ZMQ_REP);
		}

		QString address = QString("tcp://%1:%2").arg(IP).arg(Port);
		m_zmqRepSocket->bind(address.toStdString());

		QMessageBox::information(this, QString::fromLocal8Bit("连接提示"), QString::fromLocal8Bit("成功启动服务绑定并监听: ") + address);
		qDebug() << "服务端成功绑定并在监听:" << address;

	} catch (const zmq::error_t& e) {
		QMessageBox::critical(this, QString::fromLocal8Bit("ZMQ 错误"), QString::fromLocal8Bit("ZMQ 初始化或绑定失败: ") + QString(e.what()));
	}
}
