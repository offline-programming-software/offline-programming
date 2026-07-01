#include "test/RobxFileIOManagerWidget.h"
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFile>
#include <QMessageBox>
#include <filesystem>
#include <QDateTime>

RobxFileIOManagerWidget::RobxFileIOManagerWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::RobxFileIOManagerWidgetClass())
{
	ui->setupUi(this);

	// Set up tree widget
	QStringList headers;
	headers << u8"文件名" << u8"修改时间";
	ui->treeWidget->setHeaderLabels(headers);
	ui->treeWidget->setColumnCount(2);

	// Connect signals
	connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &RobxFileIOManagerWidget::onTreeItemSelectionChanged);
	connect(ui->btnDelete, &QPushButton::clicked, this, &RobxFileIOManagerWidget::onDeleteButtonClicked);

	// Connect refresh button if it exists
	QPushButton* btnRefresh = findChild<QPushButton*>("btnRefresh");
	connect(btnRefresh, &QPushButton::clicked, this, &RobxFileIOManagerWidget::onRefreshButtonClicked);
	

	// Load temp folder content
	loadTempFolder();
	const QString jsonsPath = "./temp/jsons";
	QTreeWidgetItem* item = findItemByPath(jsonsPath);
	ui->treeWidget->expandItem(item);
	ui->treeWidget->setColumnWidth(0, 300);
}

RobxFileIOManagerWidget::~RobxFileIOManagerWidget()
{
	delete ui;
}

void RobxFileIOManagerWidget::loadTempFolder()
{
	ui->treeWidget->clear();
	m_itemPathMap.clear();

	QString tempPath = "./temp";
	QDir tempDir(tempPath);

	if (!tempDir.exists())
	{
		QMessageBox::information(this, "Info", u8" 当前目录没有temp文件夹" + tempPath);
		return;
	}

	// Create root item for temp folder
	QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidget);
	rootItem->setText(0, "temp");
	m_itemPathMap[rootItem] = tempPath;

	// Recursively populate tree widget
	populateTreeWidget(rootItem, tempPath);
	ui->treeWidget->collapseAll();
	ui->treeWidget->expandItem(rootItem);
		
}

void RobxFileIOManagerWidget::populateTreeWidget(QTreeWidgetItem* parentItem, const QString& folderPath)
{
	QDir dir(folderPath);
	if (!dir.exists())
	{
		return;
	}

	// Get all entries (files and folders)
	QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::DirsFirst);

	for (const QFileInfo& entry : entries)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
		item->setText(0, entry.fileName());
		item->setText(1, entry.lastModified().toString("yyyy-MM-dd HH:mm:ss"));
		m_itemPathMap[item] = entry.absoluteFilePath();

		// If it's a directory, recursively populate it
		if (entry.isDir())
		{
			populateTreeWidget(item, entry.absoluteFilePath());
		}
	}
}

QString RobxFileIOManagerWidget::readFileContent(const QString& filePath)
{
	QFileInfo fileInfo(filePath);

	if (!fileInfo.exists())
	{
		return "Error: File does not exist";
	}

	if (fileInfo.isDir())
	{
		return "This is a folder, not a file";
	}

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return "Error: Cannot open file";
	}

	QString content = QString::fromUtf8(file.readAll());
	file.close();

	return content;
}

QTreeWidgetItem* RobxFileIOManagerWidget::findItemByPath(const QString& path)
{
	for (auto item = m_itemPathMap.constBegin(); item != m_itemPathMap.constEnd(); ++item)
	{
		if (item.value() == path)
		{
			return item.key();
		}
	}
	return nullptr;
}

void RobxFileIOManagerWidget::onTreeItemSelectionChanged()
{
	QTreeWidgetItem* currentItem = ui->treeWidget->currentItem();
	if (!currentItem)
	{
		ui->textBrowser->clear();
		return;
	}

	QString filePath = m_itemPathMap.value(currentItem);
	QString content = readFileContent(filePath);
	ui->textBrowser->setText(content);
}

void RobxFileIOManagerWidget::onDeleteButtonClicked()
{
	QTreeWidgetItem* currentItem = ui->treeWidget->currentItem();
	if (!currentItem)
	{
		QMessageBox::warning(this, "Warning", "Please select an item to delete");
		return;
	}

	// Don't allow deleting the root "temp" folder
	if (currentItem->parent() == nullptr)
	{
		QMessageBox::warning(this, "Warning", u8"不能删除temp文件夹");
		return;
	}

	QString filePath = m_itemPathMap.value(currentItem);
	QFileInfo fileInfo(filePath);

	bool success = false;
	if (fileInfo.isDir())
	{
		// Delete folder and its contents
		QDir dir(filePath);
		success = dir.removeRecursively();
	}
	else
	{
		// Delete file
		QFile file(filePath);
		success = file.remove();
	}

	if (success)
	{
		m_itemPathMap.remove(currentItem);
		delete currentItem;
	}
	else
	{
		QMessageBox::critical(this, "Error", "Failed to delete item");
	}
}

void RobxFileIOManagerWidget::onRefreshButtonClicked()
{
	// Reload the temp folder content
	loadTempFolder();

	// Clear the text browser
	ui->textBrowser->clear();

}

