#include "BendingCorrectDock.h"

BendingCorrectDock::BendingCorrectDock(
		CComPtr<IPQPlatformComponent> ptrKit,
		CPQKitCallback* ptrKitCallback, 
		QWidget* parent
)
		: QDockWidget(parent)
		, m_ptrKit(ptrKit)
		, m_ptrKitCallback(ptrKitCallback)
		, ui(new Ui::BendingCorrectDockClass())
{
	this->setFixedWidth(300);
	QScrollArea* scrollArea = new QScrollArea(this);
	QWidget* contentWidget = new QWidget(this);
	ui->setupUi(contentWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setWidget(contentWidget);
	
	
}

BendingCorrectDock::~BendingCorrectDock()
{
	delete ui;
}



#pragma region SLOTS
void BendingCorrectDock::on_btnDelete_clicked()
{

}

void BendingCorrectDock::on_btnNew_clicked()
{
	QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
	QString itemName = QString::fromLocal8Bit("新建变形函数");
	if (m_correctionList.size() == 0)
	{
		item->setText(itemName);
	}
	for(int i = 0; i < m_itemList.size(); ++i)
	{
		if(m_correctionList[i].m_name == itemName)
		{
			item->setText(QString::fromLocal8Bit("新建变形函数%1").arg(i + 1));
		}
	}
	ui->edtName->setText(itemName);
	ui->btnNew->setEnabled(false);
	item->setSelected(true);
}

void BendingCorrectDock::on_listWidget_itemChanged(QListWidgetItem* item)
{
	ui->edtName->setText(item->text());
}



void BendingCorrectDock::on_btnOK_clicked()  //存取变形函数；
{
	Correction newCor;
	newCor.setName(ui->edtName->text());
	ui->btnNew->setEnabled(true);

	m_correctionList.push_back(newCor);
}
#pragma endregion