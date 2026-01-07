#include "postProcessing.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>

postProcessing::postProcessing(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::postProcessingClass())
{
	ui->setupUi(this);

	QFont font;
	font.setFamily("Microsoft YaHei");
	this->setFont(font);
	this->setWindowState(Qt::WindowMaximized);

	// 默认保存路径为桌面
	savePath = QDir::homePath() + "/Desktop";

	initUI();
}

postProcessing::~postProcessing()
{
	delete ui;
}

void postProcessing::initUI()
{
	// 主分割窗口
	mainSplitter = new QSplitter(Qt::Horizontal, this);

	// 树控件
	treeWidget = new QTreeWidget(this);
	treeWidget->setHeaderLabel(QString::fromLocal8Bit("节点列表"));
	treeWidget->setColumnCount(1);
	treeWidget->header()->setStretchLastSection(false);
	treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	// 文本编辑器 - 设置为可编辑模式
	textEdit = new QTextEdit(this);
	textEdit->setReadOnly(false);
	textEdit->setPlaceholderText(QString::fromLocal8Bit("选择机器人节点生成所有组文件，选择组节点查看和编辑内容"));

	// 添加到主分割窗口
	mainSplitter->addWidget(treeWidget);
	mainSplitter->addWidget(textEdit);

	// 设置分割比例
	mainSplitter->setStretchFactor(0, 1);
	mainSplitter->setStretchFactor(1, 3);

	// 选择路径按钮
	selectPathButton = new QPushButton(QString::fromLocal8Bit("选择保存路径"), this);
	selectPathButton->setFixedHeight(35);
	connect(selectPathButton, &QPushButton::clicked, this, &postProcessing::onSelectSavePathClicked);

	// 生成文件按钮
	generateButton = new QPushButton(QString::fromLocal8Bit("生成机器人后置文件"), this);
	generateButton->setFixedHeight(35);
	connect(generateButton, &QPushButton::clicked, this, &postProcessing::onGenerateRobotFilesClicked);

	// 主布局
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(mainSplitter);

	// 按钮和路径标签放在底部
	QHBoxLayout *buttonLayout = new QHBoxLayout();

	// 路径选择按钮和标签
	QLabel *pathLabel = new QLabel(QString::fromLocal8Bit("路径: ") + savePath, this);
	pathLabel->setWordWrap(true);
	pathLabel->setMinimumWidth(300); 

 // 添加弹簧，将按钮推到右侧
	buttonLayout->addWidget(pathLabel);
	buttonLayout->addStretch();
	buttonLayout->addWidget(selectPathButton);
	buttonLayout->addWidget(generateButton);

	// 设置底部布局的边距和对齐方式
	buttonLayout->setContentsMargins(10, 10, 10, 10);
	buttonLayout->setAlignment(Qt::AlignRight);

	mainLayout->addLayout(buttonLayout);

	// 设置布局
	setLayout(mainLayout);

	// 连接信号槽
	connect(treeWidget, &QTreeWidget::itemClicked, this, &postProcessing::onTreeItemClicked);
}

void postProcessing::onSelectSavePathClicked()
{
	QString selectedPath = QFileDialog::getExistingDirectory(this,
		QString::fromLocal8Bit("选择文件路径"),
		savePath);

	if (!selectedPath.isEmpty()) {
		savePath = selectedPath;
		// 更新路径显示
		QLabel *pathLabel = findChild<QLabel*>();
		if (pathLabel) {
			pathLabel->setText(QString::fromLocal8Bit("路径: ") + savePath);
		}
	}
}

void postProcessing::onGenerateRobotFilesClicked()
{
	// 检查是否有机器人节点
	if (parentIdToItemMap.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("没有可用的机器人节点"));
		return;
	}

	// 检查保存路径
	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("请选择保存路径"));
		return;
	}

	bool allSuccess = true;
	int successCount = 0;
	int totalCount = parentIdToItemMap.size();

	// 遍历所有机器人节点并生成文件
	QMapIterator<int, QTreeWidgetItem*> it(parentIdToItemMap);
	while (it.hasNext()) {
		it.next();
		int robotNodeId = it.key();

		if (generateRobotPostFiles(robotNodeId)) {
			successCount++;
		}
		else {
			allSuccess = false;
		}
	}

	// 显示生成结果
	if (allSuccess) {
		QMessageBox::information(this, QString::fromLocal8Bit("成功"),
			QString::fromLocal8Bit("成功生成所有机器人的后置文件\n共生成 %1 个机器人的文件").arg(successCount));
	}
	else {
		QMessageBox::information(this, QString::fromLocal8Bit("完成"),
			QString::fromLocal8Bit("文件生成完成\n成功: %1 个, 失败: %2 个")
			.arg(successCount).arg(totalCount - successCount));
	}
}


bool postProcessing::generateRobotPostFiles(int robotNodeId)
{
	// 获取机器人名称
	QString robotName = getNodeName(robotNodeId);
	if (robotName.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("无法获取机器人名称"));
		return false;
	}

	// 判断是否为AGV小车
	bool isAGV = robotName.contains("AGV", Qt::CaseInsensitive);
	QString positionFileSuffix = isAGV ? ".agv" : ".txt";

	// 获取用户设置的组 - 移除了空组检查
	QMap<int, QString> groups = getRobotGroups(robotNodeId);

	// 检查保存路径
	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("请选择保存路径"));
		return false;
	}

	// 创建机器人文件夹
	QString robotFolderPath = savePath + "/" + robotName;
	QDir robotDir(robotFolderPath);

	// 删除已存在的机器人文件夹内容
	if (robotDir.exists()) {
		// 删除文件夹内的所有内容
		QDirIterator it(robotFolderPath, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fileInfo = it.fileInfo();
			if (fileInfo.isDir()) {
				QDir dir(fileInfo.absoluteFilePath());
				dir.removeRecursively();
			}
			else {
				QFile::remove(fileInfo.absoluteFilePath());
			}
		}
	}

	// 创建文件夹
	if (!robotDir.mkpath(".")) {
		QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
			QString::fromLocal8Bit("无法创建文件夹: %1").arg(robotFolderPath));
		return false;
	}

	// 如果没有组，创建一个默认组
	if (groups.isEmpty()) {
		// 创建默认组
		groups[0] = "DefaultGroup";
	}

	// 为每个组生成文件
	bool allSuccess = true;
	QStringList generatedFiles;

	QMapIterator<int, QString> it(groups);
	while (it.hasNext()) {
		it.next();
		int groupId = it.key();
		QString groupName = it.value();

		// 生成轨迹内容
		QString trajectoryContent = generateTrajectoryContentForGroup(groupId);
		// 即使轨迹内容为空也继续生成文件

		// 生成位置内容
		QString positionContent = generatePositionContentForGroup(groupId);

		// 创建组文件夹
		QString groupFolderName = robotName + "_" + groupName;
		QString groupFolderPath = robotFolderPath + "/" + groupFolderName;
		QDir groupDir(groupFolderPath);
		if (!groupDir.mkpath(".")) {
			QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
				QString::fromLocal8Bit("无法创建组文件夹: %1").arg(groupFolderPath));
			allSuccess = false;
			continue;
		}

		// 写入轨迹文件
		QString trajectoryFilePath = groupFolderPath + "/" + groupFolderName + "_trajectory.txt";
		QFile trajectoryFile(trajectoryFilePath);
		if (!trajectoryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
				QString::fromLocal8Bit("无法创建轨迹文件: %1").arg(trajectoryFilePath));
			allSuccess = false;
			continue;
		}

		QTextStream trajectoryStream(&trajectoryFile);
		trajectoryStream.setCodec("UTF-8");
		trajectoryStream << trajectoryContent;
		trajectoryFile.close();

		// 写入位置文件
		QString positionFilePath = groupFolderPath + "/" + groupFolderName + "_position.agv";
		QFile positionFile(positionFilePath);
		if (!positionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
				QString::fromLocal8Bit("无法创建位置文件: %1").arg(positionFilePath));
			allSuccess = false;
			continue;
		}

		QTextStream positionStream(&positionFile);
		positionStream.setCodec("UTF-8");
		positionStream << positionContent;
		positionFile.close();

		generatedFiles << groupFolderPath;
	}

	return allSuccess;
}

QMap<int, QString> postProcessing::getRobotGroups(int robotNodeId) const
{
	QMap<int, QString> groups;

	// 查找对应的父节点
	if (!parentIdToItemMap.contains(robotNodeId)) {
		return groups; // 返回空组
	}

	QTreeWidgetItem *robotItem = parentIdToItemMap[robotNodeId];

	// 遍历子节点（组节点）
	for (int i = 0; i < robotItem->childCount(); i++) {
		QTreeWidgetItem *groupItem = robotItem->child(i);
		int groupId = groupItem->data(0, Qt::UserRole).toInt();
		QString groupName = groupItem->text(0);
		groups[groupId] = groupName;
	}

	return groups;
}

QString postProcessing::generateTrajectoryContentForGroup(int groupNodeId)
{
	// 直接获取节点内容，如果没有内容返回默认轨迹
	if (nodeIdToContentMap.contains(groupNodeId)) {
		QString content = nodeIdToContentMap[groupNodeId];
		if (!content.isEmpty()) {
			return content;
		}
	}

	// 返回默认的轨迹文件内容
	return QString::fromLocal8Bit("; 轨迹信息文件\n; 该组没有配置具体的轨迹信息\n");
}


QString postProcessing::generatePositionContentForGroup(int groupNodeId)
{
	QString positionContent;
	int positionCount = 0;

	// 检查是否有子节点（位置信息节点）
	if (!childIdToItemMap.contains(groupNodeId)) {
		// 如果没有子节点，返回默认的位置文件内容
		return QString::fromLocal8Bit("; 位置信息文件\n; 该组没有配置具体的位置点\n");
	}

	QTreeWidgetItem *groupItem = childIdToItemMap[groupNodeId];

	// 遍历子节点（位置信息节点）
	for (int i = 0; i < groupItem->childCount(); i++) {
		QTreeWidgetItem *positionItem = groupItem->child(i);
		int positionId = positionItem->data(0, Qt::UserRole).toInt();
		QString positionText = getNodeContent(positionId);

		if (!positionText.isEmpty()) {
			positionContent += QString::fromLocal8Bit("; 位置信息 %1\n").arg(++positionCount);
			positionContent += positionText + "\n\n";
		}
	}

	// 如果没有位置信息，返回默认内容
	if (positionContent.isEmpty()) {
		positionContent = QString::fromLocal8Bit("; 位置信息文件\n; 该组没有配置具体的位置点\n");
	}

	return positionContent;
}

bool postProcessing::savePostFile(const QString &trajectoryContent, const QString &positionContent)
{
	if (robotName.isEmpty() || pathGroupName.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("机器人名称或路径组名称未设置，无法生成文件"));
		return false;
	}

	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("请选择保存路径"));
		return false;
	}

	// 根据机器人名称判断是否为AGV小车
	bool isAGV = robotName.contains("AGV", Qt::CaseInsensitive);
	QString positionFileSuffix = isAGV ? ".agv" : ".txt";

	// 创建机器人文件夹
	QString robotFolderPath = savePath + "/" + robotName;
	QDir robotDir(robotFolderPath);
	if (!robotDir.exists()) {
		if (!robotDir.mkpath(".")) {
			QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
				QString::fromLocal8Bit("无法创建机器人文件夹: %1").arg(robotFolderPath));
			return false;
		}
	}

	// 路径组文件夹名称
	QString groupFolderName = robotName + "_" + pathGroupName;
	QString groupFolderPath = robotFolderPath + "/" + groupFolderName;
	QDir groupDir(groupFolderPath);

	// 删除已存在的路径组文件夹（清理旧文件）
	if (groupDir.exists()) {
		if (!groupDir.removeRecursively()) {
			QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
				QString::fromLocal8Bit("无法删除旧文件: %1").arg(groupFolderPath));
			return false;
		}
	}

	// 重新创建路径组文件夹
	if (!groupDir.mkpath(".")) {
		QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
			QString::fromLocal8Bit("无法创建路径组文件夹: %1").arg(groupFolderPath));
		return false;
	}

	// 创建轨迹文件（保持.txt后缀）
	QString trajectoryFilePath = groupFolderPath + "/" + groupFolderName + "_trajectory.txt";
	QFile trajectoryFile(trajectoryFilePath);
	if (!trajectoryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
			QString::fromLocal8Bit("无法创建轨迹文件: %1").arg(trajectoryFilePath));
		return false;
	}

	QTextStream trajectoryStream(&trajectoryFile);
	trajectoryStream.setCodec("UTF-8");
	trajectoryStream << trajectoryContent;
	trajectoryFile.close();

	// 创建位置信息文件（根据是否为AGV决定后缀）
	QString positionFilePath = groupFolderPath + "/" + groupFolderName + "_position" + positionFileSuffix;
	QFile positionFile(positionFilePath);
	if (!positionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromLocal8Bit("错误"),
			QString::fromLocal8Bit("无法创建位置文件: %1").arg(positionFilePath));
		return false;
	}

	QTextStream positionStream(&positionFile);
	positionStream.setCodec("UTF-8");
	positionStream << positionContent;
	positionFile.close();

	return true;
}

int postProcessing::addParentNode(const QString &name, int id)
{
	if (parentIdToItemMap.contains(id)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("节点ID %1 已存在").arg(id));
		return -1;
	}

	QTreeWidgetItem *parentItem = new QTreeWidgetItem(treeWidget);
	parentItem->setText(0, name);
	parentItem->setData(0, Qt::UserRole, id);
	parentItem->setFlags(parentItem->flags() | Qt::ItemIsEnabled);

	parentIdToItemMap[id] = parentItem;
	nodeIdToNameMap[id] = name;

	return id;
}

int postProcessing::addChildNode(int parentId, const QString &name, int childId, const QString &content)
{
	if (!parentIdToItemMap.contains(parentId)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("父节点ID %1 不存在").arg(parentId));
		return -1;
	}

	if (childIdToItemMap.contains(childId)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("子节点ID %1 已存在").arg(childId));
		return -1;
	}

	QTreeWidgetItem *parentItem = parentIdToItemMap[parentId];
	QTreeWidgetItem *childItem = new QTreeWidgetItem(parentItem);
	childItem->setText(0, name);
	childItem->setData(0, Qt::UserRole, childId);
	childItem->setFlags(childItem->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	childIdToItemMap[childId] = childItem;
	nodeIdToNameMap[childId] = name;
	nodeIdToContentMap[childId] = content;

	// 展开父节点
	parentItem->setExpanded(true);

	return childId;
}

int postProcessing::addSubChildNode(int parentChildId, const QString &name, int subChildId, const QString &content)
{
	if (!childIdToItemMap.contains(parentChildId)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("子节点ID %1 不存在").arg(parentChildId));
		return -1;
	}

	if (subChildIdToItemMap.contains(subChildId) || childIdToItemMap.contains(subChildId) || parentIdToItemMap.contains(subChildId)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("子子节点ID %1 已存在").arg(subChildId));
		return -1;
	}

	QTreeWidgetItem *parentChildItem = childIdToItemMap[parentChildId];
	QTreeWidgetItem *subChildItem = new QTreeWidgetItem(parentChildItem);
	subChildItem->setText(0, name);
	subChildItem->setData(0, Qt::UserRole, subChildId);
	subChildItem->setFlags(subChildItem->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	subChildIdToItemMap[subChildId] = subChildItem;
	nodeIdToNameMap[subChildId] = name;
	nodeIdToContentMap[subChildId] = content;

	// 展开子节点
	parentChildItem->setExpanded(true);

	return subChildId;
}

void postProcessing::addChildContent(int childId, const QString &content)
{
	if (!childIdToItemMap.contains(childId)) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("子节点ID %1 不存在").arg(childId));
		return;
	}

	nodeIdToContentMap[childId] = content;
}

void postProcessing::clearAllNodes()
{
	treeWidget->clear();
	nodeIdToContentMap.clear();
	nodeIdToNameMap.clear();
	parentIdToItemMap.clear();
	childIdToItemMap.clear();
	subChildIdToItemMap.clear();
	textEdit->clear();
}

QString postProcessing::getNodeContent(int nodeId) const
{
	if (nodeIdToContentMap.contains(nodeId)) {
		return nodeIdToContentMap[nodeId];
	}
	return QString();
}

QString postProcessing::getNodeName(int nodeId) const
{
	if (nodeIdToNameMap.contains(nodeId)) {
		return nodeIdToNameMap[nodeId];
	}
	return QString();
}

void postProcessing::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
	if (!item) return;

	int nodeId = item->data(0, Qt::UserRole).toInt();
	QString nodeName = item->text(0);

	// 保存之前选中的子节点的内容到文本编辑框
	static int lastSelectedNodeId = -1;
	if (lastSelectedNodeId != -1 && lastSelectedNodeId != nodeId) {
		QString currentText = textEdit->toPlainText();
		if (nodeIdToContentMap.contains(lastSelectedNodeId)) {
			nodeIdToContentMap[lastSelectedNodeId] = currentText;
		}
	}

	lastSelectedNodeId = nodeId;

	// 检查是否为父节点（机器人节点，没有父节点）
	if (item->parent() == nullptr) {
		// 机器人节点：显示提示信息，内容不可编辑
		textEdit->clear();
		textEdit->setReadOnly(true);
		textEdit->setPlaceholderText(QString::fromLocal8Bit("这是机器人节点，点击\"生成机器人后置文件\"按钮将生成该机器人下所有组的文件"));
	}
	else {
		// 子节点或子子节点：显示内容并可编辑
		QString content = getNodeContent(nodeId);
		textEdit->setPlainText(content);
		textEdit->setReadOnly(false);  // 确保内容可编辑

		emit nodeSelected(nodeId, nodeName, content);
	}
}