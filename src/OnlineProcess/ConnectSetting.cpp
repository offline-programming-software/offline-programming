#include "ConnectSetting.h"
#include <QMessageBox>
#include <QDebug>
#include "OnlineProcess/worker/zmqWorker.h"
#include <qprogressbar.h>
#include <qprogressdialog.h>

ConnectSetting::ConnectSetting(QWidget* parent)
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
	m_worker = new ZmqWorker();
	m_worker->moveToThread(&m_workerThread);
	connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
	connect(m_worker, &ZmqWorker::serverStarted, this, [this](bool success, const QString& addr, const QString& errorMsg) {
		if (success) {
			QMessageBox::information(this, QString::fromLocal8Bit("连接提示"), QString::fromLocal8Bit("成功启动服务绑定并监听: ") + addr);
		}
		else {
			QMessageBox::critical(this, QString::fromLocal8Bit("ZMQ 错误"), QString::fromLocal8Bit("ZMQ 初始化或绑定失败: ") + errorMsg);
		}
		});

	// 5. 监听 Worker 收到的消息并在树上显示
	connect(m_worker, &ZmqWorker::messageReceived, this, [this](const QString& msg) {
		qDebug() << "收到客户端消息:" << msg;

		// 简单的解析机制：假设客户端发来的消息格式是 "设备类型,设备名称" (例如 "Robot,KUKA_1")
		// 如果你想显示所有收到的信息，可以直接把整个 msg 当作设备名
		QStringList parts = msg.split(",");
		QString type = QString::fromLocal8Bit("未知设备");
		QString name = msg;

		if (parts.size() >= 2) {
			type = parts[0];
			name = parts[1];
		}

		// 更新树节点，状态标记为“已连接”
		updateDeviceTree(type, name, QString::fromLocal8Bit("已连接"));
		});

	// 6. 启动子线程 (此时 worker 还没开始工作，只是所在的线程跑起来了)
	m_workerThread.start();
}

ConnectSetting::~ConnectSetting()
{
	if (m_worker) {
		// 跨线程通知 Worker 停止循环并清理 ZMQ
		QMetaObject::invokeMethod(m_worker, "stopServer", Qt::QueuedConnection);
	}
	// 停止线程并等待其安全退出
	m_workerThread.quit();
	m_workerThread.wait();
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

	ui->treeClient->setHeaderLabels(head);
}

// === 新增的添加树节点函数实现 ===
void ConnectSetting::updateDeviceTree(const QString& deviceType, const QString& deviceName, const QString& status)
{
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
	}
	else if (IP.isEmpty() || IP == "...") {
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请输入有效的IP地址或勾选本地"));
		return;
	}

	if (Port.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请输入端口号"));
		return;
	}

	QString address = QString("tcp://%1:%2").arg(IP).arg(Port);

	// 不要直接调用 m_worker->startServer(address)，会阻塞主线程!
	// 必须通过 invokeMethod 触发排队调用，让它是子线程里执行
	QMetaObject::invokeMethod(m_worker, "startServer", Qt::QueuedConnection, Q_ARG(QString, address));
	QProgressDialog* dialog = new QProgressDialog("正在处理数据...", "取消", 0, 0, this);
	dialog->setWindowTitle("请稍候");
	dialog->setWindowModality(Qt::WindowModal); // 模态对话框，防止用户操作主窗口
	dialog->show();
}

void ConnectSetting::on_btnAdd_clicked()
{
	Client client;
	client.Name = QString::fromLocal8Bit("客户端") + QString::number(m_clientModel->rowCount() + 1);
	client.IP = ui->edtIP->text();
	client.Port = ui->edtPort->text();
	client.status = false; // 默认状态为未连接

	const auto& clients = m_clientModel->getClients();
	m_clientModel->addClient(client);
	
}
