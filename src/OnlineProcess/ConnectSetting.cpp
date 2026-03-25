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
	itemList << QString::fromLocal8Bit("机器人") << QString::fromLocal8Bit("测量设备") << QString::fromLocal8Bit("agv");
	ui->comboBox->addItems(itemList);
	ui->lineEdit->setInputMask("000.000.000.000;_");

}

void ConnectSetting::initTree()
{
	QList<QString> head;
	head << QString::fromLocal8Bit("设备类型") << QString::fromLocal8Bit("设备名") << QString::fromLocal8Bit("连接状态");
	
	ui->treeWidget->setHeaderLabels(head);
}
