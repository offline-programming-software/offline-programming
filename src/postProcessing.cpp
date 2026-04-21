#include "postProcessing.h"
#include "robxFileIO.h"
#include "TrajectoryFileGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QSet>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>

postProcessing::postProcessing(QWidget* parent,
	CComPtr<IPQPlatformComponent> ptrKit ,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::postProcessingClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
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

bool postProcessing::loadRobotAndPathData()
{
	clearAllNodes();

	QMap<ULONG, QString> allRobots = getObjectsByType(PQ_ROBOT);
	if (allRobots.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的机器人！");
		return false;
	}

	QMap<ULONG, QString> robotMap;
	for (auto it = allRobots.begin(); it != allRobots.end(); ++it) {
		ULONG robotID = it.key();
		PQRobotType eRobotType = PQ_MECHANISM_ROBOT;
		if (SUCCEEDED(m_ptrKit->Robot_get_type(robotID, &eRobotType)) && eRobotType == PQ_MECHANISM_ROBOT) {
			robotMap.insert(robotID, it.value());
		}
	}

	if (robotMap.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的机械臂机器人！");
		return false;
	}

	bool hasValidData = false;

	for (auto it = robotMap.begin(); it != robotMap.end(); ++it) {
		ULONG robotId = it.key();
		const QString robotName = it.value();

		int parentNodeId = addParentNode(robotName, static_cast<int>(robotId));
		if (parentNodeId == -1) {
			continue;
		}

		VARIANT pathGroupVariant;
		VariantInit(&pathGroupVariant);
		pathGroupVariant.parray = NULL;

		if (SUCCEEDED(m_ptrKit->Doc_get_pathgroup_name(robotId, &pathGroupVariant))) {
			SAFEARRAY* pathGroupArray = pathGroupVariant.parray;
			if (pathGroupArray && pathGroupArray->cDims == 1) {
				ULONG pathGroupCount = pathGroupArray->rgsabound[0].cElements;

				for (ULONG pgIndex = 0; pgIndex < pathGroupCount; ++pgIndex) {
					BSTR pathGroupId = 0;
					if (SUCCEEDED(SafeArrayGetElement(pathGroupArray, (LONG*)&pgIndex, &pathGroupId))) {
						QString pathGroupName = QString::fromWCharArray(pathGroupId);

						QString baseContent = generatePathGroupPostContentInternal(
							robotId, pathGroupId, robotName, pathGroupName);

						QString sprayContent = buildSprayEventSection(robotId, pathGroupName.trimmed());
						QString finalTrajectoryContent = baseContent + sprayContent;

						int groupNodeId = static_cast<int>(robotId * 1000 + pgIndex);
						addChildNode(parentNodeId, pathGroupName, groupNodeId, "");
						addChildContent(groupNodeId, finalTrajectoryContent);

						int trajectoryNodeId = static_cast<int>(robotId * 10000 + pgIndex * 100 + 1);
						addSubChildNode(groupNodeId, QString("轨迹_%1").arg(pathGroupName), trajectoryNodeId, "");
						addChildContent(trajectoryNodeId, finalTrajectoryContent);

						int positionNodeId = static_cast<int>(robotId * 10000 + pgIndex * 100 + 2);
						addSubChildNode(groupNodeId, QString("位置_%1").arg(pathGroupName), positionNodeId, "");
						// 关键：通过 addChildContent 注入 AGV 站位 + AGV 事件
						QString positionExtra;

						ULONG agvId = 0;
						{
							// relations.json: [robotName, railName, agvName]
							QVector<std::tuple<QString, QString, QString>> relations;
							RobxIO io;
							io.updateData(relations, "relations.json");

							QString agvName;
							for (int i = 0; i < relations.size(); ++i) {
								const QString rName = std::get<0>(relations[i]).trimmed();
								if (rName == robotName.trimmed()) {
									agvName = std::get<2>(relations[i]).trimmed();
									break;
								}
							}

							if (!agvName.isEmpty() && agvName != "无") {
								agvId = getObjIdByName(PQ_WORKINGPART, agvName);
							}
						}

						positionExtra += buildAgvStationSection(robotName, pathGroupName);
						positionExtra += buildAgvStationEventSection(agvId, robotName, pathGroupName);
						addChildContent(positionNodeId, positionExtra);

						SysFreeString(pathGroupId);
						hasValidData = true;
					}
				}
			}
		}

		VariantClear(&pathGroupVariant);
	}

	if (!hasValidData) {
		QMessageBox::information(this, "提示", "没有找到有效的路径组数据！");
		return false;
	}

	return true;
}

void postProcessing::initUI()
{
	// 主分割窗口
	mainSplitter = new QSplitter(Qt::Horizontal, this);

	// 树控件
	treeWidget = new QTreeWidget(this);
	treeWidget->setHeaderLabel("节点列表");
	treeWidget->setColumnCount(1);
	treeWidget->header()->setStretchLastSection(false);
	treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	// 文本编辑器 - 设置为可编辑模式
	textEdit = new QTextEdit(this);
	textEdit->setReadOnly(false);
	textEdit->setPlaceholderText("选择机器人节点生成所有组文件，选择组节点查看和编辑内容");

	// 添加到主分割窗口
	mainSplitter->addWidget(treeWidget);
	mainSplitter->addWidget(textEdit);

	// 设置分割比例
	mainSplitter->setStretchFactor(0, 1);
	mainSplitter->setStretchFactor(1, 3);

	// 选择路径按钮
	selectPathButton = new QPushButton("选择保存路径", this);
	selectPathButton->setFixedHeight(35);
	connect(selectPathButton, &QPushButton::clicked, this, &postProcessing::onSelectSavePathClicked);

	// 生成文件按钮
	generateButton = new QPushButton("生成机器人后置文件", this);
	generateButton->setFixedHeight(35);
	connect(generateButton, &QPushButton::clicked, this, &postProcessing::onGenerateRobotFilesClicked);

	// 主布局
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(mainSplitter);

	// 按钮和路径标签放在底部
	QHBoxLayout *buttonLayout = new QHBoxLayout();

	// 路径选择按钮和标签
	QLabel *pathLabel = new QLabel("路径: " + savePath, this);
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
		"选择文件路径",
		savePath);

	if (!selectedPath.isEmpty()) {
		savePath = selectedPath;
		// 更新路径显示
		QLabel *pathLabel = findChild<QLabel*>();
		if (pathLabel) {
			pathLabel->setText("路径: " + savePath);
		}
	}
}

void postProcessing::onGenerateRobotFilesClicked()
{
	// 检查是否有机器人节点
	if (parentIdToItemMap.isEmpty()) {
		QMessageBox::warning(this, "警告",
			"没有可用的机器人节点");
		return;
	}

	// 检查保存路径
	if (savePath.isEmpty()) {
		QMessageBox::warning(this, "警告",
			"请选择保存路径");
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
		QMessageBox::information(this, QString ("成功"),
			QString ("成功生成所有机器人的后置文件\n共生成 %1 个机器人的文件").arg(successCount));
	}
	else {
		QMessageBox::information(this, QString ("完成"),
			QString ("文件生成完成\n成功: %1 个, 失败: %2 个")
			.arg(successCount).arg(totalCount - successCount));
	}
}


bool postProcessing::generateRobotPostFiles(int robotNodeId)
{
	// 获取机器人名称
	QString robotName = getNodeName(robotNodeId);
	if (robotName.isEmpty()) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("无法获取机器人名称"));
		return false;
	}

	// 判断是否为AGV小车
	bool isAGV = robotName.contains("AGV", Qt::CaseInsensitive);
	QString positionFileSuffix = isAGV ? ".agv" : ".txt";

	// 获取用户设置的组 - 移除了空组检查
	QMap<int, QString> groups = getRobotGroups(robotNodeId);

	// 检查保存路径
	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("请选择保存路径"));
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
		QMessageBox::critical(this, "错误",
			QString("无法创建文件夹: %1").arg(robotFolderPath));
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

		// 生成位置内容（原始树节点内容）
		QString positionContent = generatePositionContentForGroup(groupId);

		// 创建组文件夹
		QString groupFolderName = robotName + "_" + groupName;
		QString groupFolderPath = robotFolderPath + "/" + groupFolderName;
		
		QDir groupDir(groupFolderPath);
		if (!groupDir.mkpath(".")) {
			QMessageBox::critical(this, QString ("错误"),
				QString("无法创建组文件夹: %1").arg(groupFolderPath));
			allSuccess = false;
			continue;
		}

		// 写入轨迹文件
		QString trajectoryFilePath = groupFolderPath + "/" + groupFolderName + "_trajectory.txt";
		QFile trajectoryFile(trajectoryFilePath);
		if (!trajectoryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, QString("错误"),
				QString("无法创建轨迹文件: %1").arg(trajectoryFilePath));
			allSuccess = false;
			continue;
		}

		QTextStream trajectoryStream(&trajectoryFile);
		trajectoryStream.setCodec("UTF-8");
		trajectoryStream << trajectoryContent;
		trajectoryFile.close();

		// 写入位置文件
		QString positionFilePath = groupFolderPath + "/" + groupFolderName + "_position" + positionFileSuffix;
		QFile positionFile(positionFilePath);
		if (!positionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, QString ("错误"),
				QString ("无法创建位置文件: %1").arg(positionFilePath));
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
	return QString ("; 轨迹信息文件\n; 该组没有配置具体的轨迹信息\n");
}


QString postProcessing::generatePositionContentForGroup(int groupNodeId)
{
	QString positionContent;
	int positionCount = 0;

	// 检查是否有子节点（位置信息节点）
	if (!childIdToItemMap.contains(groupNodeId)) {
		// 如果没有子节点，返回默认的位置文件内容
		return QString ("; 位置信息文件\n; 该组没有配置具体的位置点\n");
	}

	QTreeWidgetItem *groupItem = childIdToItemMap[groupNodeId];

	// 遍历子节点（位置信息节点）
	for (int i = 0; i < groupItem->childCount(); i++) {
		QTreeWidgetItem *positionItem = groupItem->child(i);
		int positionId = positionItem->data(0, Qt::UserRole).toInt();
		QString positionText = getNodeContent(positionId);

		if (!positionText.isEmpty()) {
			positionContent += QString ("; 位置信息 %1\n").arg(++positionCount);
			positionContent += positionText + "\n\n";
		}
	}

	// 如果没有位置信息，返回默认内容
	if (positionContent.isEmpty()) {
		positionContent = QString ("; 位置信息文件\n; 该组没有配置具体的位置点\n");
	}

	return positionContent;
}

bool postProcessing::savePostFile(const QString &trajectoryContent, const QString &positionContent)
{
	if (robotName.isEmpty() || pathGroupName.isEmpty()) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("机器人名称或路径组名称未设置，无法生成文件"));
		return false;
	}

	if (savePath.isEmpty()) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("请选择保存路径"));
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
			QMessageBox::critical(this, QString ("错误"),
				QString ("无法创建机器人文件夹: %1").arg(robotFolderPath));
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
			QMessageBox::critical(this, QString ("错误"),
				QString ("无法删除旧文件: %1").arg(groupFolderPath));
			return false;
		}
	}

	// 重新创建路径组文件夹
	if (!groupDir.mkpath(".")) {
		QMessageBox::critical(this, QString ("错误"),
			QString ("无法创建路径组文件夹: %1").arg(groupFolderPath));
		return false;
	}

	// 创建轨迹文件（保持.txt后缀）
	QString trajectoryFilePath = groupFolderPath + "/" + groupFolderName + "_trajectory.txt";
	QFile trajectoryFile(trajectoryFilePath);
	if (!trajectoryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString ("错误"),
			QString ("无法创建轨迹文件: %1").arg(trajectoryFilePath));
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
		QMessageBox::critical(this, QString ("错误"),
			QString ("无法创建位置文件: %1").arg(positionFilePath));
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
		QMessageBox::warning(this, QString ("警告"),
			QString ("节点ID %1 已存在").arg(id));
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
		QMessageBox::warning(this, QString ("警告"),
			QString ("父节点ID %1 不存在").arg(parentId));
		return -1;
	}

	if (childIdToItemMap.contains(childId)) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("子节点ID %1 已存在").arg(childId));
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
		QMessageBox::warning(this, QString ("警告"),
			QString ("子节点ID %1 不存在").arg(parentChildId));
		return -1;
	}

	if (subChildIdToItemMap.contains(subChildId) || childIdToItemMap.contains(subChildId) || parentIdToItemMap.contains(subChildId)) {
		QMessageBox::warning(this, QString ("警告"),
			QString ("子子节点ID %1 已存在").arg(subChildId));
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

void postProcessing::addChildContent(int childId, const QString& content)
{
	if (!childIdToItemMap.contains(childId) && !subChildIdToItemMap.contains(childId)) {
		QMessageBox::warning(this, "警告",
			QString("节点ID %1 不存在").arg(childId));
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
		textEdit->setPlaceholderText(QString ("这是机器人节点，点击\"生成机器人后置文件\"按钮将生成该机器人下所有组的文件"));
	}
	else {
		// 子节点或子子节点：显示内容并可编辑
		QString content = getNodeContent(nodeId);
		textEdit->setPlainText(content);
		textEdit->setReadOnly(false);  // 确保内容可编辑

		emit nodeSelected(nodeId, nodeName, content);
	}
}

QList<ULONG> postProcessing::extractULongArrayFromVariant(const VARIANT& variant) const
{
	QList<ULONG> result;

	if ((variant.vt & VT_ARRAY) == 0 || variant.parray == NULL || variant.parray->cDims != 1) {
		return result;
	}

	SAFEARRAY* array = variant.parray;
	VARTYPE vt = VT_EMPTY;
	if (FAILED(SafeArrayGetVartype(array, &vt))) {
		return result;
	}

	LONG lBound = 0;
	LONG uBound = -1;
	if (FAILED(SafeArrayGetLBound(array, 1, &lBound)) || FAILED(SafeArrayGetUBound(array, 1, &uBound))) {
		return result;
	}

	const LONG count = uBound - lBound + 1;
	if (count <= 0) {
		return result;
	}

	if (FAILED(SafeArrayLock(array))) {
		return result;
	}

	switch (vt) {
	case VT_I4:
	{
		LONG* data = static_cast<LONG*>(array->pvData);
		for (LONG i = 0; i < count; ++i) result.append(static_cast<ULONG>(data[i]));
		break;
	}
	case VT_UI4:
	{
		ULONG* data = static_cast<ULONG*>(array->pvData);
		for (LONG i = 0; i < count; ++i) result.append(data[i]);
		break;
	}
	case VT_I8:
	{
		LONGLONG* data = static_cast<LONGLONG*>(array->pvData);
		for (LONG i = 0; i < count; ++i) result.append(static_cast<ULONG>(data[i]));
		break;
	}
	default:
		break;
	}

	SafeArrayUnlock(array);
	return result;
}

QString postProcessing::buildSprayEventSection(ULONG robotId, const QString& pathGroupName) const
{
	QString content;
	QTextStream stream(&content);

	if (!m_ptrKit || robotId == 0 || pathGroupName.trimmed().isEmpty()) {
		return content;
	}

	VARIANT pathNamesVar;
	VARIANT pathIdsVar;
	VariantInit(&pathNamesVar);
	VariantInit(&pathIdsVar);
	pathNamesVar.parray = NULL;
	pathIdsVar.parray = NULL;

	CComBSTR groupBstr(pathGroupName.toStdWString().c_str());
	HRESULT hr = m_ptrKit->Path_get_group_path(robotId, groupBstr, &pathNamesVar, &pathIdsVar);
	if (FAILED(hr) || !pathIdsVar.parray) {
		VariantClear(&pathNamesVar);
		VariantClear(&pathIdsVar);
		return content;
	}

	QList<ULONG> pathIds;
	SAFEARRAY* pathIDArray = pathIdsVar.parray;
	LONG* pathIdData = NULL;
	if (SUCCEEDED(SafeArrayAccessData(pathIDArray, (void**)&pathIdData)) && pathIdData) {
		ULONG pathCount = pathIDArray->rgsabound[0].cElements;
		for (ULONG i = 0; i < pathCount; ++i) {
			pathIds.append(static_cast<ULONG>(pathIdData[i]));
		}
		SafeArrayUnaccessData(pathIDArray);
	}

	VariantClear(&pathNamesVar);
	VariantClear(&pathIdsVar);

	stream << QString::fromStdString(u8"\n; ===== 喷涂事件 =====\n");

	int eventCount = 0;
	for (int i = 0; i < pathIds.size(); ++i) {
		const ULONG pathId = pathIds.at(i);

		int nPointsCount = 0;
		ULONG* ulPointsIDs = nullptr;
		HRESULT hrPts = m_ptrKit->Path_get_point_id(pathId, &nPointsCount, &ulPointsIDs);
		if (FAILED(hrPts) || nPointsCount <= 0 || !ulPointsIDs) {
			if (ulPointsIDs) {
				m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);
			}
			continue;
		}

		for (int p = 0; p < nPointsCount; ++p) {
			const ULONG pointId = ulPointsIDs[p];

			WCHAR* wsName = nullptr;
			ULONG executeDeviceId = 0;
			LONG pointForward = 1;
			WCHAR* wsEventTemplate = nullptr;
			WCHAR* wsEventContent = nullptr;
			LONG bUnion = 0;
			ULONG* nozzleIds = nullptr;
			ULONG* assistorIds = nullptr;
			INT assistorCount = 0;

			HRESULT ehr = m_ptrKit->Point_get_spray_event_content(
				pointId, &wsName, &executeDeviceId, &pointForward,
				&wsEventTemplate, &wsEventContent, &bUnion,
				&nozzleIds, &assistorIds, &assistorCount);

			if (SUCCEEDED(ehr)) {
				const QString eventName = wsEventTemplate ? QString::fromWCharArray(wsEventTemplate)
					: (wsName ? QString::fromWCharArray(wsName) : QString());
				const QString eventContent = wsEventContent ? QString::fromWCharArray(wsEventContent) : QString();

				//if (!eventName.isEmpty() || !eventContent.isEmpty()) {
					++eventCount;
					stream << "; PATH_ID=" << pathId
						<< ", POINT_ID=" << pointId
						<< ", EVENT_NAME=" << eventName
						<< ", EVENT_CONTENT=" << eventContent
						<< ", EXEC_DEVICE_ID=" << executeDeviceId
						<< ", FORWARD=" << (pointForward != 0 ? "TRUE" : "FALSE")
						<< ", UNION=" << (bUnion != 0 ? "TRUE" : "FALSE")
						<< "\n";
				//}
			}

			if (wsName) m_ptrKit->PQAPIFree((LONG_PTR*)wsName);
			if (wsEventTemplate) m_ptrKit->PQAPIFree((LONG_PTR*)wsEventTemplate);
			if (wsEventContent) m_ptrKit->PQAPIFree((LONG_PTR*)wsEventContent);
			if (nozzleIds) m_ptrKit->PQAPIFreeArray((LONG_PTR*)nozzleIds);
			if (assistorIds) m_ptrKit->PQAPIFreeArray((LONG_PTR*)assistorIds);
		}

		m_ptrKit->PQAPIFreeArray((LONG_PTR*)ulPointsIDs);
	}

	if (eventCount == 0) {
		stream << QString::fromStdString(u8"; 未检测到喷涂事件\n");
	}

	return content;
}

QString postProcessing::buildAgvStationSection(const QString& robotName, const QString& pathGroupName) const
{
	QString content;
	QTextStream stream(&content);

	QVector<AgvStationInfo> stations;
	RobxIO io;
	io.updateData(stations, "AgvStationInfo.json");

	bool hasAny = false;
	for (int i = 0; i < stations.size(); ++i) {
		const AgvStationInfo& s = stations.at(i);
		const QString sRobot = QString::fromStdString(s.robotName).trimmed();
		const QString sGroup = QString::fromStdString(s.groupName).trimmed();

		if (sRobot != robotName.trimmed() || sGroup != pathGroupName.trimmed()) {
			continue;
		}

		if (!hasAny) {
			stream << QString::fromStdString("\n; ===== AGV站位点 =====\n");
			hasAny = true;
		}

		stream << "; STATION=" << QString::fromStdString(s.stationName)
			<< ", PATH=" << QString::fromStdString(s.pathName)
			<< ", X=" << QString::number(s.x, 'f', 3)
			<< ", Y=" << QString::number(s.y, 'f', 3)
			<< ", Z=" << QString::number(s.z, 'f', 3)
			<< ", THETA=" << QString::number(s.theta, 'f', 3)
			<< "\n";
	}

	return content;
}

QString postProcessing::buildAgvStationEventSection(ULONG agvId, const QString& robotName, const QString& pathGroupName) const
{
	QString content;
	QTextStream stream(&content);

	if (!m_ptrKit || agvId == 0) {
		stream << QString::fromStdString("; AGV ID 无效，未读取到站位事件\n");
		return content;
	}


	QVector<AgvStationInfo> stations;
	RobxIO io;
	io.updateData(stations, "AgvStationInfo.json");

	// 构建更完整匹配键：robot_group_station、robot_group_path、robot_group
	QStringList keys;
	for (int i = 0; i < stations.size(); ++i) {
		const AgvStationInfo& s = stations.at(i);
		const QString sRobot = QString::fromStdString(s.robotName).trimmed();
		const QString sGroup = QString::fromStdString(s.groupName).trimmed();
		const QString sStation = QString::fromStdString(s.stationName).trimmed();
		const QString sPath = QString::fromStdString(s.pathName).trimmed();

		if (sRobot != robotName.trimmed() || sGroup != pathGroupName.trimmed()) {
			continue;
		}

		if (!sStation.isEmpty()) {
			keys << QString("%1_%2_%3").arg(sRobot, sGroup, sStation);
		}
		if (!sPath.isEmpty()) {
			keys << QString("%1_%2_%3").arg(sRobot, sGroup, sPath);
		}
		keys << QString("%1_%2").arg(sRobot, sGroup);
	}
	keys.removeDuplicates();

	stream << QString::fromStdString("\n; ===== AGV站位点事件（发送/等待） =====\n");

	int nEventTemplateLen = 0;
	if (FAILED(m_ptrKit->Doc_get_event_template_count(agvId, &nEventTemplateLen)) || nEventTemplateLen <= 0) {
		stream << QString::fromStdString("; 未获取到事件模板\n");
		return content;
	}

	QSet<QString> sendLines;
	QSet<QString> waitLines;

	for (int idx = 1; idx < nEventTemplateLen; ++idx) {
		WCHAR* wsEventTemplate = nullptr;
		WCHAR* wsTemplateContext = nullptr;

		HRESULT hr = m_ptrKit->Doc_get_event_template(agvId, idx, &wsEventTemplate, &wsTemplateContext);
		if (SUCCEEDED(hr)) {
			const QString eventName = wsEventTemplate ? QString::fromWCharArray(wsEventTemplate) : QString();
			const QString eventCtx = wsTemplateContext ? QString::fromWCharArray(wsTemplateContext) : QString();

			bool matched = keys.isEmpty();
			for (int k = 0; !matched && k < keys.size(); ++k) {
				const QString& key = keys.at(k);
				if (eventName.contains(key, Qt::CaseInsensitive) || eventCtx.contains(key, Qt::CaseInsensitive)) {
					matched = true;
				}
			}

			if (matched && (!eventName.isEmpty() || !eventCtx.isEmpty())) {
				const QString line = QString("; EVENT_NAME=%1, CONTENT=%2").arg(eventName, eventCtx);
				if (eventName.contains("_wait", Qt::CaseInsensitive) || eventCtx.contains("_wait", Qt::CaseInsensitive)) {
					waitLines.insert(line);
				}
				else {
					sendLines.insert(line);
				}
			}
		}

		if (wsEventTemplate) m_ptrKit->PQAPIFree((LONG_PTR*)wsEventTemplate);
		if (wsTemplateContext) m_ptrKit->PQAPIFree((LONG_PTR*)wsTemplateContext);
	}

	if (sendLines.isEmpty() && waitLines.isEmpty()) {
		stream << "; 未匹配到AGV站位点相关事件\n";
		return content;
	}

	if (!sendLines.isEmpty()) {
		stream << "; -- 发送事件 --\n";
		for (auto it = sendLines.constBegin(); it != sendLines.constEnd(); ++it) {
			stream << *it << "\n";
		}
	}
	if (!waitLines.isEmpty()) {
		stream << "; -- 等待事件 --\n";
		for (auto it = waitLines.constBegin(); it != waitLines.constEnd(); ++it) {
			stream << *it << "\n";
		}
	}

	return content;
}

QMap<ULONG, QString> postProcessing::getObjectsByType(PQDataType objType) const
{
	QMap<ULONG, QString> objectMap;

	if (!m_ptrKit) {
		return objectMap;
	}

	VARIANT namesVariant;
	VariantInit(&namesVariant);
	namesVariant.parray = NULL;

	VARIANT idsVariant;
	VariantInit(&idsVariant);
	idsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Doc_get_obj_bytype(objType, &namesVariant, &idsVariant);
	if (FAILED(hr)) {
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap;
	}

	QStringList names = extractStringArrayFromVariant(namesVariant);
	QList<long> ids = extractLongArrayFromVariant(idsVariant);

	const int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; ++i) {
		objectMap[static_cast<ULONG>(ids[i])] = names[i];
	}

	VariantClear(&namesVariant);
	VariantClear(&idsVariant);

	return objectMap;
}

QStringList postProcessing::extractStringArrayFromVariant(const VARIANT& variant) const
{
	QStringList result;

	if ((variant.vt & VT_ARRAY) == 0 || !variant.parray || variant.parray->cDims != 1) {
		return result;
	}

	SAFEARRAY* array = variant.parray;

	VARTYPE vt = VT_EMPTY;
	if (FAILED(SafeArrayGetVartype(array, &vt)) || vt != VT_BSTR) {
		return result;
	}

	BSTR* data = nullptr;
	if (FAILED(SafeArrayAccessData(array, (void**)&data)) || !data) {
		return result;
	}

	long lowerBound = 0;
	long upperBound = -1;
	SafeArrayGetLBound(array, 1, &lowerBound);
	SafeArrayGetUBound(array, 1, &upperBound);

	for (long i = 0; i <= upperBound - lowerBound; ++i) {
		result.append(data[i] ? QString::fromWCharArray(data[i]) : QString());
	}

	SafeArrayUnaccessData(array);
	return result;
}

QList<long> postProcessing::extractLongArrayFromVariant(const VARIANT& variant) const
{
	QList<long> result;

	if ((variant.vt & VT_ARRAY) == 0 || !variant.parray || variant.parray->cDims != 1) {
		return result;
	}

	SAFEARRAY* array = variant.parray;
	VARTYPE vt = VT_EMPTY;
	if (FAILED(SafeArrayGetVartype(array, &vt))) {
		return result;
	}

	long lowerBound = 0;
	long upperBound = -1;
	if (FAILED(SafeArrayGetLBound(array, 1, &lowerBound)) ||
		FAILED(SafeArrayGetUBound(array, 1, &upperBound))) {
		return result;
	}

	const long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		return result;
	}

	if (FAILED(SafeArrayLock(array))) {
		return result;
	}

	switch (vt) {
	case VT_I4:
	{
		LONG* data = static_cast<LONG*>(array->pvData);
		for (long i = 0; i < elementCount; ++i) {
			result.append(static_cast<long>(data[i]));
		}
		break;
	}
	case VT_UI4:
	{
		ULONG* data = static_cast<ULONG*>(array->pvData);
		for (long i = 0; i < elementCount; ++i) {
			result.append(static_cast<long>(data[i]));
		}
		break;
	}
	case VT_I8:
	{
		LONGLONG* data = static_cast<LONGLONG*>(array->pvData);
		for (long i = 0; i < elementCount; ++i) {
			result.append(static_cast<long>(data[i]));
		}
		break;
	}
	case VT_BSTR:
	{
		BSTR* data = static_cast<BSTR*>(array->pvData);
		for (long i = 0; i < elementCount; ++i) {
			if (data[i]) {
				bool ok = false;
				long value = QString::fromWCharArray(data[i]).toLong(&ok);
				result.append(ok ? value : 0L);
			}
			else {
				result.append(0L);
			}
		}
		break;
	}
	default:
		break;
	}

	SafeArrayUnlock(array);
	return result;
}

ULONG postProcessing::getObjIdByName(PQDataType objType, const QString& objName) const
{
	ULONG objId = 0;
	QMap<ULONG, QString> objectMap = getObjectsByType(objType);

	if (objectMap.isEmpty()) {
		return objId;
	}

	const QString target = objName.trimmed();

	for (auto it = objectMap.constBegin(); it != objectMap.constEnd(); ++it) {
		if (it.value().trimmed() == target) {
			objId = it.key();
			break;
		}
	}

	return objId;
}

QString postProcessing::generatePathGroupPostContentInternal(
	ULONG robotId,
	BSTR pathGroupId,
	const QString& robotName,
	const QString& pathGroupName) const
{
	QString content;
	QTextStream stream(&content);

	if (!m_ptrKit || robotId == 0 || pathGroupId == nullptr) {
		stream << "错误: 参数无效\n";
		return content;
	}

	// 外部轴（导轨）
	ULONG externalId = getObjIdByName(PQ_ROBOT, robotName + "_rail");
	int railCount = 0;
	if (externalId != 0) {
		m_ptrKit->Doc_get_obj_joint_count(externalId, &railCount);
	}

	int robotJointCount = 0;
	m_ptrKit->Doc_get_obj_joint_count(robotId, &robotJointCount);

	QString outputFilename = QString("%1_%2.txt").arg(robotName).arg(pathGroupName);
	TrajectoryFileGenerator generator(outputFilename.toStdString(), robotName.toStdString());
	generator.setRobotJointsCount(robotJointCount);
	if (externalId != 0) {
		generator.setExternalAxesCount(railCount);
	}

	int globalPointIndex = 1;

	VARIANT pathNamesVariant;
	VARIANT pathIDsVariant;
	VariantInit(&pathNamesVariant);
	VariantInit(&pathIDsVariant);
	pathNamesVariant.parray = NULL;
	pathIDsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Path_get_group_path(robotId, pathGroupId, &pathNamesVariant, &pathIDsVariant);
	if (SUCCEEDED(hr) && pathIDsVariant.parray && pathIDsVariant.parray->cDims == 1) {
		SAFEARRAY* pathIDArray = pathIDsVariant.parray;
		LONG* pathIdData = NULL;

		if (SUCCEEDED(SafeArrayAccessData(pathIDArray, (void**)&pathIdData)) && pathIdData) {
			const ULONG pathCount = pathIDArray->rgsabound[0].cElements;

			for (ULONG pathIndex = 0; pathIndex < pathCount; ++pathIndex) {
				const ULONG pathId = static_cast<ULONG>(pathIdData[pathIndex]);

				int pointCount = 0;
				ULONG* pointIDs = nullptr;
				HRESULT hrPts = m_ptrKit->Path_get_point_id(pathId, &pointCount, &pointIDs);
				if (FAILED(hrPts) || pointCount <= 0 || !pointIDs) {
					if (pointIDs) {
						m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointIDs);
					}
					continue;
				}

				for (int ptIndex = 0; ptIndex < pointCount; ++ptIndex) {
					const ULONG pointId = pointIDs[ptIndex];

					VARIANT jointDataVariant;
					VariantInit(&jointDataVariant);

					VARIANT externalJointsVariant;
					VariantInit(&externalJointsVariant);

					m_ptrKit->PQAPIGetRobotJointsFromPoints(pointId, &jointDataVariant);
					if (externalId != 0) {
						m_ptrKit->PQAPIGetExternalJointsFromPoints(pointId, externalId, &externalJointsVariant);
					}

					PQPostureType nType = QUATERNION;
					int nPostureCount = 0;
					double* dPointPosture = nullptr;
					double dVelocity = 0.0;
					double dSpeedPercent = 0.0;
					PQPointInstruction iInstruct = PQ_LINE;
					int iApproach = 0;

					m_ptrKit->PQAPIGetPointInfo(pointId, nType, &nPostureCount, &dPointPosture,
						&dVelocity, &dSpeedPercent, &iInstruct, &iApproach);

					TrajectoryPointInfo pointInfo;
					pointInfo.pointIndex = globalPointIndex++;
					pointInfo.motionType = (ptIndex == 0) ? "MOVJ" : "MOVD";
					pointInfo.motionPercentage = static_cast<int>(dSpeedPercent);
					pointInfo.velocity = dVelocity;

					// 机器人关节
					if (jointDataVariant.parray && jointDataVariant.parray->cDims == 1) {
						SAFEARRAY* jointArray = jointDataVariant.parray;
						double* jointData = nullptr;
						if (SUCCEEDED(SafeArrayAccessData(jointArray, (void**)&jointData)) && jointData) {
							ULONG jointCount = jointArray->rgsabound[0].cElements;
							for (ULONG j = 0; j < jointCount; ++j) {
								pointInfo.robotJoints.push_back(jointData[j]);
							}
							SafeArrayUnaccessData(jointArray);
						}
					}

					// 外部轴
					if (externalId != 0 && externalJointsVariant.parray && externalJointsVariant.parray->cDims == 1) {
						SAFEARRAY* extArray = externalJointsVariant.parray;
						double* extData = nullptr;
						if (SUCCEEDED(SafeArrayAccessData(extArray, (void**)&extData)) && extData) {
							ULONG extCount = extArray->rgsabound[0].cElements;
							for (ULONG e = 0; e < extCount; ++e) {
								pointInfo.externalAxes.push_back(extData[e]);
							}
							SafeArrayUnaccessData(extArray);
						}
					}

					generator.addTrajectoryPoint(pointInfo);

					if (dPointPosture) {
						m_ptrKit->PQAPIFree((LONG_PTR*)dPointPosture);
					}
					VariantClear(&jointDataVariant);
					VariantClear(&externalJointsVariant);
				}

				m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointIDs);
			}

			SafeArrayUnaccessData(pathIDArray);
		}
	}
	else {
		stream << "错误: 无法获取路径信息\n";
	}

	VariantClear(&pathNamesVariant);
	VariantClear(&pathIDsVariant);

	generator.calculateAndSetNposParams();

	try {
		stream << QString::fromStdString(generator.generateToString());
	}
	catch (const std::exception& e) {
		stream << "\n错误: 生成轨迹文件失败: " << e.what() << "\n";
	}

	return content;
}

ULONG postProcessing::getAgvIdForRobot(const QString& robotName) const
{
	auto tryFile = [&](const QString& filePath) -> ULONG {
		QFile f(filePath);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			return 0;
		}
		const QByteArray data = f.readAll();
		f.close();

		QJsonParseError err;
		QJsonDocument doc = QJsonDocument::fromJson(data, &err);
		if (err.error != QJsonParseError::NoError || !doc.isArray()) {
			return 0;
		}

		QJsonArray arr = doc.array();
		for (const QJsonValue& v : arr) {
			if (!v.isArray()) {
				continue;
			}
			QJsonArray row = v.toArray();
			if (row.size() < 3) {
				continue;
			}
			const QString rName = row.at(0).toString().trimmed();
			const QString agvName = row.at(2).toString().trimmed();
			if (rName == robotName && !agvName.isEmpty() && agvName != QStringLiteral("无")) {
				return getObjIdByName(PQ_WORKINGPART, agvName);
			}
		}
		return 0;
		};

	ULONG id = tryFile("./temp/jsons/relations.json");
	if (id != 0) {
		return id;
	}
	return tryFile("relations.json");
}