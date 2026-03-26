#include "ConnectSetting.h"

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
	
}
