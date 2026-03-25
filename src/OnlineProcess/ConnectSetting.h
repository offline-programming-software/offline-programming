#pragma once



//检查架次信息是否同步


#include <QWidget>
#include "ui_ConnectSetting.h"
#include "Device.h"

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
};
