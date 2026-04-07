#include "ConnectSetting.h"
#include <QMessageBox>
#include <QDebug>
#include "OnlineProcess/worker/zmqWorker.h"
#include <qprogressbar.h>
#include <qprogressdialog.h>

ConnectSetting::ConnectSetting(zmq::context_t& ctx, QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::ConnectSettingClass()), m_zmqContext(ctx)
{
	ui->setupUi(this);
	initUI();
	initTree();
	setWindowTitle(QString::fromLocal8Bit("连接配置"));
	setWindowFlags(Qt::Window
		| Qt::WindowContextHelpButtonHint   // 问号
		| Qt::WindowCloseButtonHint);       // 关闭
	m_clientModel = new ClientModel(this);

	ui->tblClient->setModel(m_clientModel);
	ui->tblClient->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	// 设置为只能整行选中
	ui->tblClient->setSelectionBehavior(QAbstractItemView::SelectRows);

	// 隐藏左侧自带的行号表头（如果不需要的话）
	ui->tblClient->verticalHeader()->setVisible(false);

	// 创建 Worker 对象并移动到线程
	m_worker = new ZmqWorker(m_zmqContext);
	m_worker->moveToThread(&m_workerThread);
	connect(m_worker, &ZmqWorker::connectResult, this, &ConnectSetting::on_connect_result);

	connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

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
	QModelIndex currentIndex = ui->tblClient->currentIndex();

	if (!currentIndex.isValid()) {
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先选择一个客户端"));
		return;
	}

	int row = currentIndex.row();

	const auto& clients = m_clientModel->getAllClient();
	if (row >= 0 && row < clients.size()) {
		const Client& selectedClient = clients.at(row);

		QString name = selectedClient.name();
		QString ip = selectedClient.ip();
		m_worker->on_connectToServer(ip, selectedClient.port());
		
		qDebug() << "Selected Client:" << name << ip;
	}

}

void ConnectSetting::on_btnAdd_clicked()
{
	QString newName = ui->edtClientName->text();
	// 1. 判断名称是否为空
	if (newName.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请输入主机名称"));
		return;
	}

	// 2. 同名检查
	const auto& existingClients = m_clientModel->getAllClient();
	for (const auto& client : existingClients) {
		if (client.name() == newName) {
			QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("该主机名称已存在，请使用其他名称"));
			return;
		}
	}

	// 3. 构造并添加新 Client
	Client newClient;
	newClient.setName(newName);
	newClient.setPort(ui->edtPort->text());
	newClient.setIp(ui->edtIP->text());
	newClient.setIsLocal(ui->chkLocal->isChecked());

	m_clientModel->addClient(newClient);
}

void ConnectSetting::on_connect_result(bool flag)
{
	QModelIndex currentIndex = ui->tblClient->currentIndex();
	int row = currentIndex.row();
	Client& client = m_clientModel->getClient(row) ;
	if (flag)
	{
		QMessageBox::information(this, QString::fromLocal8Bit("成功"), QString::fromLocal8Bit("连接成功！"));
		client.setIsConnected(true);

	}
	else
	{
		QMessageBox::critical(this, QString::fromLocal8Bit("失败"), QString::fromLocal8Bit("连接失败，请检查IP和端口设置！"));
		client.setIsConnected(false);
	}
}
