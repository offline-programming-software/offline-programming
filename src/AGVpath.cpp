#include "AGVpath.h"
#include "parseJSON.h"
#include <comdef.h>
#include <oaidl.h>
#include <fstream>
#include <QMessageBox>
#include <QVariant>  
#include <sstream>
#include <stdexcept>
#include <algorithm>

AGVpath::AGVpath(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::AGVpathClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);

	initTable();
	refreshAgvStationTable();
	init();

	connect(ui->pushButton_1, &QPushButton::clicked, this, &AGVpath::onCalculate);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &AGVpath::onConfirm);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &AGVpath::onCancel);

	connect(ui->pushButton_4, &QPushButton::clicked, this, &AGVpath::onInsertRow);
	connect(ui->pushButton_7, &QPushButton::clicked, this, &AGVpath::onDeleteRow);
	connect(ui->pushButton_6, &QPushButton::clicked, this, &AGVpath::onMoveRowUp);
	connect(ui->pushButton_5, &QPushButton::clicked, this, &AGVpath::onMoveRowDown);
	connect(ui->pushButton_8, &QPushButton::clicked, this, &AGVpath::onAddSimulationEvent);
}

AGVpath::~AGVpath()
{
	delete ui;
}

QStringList AGVpath::getPathGroupNames(ULONG robotID)
{
	QStringList groupNames;

	VARIANT varGroupName;
	VariantInit(&varGroupName);
	varGroupName.parray = NULL;

	// 调用接口获取轨迹组名称
	HRESULT hr = m_ptrKit->Doc_get_pathgroup_name(robotID, &varGroupName);
	if (SUCCEEDED(hr)) {
		groupNames = extractStringArrayFromVariant(varGroupName);
		qDebug() << "成功获取轨迹组名称，数量:" << groupNames.size() << "机器人ID:" << robotID;
	}
	else {
		qDebug() << "获取轨迹组名称失败，机器人ID:" << robotID << "错误码:" << hr;
		QMessageBox::warning(this, "警告", "获取轨迹组名称失败！");
	}

	VariantClear(&varGroupName);
	return groupNames;
}

QStringList AGVpath::getPathNames(ULONG robotID, const QString & groupName)
{
	QStringList pathNames;

	VARIANT sNames;
	VARIANT sIDs;
	VariantInit(&sNames);
	VariantInit(&sIDs);
	sNames.parray = NULL;
	sIDs.parray = NULL;

	std::wstring wstrGroupName = groupName.toStdWString();
	BSTR bstrGroupName = SysAllocString(wstrGroupName.c_str());

	HRESULT hr = m_ptrKit->Path_get_group_path(robotID, bstrGroupName, &sNames, &sIDs);

	SysFreeString(bstrGroupName);

	if (SUCCEEDED(hr)) {
		pathNames = extractStringArrayFromVariant(sNames);
		const QList<long> pathIds = extractLongArrayFromVariant(sIDs);

		const QString trimmedGroup = groupName.trimmed();
		const QString prefix = QStringLiteral("%1|%2|")
			.arg(QString::number(robotID), trimmedGroup);

		for (auto it = m_pathIdCache.begin(); it != m_pathIdCache.end();) {
			it = it.key().startsWith(prefix) ? m_pathIdCache.erase(it) : std::next(it);
		}

		const int count = qMin(pathNames.size(), pathIds.size());
		for (int i = 0; i < count; ++i) {
			const QString trimmedPath = pathNames.at(i).trimmed();
			m_pathIdCache.insert(
				buildPathCacheKey(robotID, trimmedGroup, trimmedPath),
				static_cast<ULONG>(pathIds.at(i)));
		}

		qDebug() << "成功获取轨迹名称，数量:" << pathNames.size()
			<< "机器人ID:" << robotID << "组名:" << groupName;
	}
	else {
		qDebug() << "获取轨迹名称失败，机器人ID:" << robotID
			<< "组名:" << groupName << "错误码:" << hr;
	}

	VariantClear(&sNames);
	VariantClear(&sIDs);

	return pathNames;
}

void AGVpath::init()
{

	QString relations = m_tempDir + "relations.json";
	// 加载机器人关系数据
	relationsMap = loadRobotRelations(relations.toStdString());

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		reject();
		return;
	}

	ui->comboBox_1->addItems(robotNames);

<<<<<<< HEAD
	QString robotName = ui->comboBox_1->currentText();
=======
	ui->comboBox_1->addItems(AGVNames);
	//ui->comboBox_5->addItems(AGVNames);
>>>>>>> 1c85fd5454076c0fd354098cec5f645d8b2ac145

	ULONG AGVID = m_robotMap.key(robotName);

	if (ui->comboBox_1->count() > 0) { // 确保有选项后再触发
		onAGVSelectionChanged(ui->comboBox_1->currentIndex());
	}

	connect(ui->comboBox_1, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onAGVSelectionChanged);
	connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onComboBox2CurrentIndexChanged);
	connect(ui->comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onComboBox4CurrentIndexChanged);
	connect(ui->comboBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onComboBox3CurrentIndexChanged);
	connect(ui->comboBox_5, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AGVpath::onComboBox5CurrentIndexChanged);


}

QVector<AgvStationInfo> AGVpath::loadAgvStationInfos() const
{
	QVector<AgvStationInfo> stations;
	RobxIO io;
	io.updateData(stations, "AgvStationInfo.json");
	return stations;
}

void AGVpath::refreshAgvStationTable()
{
	if (!ui || !ui->tableWidget) {
		return;
	}

	const QVector<AgvStationInfo> stations = loadAgvStationInfos();
	agvStations = stations;

	QTableWidget* table = ui->tableWidget;
	table->setRowCount(0);

	for (const auto& station : stations) {
		const int row = table->rowCount();
		table->insertRow(row);

		auto* groupItem = new QTableWidgetItem(QString::fromStdString(station.groupName));
		groupItem->setData(Qt::UserRole, QString::fromStdString(station.robotName));
		table->setItem(row, 0, groupItem);

		table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(station.pathName)));

		auto* nameItem = new QTableWidgetItem(QString::fromStdString(station.stationName));
		nameItem->setData(Qt::UserRole, station.z);
		table->setItem(row, 2, nameItem);

		table->setItem(row, 3, new QTableWidgetItem(QString::number(station.x, 'f', 3)));
		table->setItem(row, 4, new QTableWidgetItem(QString::number(station.y, 'f', 3)));
		table->setItem(row, 5, new QTableWidgetItem(QString::number(station.theta, 'f', 3)));
	}

	updateVisibleStationRange();
}

void AGVpath::updateVisibleStationRange()
{
	if (!ui || !ui->tableWidget) {
		return;
	}

	const QString startGroup = ui->comboBox_2 ? ui->comboBox_2->currentText().trimmed() : QString();
	const QString startPath = ui->comboBox_3 ? ui->comboBox_3->currentText().trimmed() : QString();
	const QString startStation = ui->textBrowser_2 ? ui->textBrowser_2->toPlainText().trimmed() : QString();
	const QString endGroup = ui->comboBox_4 ? ui->comboBox_4->currentText().trimmed() : QString();
	const QString endPath = ui->comboBox_5 ? ui->comboBox_5->currentText().trimmed() : QString();
	const QString endStation = ui->textBrowser_3 ? ui->textBrowser_3->toPlainText().trimmed() : QString();

	QTableWidget* table = ui->tableWidget;
	auto setAllHidden = [table](bool hidden) {
		for (int row = 0; row < table->rowCount(); ++row) {
			table->setRowHidden(row, hidden);
		}
		};

	auto locateRow = [table](const QString& group, const QString& path, const QString& station) -> int {
		if (group.isEmpty() || path.isEmpty() || station.isEmpty()) {
			return -1;
		}
		for (int row = 0; row < table->rowCount(); ++row) {
			const QTableWidgetItem* groupItem = table->item(row, 0);
			const QTableWidgetItem* pathItem = table->item(row, 1);
			const QTableWidgetItem* stationItem = table->item(row, 2);
			if (groupItem && pathItem && stationItem &&
				groupItem->text().trimmed() == group &&
				pathItem->text().trimmed() == path &&
				stationItem->text().trimmed() == station) {
				return row;
			}
		}
		return -1;
		};

	int startRow = locateRow(startGroup, startPath, startStation);
	int endRow = locateRow(endGroup, endPath, endStation);

	const bool hasStart = startRow >= 0;
	const bool hasEnd = endRow >= 0;

	if (!hasStart && !hasEnd) {
		setAllHidden(true);
		return;
	}

	if (!hasStart) {
		startRow = endRow;
	}
	else if (!hasEnd) {
		endRow = startRow;
	}

	const int lower = qMin(startRow, endRow);
	const int upper = qMax(startRow, endRow);
	for (int row = 0; row < table->rowCount(); ++row) {
		const bool keepVisible = row >= lower && row <= upper;
		table->setRowHidden(row, !keepVisible);
	}
}

void AGVpath::syncTableToAgvStations()
{
	if (!ui || !ui->tableWidget) {
		agvStations.clear();
		return;
	}

	QTableWidget* table = ui->tableWidget;
	QVector<AgvStationInfo> updated;
	updated.reserve(table->rowCount());

	auto parseDouble = [](QTableWidgetItem* item) {
		if (!item) return 0.0;
		bool ok = false;
		const double value = item->text().trimmed().toDouble(&ok);
		return ok ? value : 0.0;
		};

	const QString defaultRobot = ui->comboBox_1
		? ui->comboBox_1->currentText().trimmed()
		: QString();

	for (int row = 0; row < table->rowCount(); ++row) {
		auto* groupItem = table->item(row, 0);
		auto* pathItem = table->item(row, 1);
		auto* stationItem = table->item(row, 2);
		auto* xItem = table->item(row, 3);
		auto* yItem = table->item(row, 4);
		auto* thetaItem = table->item(row, 5);

		if (!groupItem || !pathItem || !stationItem) {
			continue;
		}

		AgvStationInfo info;
		const QString robotName = groupItem->data(Qt::UserRole).toString().trimmed();
		info.robotName = (robotName.isEmpty() ? defaultRobot : robotName).toStdString();
		info.groupName = groupItem->text().trimmed().toStdString();
		info.pathName = pathItem->text().trimmed().toStdString();
		info.stationName = stationItem->text().trimmed().toStdString();
		info.x = parseDouble(xItem);
		info.y = parseDouble(yItem);
		info.theta = parseDouble(thetaItem);
		info.z = stationItem->data(Qt::UserRole).isValid()
			? stationItem->data(Qt::UserRole).toDouble()
			: 0.0;

		updated.push_back(std::move(info));
	}

	agvStations = std::move(updated);
}

void AGVpath::persistAgvStations()
{
	if (agvStations.isEmpty()) {
		// 允许写入空数组，也可在此判断是否需要
	}
	try {
		RobxIO io;
		io.writeData(agvStations, "AgvStationInfo.json");
	}
	catch (const std::exception& ex) {
		qWarning() << "写入 AgvStationInfo.json 失败:" << ex.what();
	}
}

void AGVpath::commitAgvStations()
{
	if (!ui || !ui->tableWidget) {
		return;
	}
	syncTableToAgvStations();
	persistAgvStations();
}

QString AGVpath::nextTransitionLabel() const
{
	if (!ui || !ui->tableWidget) {
		return "过渡点1";
	}

	static const QRegularExpression regex(R"(过渡点(\d+))");
	int maxIndex = 0;
	QTableWidget* table = ui->tableWidget;

	for (int row = 0; row < table->rowCount(); ++row) {
		const QTableWidgetItem* stationItem = table->item(row, 2);
		if (!stationItem) {
			continue;
		}
		const QRegularExpressionMatch match = regex.match(stationItem->text().trimmed());
		if (match.hasMatch()) {
			bool ok = false;
			const int value = match.captured(1).toInt(&ok);
			if (ok) {
				maxIndex = std::max(maxIndex, value);
			}
		}
	}
	QString pointName = "过渡点%1";
	pointName = pointName.arg(maxIndex + 1);

	return pointName;
}

int AGVpath::findSortedInsertRow(const QString& groupName,
	const QString& pathName,
	const QString& stationLabel) const
{
	if (!ui || !ui->tableWidget || groupName.isEmpty() || pathName.isEmpty()) {
		return ui && ui->tableWidget ? ui->tableWidget->rowCount() : 0;
	}

	QTableWidget* table = ui->tableWidget;
	const int rowCount = table->rowCount();
	int insertRow = rowCount;

	for (int row = 0; row < rowCount; ++row) {
		const QTableWidgetItem* groupItem = table->item(row, 0);
		const QTableWidgetItem* pathItem = table->item(row, 1);
		if (!groupItem || !pathItem) {
			continue;
		}

		const QString currentGroup = groupItem->text().trimmed();
		const QString currentPath = pathItem->text().trimmed();
		if (currentGroup != groupName || currentPath != pathName) {
			continue;
		}

		const QTableWidgetItem* stationItem = table->item(row, 2);
		const QString currentStation = stationItem ? stationItem->text().trimmed() : QString();

		if (stationLabel.compare(currentStation, Qt::CaseInsensitive) < 0) {
			return row;
		}

		insertRow = row + 1;
	}

	return insertRow;
}

QString AGVpath::buildPathCacheKey(ULONG robotID, const QString& groupName,
	const QString& pathName) const
{
	return QStringLiteral("%1|%2|%3")
		.arg(QString::number(robotID),
			groupName.trimmed(),
			pathName.trimmed());
}

bool AGVpath::tryGetPathId(ULONG robotID, const QString& groupName,
	const QString& pathName, ULONG& pathId)
{
	if (robotID == 0 || groupName.trimmed().isEmpty() || pathName.trimmed().isEmpty()) {
		return false;
	}

	const QString key = buildPathCacheKey(robotID, groupName, pathName);
	auto it = m_pathIdCache.constFind(key);
	if (it != m_pathIdCache.constEnd()) {
		pathId = it.value();
		return true;
	}

	getPathNames(robotID, groupName);
	it = m_pathIdCache.constFind(key);
	if (it != m_pathIdCache.constEnd()) {
		pathId = it.value();
		return true;
	}

	return false;
}

std::map<std::string, std::pair<std::string, std::string>> AGVpath::loadRobotRelations(const std::string& filePath)
{
	std::map<std::string, std::pair<std::string, std::string>> relationsMap;
	std::ifstream file(filePath);

	// 1. 检查文件是否打开成功
	if (!file.is_open()) {
		qWarning() << "警告: 无法打开关系文件" << QString::fromStdString(filePath) << "，将使用默认值。";
		return relationsMap; // 返回空地图
	}

	try {
		json data;
		file >> data;
		file.close();

		// 2. 确保根节点是数组
		if (!data.is_array()) {
			qWarning() << "警告: relations.json 根节点不是数组，解析跳过。";
			return relationsMap;
		}

		// 3. 遍历数组
		for (const auto& item : data) {
			// 确保每一项也是数组，且至少有一个元素（机器人名称）
			if (item.is_array() && item.size() >= 1) {
				std::string robotName = item[0].get<std::string>();

				std::string railName = "无";
				std::string agvName = "无";

				// 读取第二个元素 (导轨)，如果存在且不为空
				if (item.size() > 1 && !item[1].get<std::string>().empty()) {
					railName = item[1].get<std::string>();
				}

				// 读取第三个元素 (AGV)，如果存在且不为空
				if (item.size() > 2 && !item[2].get<std::string>().empty()) {
					agvName = item[2].get<std::string>();
				}

				// 存入 Map，方便后续通过机器人名称快速查找
				relationsMap[robotName] = std::make_pair(railName, agvName);
			}
		}
	}
	catch (const std::exception& e) {
		qCritical() << "解析 relations.json 失败:" << e.what();
	}

	return relationsMap;
}

void AGVpath::onCalculate() {

	//// 原有示例代码保留
	// ...

	if (!ui || !ui->tableWidget) {
		return;
	}

	const QString robotName = ui->comboBox_1 ? ui->comboBox_1->currentText().trimmed() : QString();
	const QString startGroup = ui->comboBox_2 ? ui->comboBox_2->currentText().trimmed() : QString();
	const QString startPath = ui->comboBox_3 ? ui->comboBox_3->currentText().trimmed() : QString();
	const QString startStation = ui->textBrowser_2 ? ui->textBrowser_2->toPlainText().trimmed() : QString();
	const QString endGroup = ui->comboBox_4 ? ui->comboBox_4->currentText().trimmed() : QString();
	const QString endPath = ui->comboBox_5 ? ui->comboBox_5->currentText().trimmed() : QString();
	const QString endStation = ui->textBrowser_3 ? ui->textBrowser_3->toPlainText().trimmed() : QString();
	const QString directionText = ui->comboBox_6 ? ui->comboBox_6->currentText().trimmed() : QString();

	if (robotName.isEmpty() || startGroup.isEmpty() || startPath.isEmpty()
		|| startStation.isEmpty() || endGroup.isEmpty() || endPath.isEmpty()
		|| endStation.isEmpty() || directionText.isEmpty()) {
		QMessageBox::warning(this, "提示", "请先完整选择起点、终点及方向、距离。");
		return;
	}

	QString distanceText;
	if (ui->textEdit) {
		const int plainIndex = ui->textEdit->metaObject()->indexOfProperty("plainText");
		if (plainIndex >= 0) {
			distanceText = ui->textEdit->property("plainText").toString().trimmed();
		}
		if (distanceText.isEmpty()) {
			distanceText = ui->textEdit->property("text").toString().trimmed();
		}
	}

	bool distanceOk = false;
	const double planeDistance = distanceText.toDouble(&distanceOk);
	if (!distanceOk) {
		QMessageBox::warning(this, "提示", "请输入有效的距离数值。");
		return;
	}

	QVector3D direction = getDirectionVector(directionText);
	if (direction.lengthSquared() <= std::numeric_limits<float>::epsilon()) {
		QMessageBox::warning(this, "提示", "请选择有效的投影方向。");
		return;
	}
	const QVector3D normalizedDir = direction.normalized();

	AgvStationInfo startInfo;
	if (!tryGetStationInfo(robotName, startGroup, startPath, startStation, startInfo)) {
		QMessageBox::warning(this, "提示", "未在站位列表中找到起点信息。");
		return;
	}
	AgvStationInfo endInfo;
	if (!tryGetStationInfo(robotName, endGroup, endPath, endStation, endInfo)) {
		QMessageBox::warning(this, "提示", "未在站位列表中找到终点信息。");
		return;
	}

	auto projectOntoPlane = [&](const QVector3D& source) {
		const double projection = QVector3D::dotProduct(source, normalizedDir);
		return source + (planeDistance - projection) * normalizedDir;
		};

	const QVector3D startProjected = projectOntoPlane(QVector3D(startInfo.x, startInfo.y, startInfo.z));
	const QVector3D endProjected = projectOntoPlane(QVector3D(endInfo.x, endInfo.y, endInfo.z));

	int insertRow = ui->tableWidget->rowCount();
	for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
		const QTableWidgetItem* groupItem = ui->tableWidget->item(row, 0);
		const QTableWidgetItem* pathItem = ui->tableWidget->item(row, 1);
		const QTableWidgetItem* stationItem = ui->tableWidget->item(row, 2);
		if (groupItem && pathItem && stationItem
			&& groupItem->text().trimmed() == startGroup
			&& pathItem->text().trimmed() == startPath
			&& stationItem->text().trimmed() == startStation) {
			insertRow = row + 1;
			break;
		}
	}

	const QString firstLabel = nextTransitionLabel();
	const int firstRow = insertTransitionRow(0, startGroup, startPath,
		firstLabel, startProjected, startInfo.theta);
	if (firstRow < 0) {
		return;
	}

	const QString secondLabel = nextTransitionLabel();
	insertTransitionRow(0, startGroup, startPath,
		secondLabel, endProjected, startInfo.theta);


	ui->tableWidget->setCurrentCell(firstRow, 0);
}

void AGVpath::onConfirm()
{
	commitAgvStations();
	reject();
}

void AGVpath::onCancel()
{
	reject();    
}

QVector3D AGVpath::getDirectionVector(const QString& dirName) {
	// 将输入转换为小写并去除首尾空格，提高匹配稳定性
	QString direction = dirName.toLower().trimmed();

	if (direction == "x轴正方向" || direction == "x+")
		return QVector3D(1, 0, 0);
	else if (direction == "x轴负方向" || direction == "x-")
		return QVector3D(-1, 0, 0);
	else if (direction == "y轴正方向" || direction == "y+")
		return QVector3D(0, 1, 0);
	else if (direction == "y轴负方向" || direction == "y-")
		return QVector3D(0, -1, 0);
	else if (direction == "z轴正方向" || direction == "z+")
		return QVector3D(0, 0, 1);
	else if (direction == "z轴负方向" || direction == "z-")
		return QVector3D(0, 0, -1);
	else {
		// 默认情况或错误处理，返回零向量
		qWarning() << "未知的方向名称:" << dirName << "，返回零向量";
		return QVector3D(0, 0, 0);
	}
}

AABB AGVpath::creatAABB(ULONG uID, ULONG uCoordinate)
{
	AABB bbox;
	// 首先获取顶点数量
	long lCount = 0;
	double dSrcPosition[6] = {};
	m_ptrKit->PQAPIGetWorkPartVertexCount(uID, &lCount);

	// 准备存储所有顶点数据
	std::vector<double> allVertexData(lCount * 3);

	// 重新设置lCount为实际要获取的数量
	int actualCount = lCount;
	m_ptrKit->PQAPIGetWorkPartVertex(uID, uCoordinate, actualCount, allVertexData.data());

	// 提取顶点坐标（假设每个顶点数据包含6个double，前3个是位置坐标）
	std::vector<Point3D> vertices;
	vertices.reserve(actualCount);

	for (int i = 0; i < actualCount; ++i) {
		double x = allVertexData[i * 3];
		double y = allVertexData[i * 3 + 1];
		double z = allVertexData[i * 3 + 2];
		vertices.emplace_back(x, y, z);
	}

	bbox = bbox.calculateAABB(vertices);

	return bbox;
}

double AGVpath::calculateBoundaryValue(const AABB& bbox, const QVector3D& dir)
{
	// 计算包围盒8个顶点在方向向量上的投影，找到最大值[1,4](@ref)
	std::vector<QVector3D> vertices = {
		QVector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z),
		QVector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.minPoint.z),
		QVector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.minPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.maxPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.minPoint.z),
		QVector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z)
	};

	double maxProjection = std::numeric_limits<double>::lowest();

	for (const auto& vertex : vertices) {
		double projection = QVector3D::dotProduct(vertex, dir);
		if (projection > maxProjection) {
			maxProjection = projection;
		}
	}

	return maxProjection;
}

std::vector<QVector3D> AGVpath::calculateTransitionPoints(
	ULONG startPointID, ULONG endPointID,
	double* startPosture, double* endPosture,
	const QVector3D& dir, double boundaryValue)
{
	std::vector<QVector3D> transitionPoints;
	transitionPoints.reserve(2); // 预分配空间，提高效率

	// 计算起点和终点在方向向量上的投影
	QVector3D startPos(startPosture[0], startPosture[1], startPosture[2]);
	QVector3D endPos(endPosture[0], endPosture[1], endPosture[2]);

	double startProjection = QVector3D::dotProduct(startPos, dir);
	double endProjection = QVector3D::dotProduct(endPos, dir);

	// 计算过渡点1（基于起点）：沿方向向量移动到边界
	QVector3D transition1Pos = startPos + (boundaryValue - startProjection) * dir;

	// 计算过渡点2（基于终点）：沿方向向量移动到边界
	QVector3D transition2Pos = endPos + (boundaryValue - endProjection) * dir;

	// 将两个过渡点位置添加到结果向量中
	transitionPoints.push_back(transition1Pos);
	transitionPoints.push_back(transition2Pos);

	return transitionPoints;
}

void AGVpath::initTable()
{
	// 设置表格列数
	ui->tableWidget->setColumnCount(6);

	// 设置表头
	QStringList headers;
	headers << "group名称" << "path名称" << "站位点名称"<< "X/mm"<< "Y/mm"<<"theta/°";
	ui->tableWidget->setHorizontalHeaderLabels(headers);

	// 设置表格选择模式为单行选择
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	// 设置表格自适应列宽
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	// 更新表格显示
	//updateTable();
}

void AGVpath::onAGVSelectionChanged(int index)
{
	if (!ui || !ui->comboBox_1) {
		return;
	}

	m_pathIdCache.clear();

	if (index < 0) {
		updateAgvNameDisplay(QString());
		return;
	}

	const QString robotName = ui->comboBox_1->itemText(index);
	ULONG robotID = m_robotMap.key(robotName, 0);
	updateAgvNameDisplay(robotName);

	QStringList groupNames = getPathGroupNames(robotID);
	ui->comboBox_2->addItems(groupNames);
	ui->comboBox_4->addItems(groupNames);

	// 如果有轨迹组，触发第一个轨迹组的轨迹加载
	if (!groupNames.isEmpty()) {
		onComboBox2CurrentIndexChanged(0);
		onComboBox4CurrentIndexChanged(0);
	}

}

void AGVpath::updateAgvNameDisplay(const QString& robotName)
{
	if (!ui || !ui->textBrowser) {
		return;
	}

	QString agvName = QStringLiteral("无");
	if (!robotName.isEmpty()) {
		const auto it = relationsMap.find(robotName.toStdString());
		if (it != relationsMap.end() && !it->second.second.empty()) {
			agvName = QString::fromStdString(it->second.second);
		}
	}

	ui->textBrowser->setText(agvName);
}

void AGVpath::onInsertRow()
{
	if (!ui || !ui->tableWidget) {
		return;
	}

	QTableWidget* table = ui->tableWidget;
	const int columnCount = table->columnCount();
	const QString transitionLabel = nextTransitionLabel();

	QSignalBlocker blocker(table);

	QVector<QTableWidgetItem*> clonedItems(columnCount, nullptr);
	if (table->rowCount() > 0) {
		int anchorRow = table->currentRow();
		if (anchorRow < 0) {
			anchorRow = 0;
		}
		for (int col = 0; col < columnCount; ++col) {
			const QTableWidgetItem* sourceItem = table->item(anchorRow, col);
			clonedItems[col] = sourceItem ? sourceItem->clone() : new QTableWidgetItem();
		}
	}
	else {
		for (int col = 0; col < columnCount; ++col) {
			clonedItems[col] = new QTableWidgetItem();
		}
		const QString robotName = ui->comboBox_1 ? ui->comboBox_1->currentText().trimmed() : QString();
		clonedItems[0]->setData(Qt::UserRole, robotName);
		clonedItems[2]->setData(Qt::UserRole, 0.0);
	}

	if (!clonedItems[2]) {
		clonedItems[2] = new QTableWidgetItem();
	}
	clonedItems[2]->setText(transitionLabel);

	const QString groupName = clonedItems[0] ? clonedItems[0]->text().trimmed() : QString();
	const QString pathName = clonedItems[1] ? clonedItems[1]->text().trimmed() : QString();
	const int insertRow = findSortedInsertRow(groupName, pathName, transitionLabel);

	table->insertRow(insertRow);
	for (int col = 0; col < columnCount; ++col) {
		table->setItem(insertRow, col, clonedItems[col]);
	}

	table->setCurrentCell(insertRow, 0);
}


void AGVpath::onDeleteRow()
{
	if (!ui->tableWidget) {
		return;
	}
	QTableWidget * table = ui->tableWidget;
	const int row = table->currentRow();
	if (row < 0) {
		return;
	}
	table->removeRow(row);
	if (table->rowCount() > 0) {
		table->setCurrentCell(qMin(row, table->rowCount() - 1), 0);
	}
	}

void AGVpath::onMoveRowUp()
{
	if (!ui->tableWidget) {
		return;
	}
	QTableWidget * table = ui->tableWidget;
	const int row = table->currentRow();
	if (row <= 0) {
		return;
	}
	swapTableRows(row, row - 1);
	table->setCurrentCell(row - 1, table->currentColumn());
}

void AGVpath::onMoveRowDown()
{
	if (!ui->tableWidget) {
		return;
		
	}
	QTableWidget * table = ui->tableWidget;
	const int row = table->currentRow();
	if (row < 0 || row >= table->rowCount() - 1) {
		return;
	}
	swapTableRows(row, row + 1);
	table->setCurrentCell(row + 1, table->currentColumn());
}

void AGVpath::onAddSimulationEvent()
{
	if (!ui) {
		return;
	}

	const QString robotName = ui->comboBox_1 ? ui->comboBox_1->currentText().trimmed() : QString();
	const QString agvName = ui->textBrowser ? ui->textBrowser->toPlainText().trimmed() : QString();
	const QString startGroup = ui->comboBox_2 ? ui->comboBox_2->currentText().trimmed() : QString();
	const QString startPath = ui->comboBox_3 ? ui->comboBox_3->currentText().trimmed() : QString();
	const QString startStation = ui->textBrowser_2 ? ui->textBrowser_2->toPlainText().trimmed() : QString();
	const QString endGroup = ui->comboBox_4 ? ui->comboBox_4->currentText().trimmed() : QString();
	const QString endPath = ui->comboBox_5 ? ui->comboBox_5->currentText().trimmed() : QString();
	const QString endStation = ui->textBrowser_3 ? ui->textBrowser_3->toPlainText().trimmed() : QString();

	if (robotName.isEmpty() || agvName.isEmpty()
		|| agvName.compare(QStringLiteral("无"), Qt::CaseInsensitive) == 0
		|| startGroup.isEmpty() || startPath.isEmpty() || startStation.isEmpty()
		|| endGroup.isEmpty() || endPath.isEmpty() || endStation.isEmpty()) {
		QMessageBox::warning(this, tr("提示"), tr("请先确认机器人、AGV 以及完整的起止站位点。"));
		return;
	}

	const auto buildStationKey = [&](const QString& group, const QString& path, const QString& station) {
		return QStringLiteral("%1_%2_%3_%4").arg(robotName, group, path, station);
		};
	const auto fetchBoundaryPoint = [&](ULONG pathId, bool takeFirst, ULONG& pointId) -> bool {
		int pointCount = 0;
		ULONG* pointIDs = nullptr;
		const HRESULT hr = m_ptrKit->Path_get_point_id(pathId, &pointCount, &pointIDs);
		if (FAILED(hr) || pointCount <= 0 || !pointIDs) {
			if (pointIDs) {
				m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointIDs);
			}
			return false;
		}
		pointId = takeFirst ? pointIDs[0] : pointIDs[pointCount - 1];
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointIDs);
		return true;
		};

	// ---------- AGV 站位事件 ----------
	const QString startStationKey = buildStationKey(startGroup, startPath, startStation);
	const QString endStationKey = buildStationKey(endGroup, endPath, endStation);

	ULONG agvID = 0;
	GetObjIDByName(PQ_WORKINGPART, agvName.toStdWString(), agvID);
	if (agvID == 0) {
		QMessageBox::warning(this, tr("提示"), tr("无法找到 AGV %1。").arg(agvName));
		return;
	}

	INT pathCount = 0;
	WCHAR* whPathNames = nullptr;
	ULONG* pathIDs = nullptr;
	const HRESULT partHr = m_ptrKit->Part_get_path(agvID, &pathCount, &whPathNames, &pathIDs);
	if (FAILED(partHr) || pathCount <= 0 || !whPathNames || !pathIDs) {
		if (whPathNames) m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
		if (pathIDs) m_ptrKit->PQAPIFree((LONG_PTR*)pathIDs);
		QMessageBox::warning(this, tr("提示"), tr("获取 AGV 站位点失败。"));
		return;
	}

	const QString rawNames = QString::fromWCharArray(whPathNames);
	QStringList nameList = rawNames.split('#', QString::SkipEmptyParts);
	QMap<QString, ULONG> stationToPathId;
	const int usable = qMin(pathCount, nameList.size());
	for (int i = 0; i < usable; ++i) {
		stationToPathId.insert(nameList.at(i).trimmed(), pathIDs[i]);
	}
	m_ptrKit->PQAPIFree((LONG_PTR*)whPathNames);
	m_ptrKit->PQAPIFree((LONG_PTR*)pathIDs);

	const ULONG stationStartPathID = stationToPathId.value(startStationKey, 0);
	const ULONG stationEndPathID = stationToPathId.value(endStationKey, 0);
	if (stationStartPathID == 0 || stationEndPathID == 0) {
		QMessageBox::warning(this, tr("提示"), tr("在 AGV %1 中找不到指定站位点。").arg(agvName));
		return;
	}

	ULONG stationStartPointID = 0;
	ULONG stationEndPointID = 0;
	if (!fetchBoundaryPoint(stationStartPathID, false, stationStartPointID)
		|| !fetchBoundaryPoint(stationEndPathID, true, stationEndPointID)) {
		QMessageBox::warning(this, tr("提示"), tr("解析站位点路径失败。"));
		return;
	}

	ULONG robotID = 0;
	GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);
	if (robotID == 0) {
		QMessageBox::warning(this, tr("提示"), tr("无法找到当前机器人。"));
		return;
	}


	// ---------- 喷涂轨迹事件 ----------
	//根据路径名称获取路径ID
	ULONG trackStartPathID = 0;
	ULONG trackEndPathID = 0;
	if (!tryGetPathId(robotID, startGroup, startPath, trackStartPathID)
		|| !tryGetPathId(robotID, endGroup, endPath, trackEndPathID)) {
		QMessageBox::warning(this, tr("提示"), tr("无法解析所选轨迹，请检查路径名称。"));
		return;
	}

	ULONG startTrackFirstPointID = 0;
	ULONG startTrackLastPointID = 0;
	ULONG endTrackFirstPointID = 0;
	if (!fetchBoundaryPoint(trackStartPathID, true, startTrackFirstPointID)
		|| !fetchBoundaryPoint(trackStartPathID, false, startTrackLastPointID)
		|| !fetchBoundaryPoint(trackEndPathID, true, endTrackFirstPointID)) {
		QMessageBox::warning(this, tr("提示"), tr("获取轨迹端点失败。"));
		return;
	}

	const QString e1Name = QStringLiteral("%1_%2_%3_e1").arg(robotName, startGroup, startStation);
	const QString e2Name = QStringLiteral("%1_%2_%3_e2").arg(robotName, startGroup, startPath);
	const QString e3Name = QStringLiteral("%1_%2_%3_e3").arg(robotName, endGroup, endStation);

	const QString e1WaitName = e1Name + QStringLiteral("_wait");
	const QString e2WaitName = e2Name + QStringLiteral("_wait");
	const QString e3WaitName = e3Name + QStringLiteral("_wait");

	CComBSTR e1SendBstr(e1Name.toStdWString().c_str());
	CComBSTR e1WaitEventBstr(e1WaitName.toStdWString().c_str());
	CComBSTR e1WaitContentBstr(e1Name.toStdWString().c_str());

	CComBSTR e2SendBstr(e2Name.toStdWString().c_str());
	CComBSTR e2WaitEventBstr(e2WaitName.toStdWString().c_str());
	CComBSTR e2WaitContentBstr(e2Name.toStdWString().c_str());

	CComBSTR e3SendBstr(e3Name.toStdWString().c_str());
	CComBSTR e3WaitEventBstr(e3WaitName.toStdWString().c_str());
	CComBSTR e3WaitContentBstr(e3Name.toStdWString().c_str());

	if (FAILED(m_ptrKit->Path_add_send_event(&stationStartPointID, 1, agvID, 0, e1SendBstr))
		|| FAILED(m_ptrKit->Path_add_wait_event(&startTrackFirstPointID, 1, robotID, 1, e1WaitEventBstr, e1WaitContentBstr))
		|| FAILED(m_ptrKit->Path_add_send_event(&startTrackLastPointID, 1, robotID, 0, e2SendBstr))
		|| FAILED(m_ptrKit->Path_add_wait_event(&stationEndPointID, 1, agvID, 1, e2WaitEventBstr, e2WaitContentBstr))
		|| FAILED(m_ptrKit->Path_add_send_event(&stationEndPointID, 1, agvID, 0, e3SendBstr))
		|| FAILED(m_ptrKit->Path_add_wait_event(&endTrackFirstPointID, 1, robotID, 1, e3WaitEventBstr, e3WaitContentBstr))) {
		QMessageBox::warning(this, tr("提示"), tr("写入发送/等待事件失败，请检查配置。"));
		return;
	}

	QMessageBox::information(this, tr("提示"),
		tr("站位与喷涂轨迹的三段信号链路已全部建立。"));
}

void AGVpath::swapTableRows(int firstRow, int secondRow)
{
	QTableWidget * table = ui->tableWidget;
	if (!table) {
		return;
		
	}
	const int columnCount = table->columnCount();
	for (int col = 0; col < columnCount; ++col) {
		QTableWidgetItem * firstItem = table->takeItem(firstRow, col);
		QTableWidgetItem * secondItem = table->takeItem(secondRow, col);
		
		if (!firstItem) {
			firstItem = new QTableWidgetItem();
		
		}
		if (!secondItem) {
			secondItem = new QTableWidgetItem();
			
		}
		table->setItem(firstRow, col, secondItem);
		table->setItem(secondRow, col, firstItem);
		
	}
}

QString AGVpath::findStationNames(
	const QString& robotName,
	const QString& groupName,
	const QString& pathName) const
{
	QString stationName = "无";

	for (const auto& station : agvStations) {
		const QString stationRobot = QString::fromStdString(station.robotName);
		const QString stationGroup = QString::fromStdString(station.groupName);
		const QString stationPath = QString::fromStdString(station.pathName);

		if ((robotName.isEmpty() || stationRobot == robotName)
			&& (groupName.isEmpty() || stationGroup == groupName)
			&& (pathName.isEmpty() || stationPath == pathName)) {
			stationName = QString::fromStdString(station.stationName);
			break;
		}
	}

	return stationName;
}

bool AGVpath::tryGetStationInfo(
	const QString& robotName,
	const QString& groupName,
	const QString& pathName,
	const QString& stationName,
	AgvStationInfo& info) const
{
	for (const auto& station : agvStations) {
		if (QString::fromStdString(station.robotName) != robotName
			|| QString::fromStdString(station.groupName) != groupName
			|| QString::fromStdString(station.pathName) != pathName) {
			continue;
		}

		if (!stationName.isEmpty()
			&& QString::fromStdString(station.stationName) != stationName) {
			continue;
		}

		info = station;
		return true;
	}

	return false;
}

int AGVpath::insertTransitionRow(
	int /*rowIndex*/,
	const QString& groupName,
	const QString& pathName,
	const QString& stationLabel,
	const QVector3D& position,
	double theta)
{
	if (!ui || !ui->tableWidget) {
		return -1;
	}

	QTableWidget* table = ui->tableWidget;
	const int insertRow = findSortedInsertRow(groupName, pathName, stationLabel);
	table->insertRow(insertRow);

	constexpr int precision = 3;
	const QString robotName = ui->comboBox_1 ? ui->comboBox_1->currentText().trimmed() : QString();

	auto* groupItem = new QTableWidgetItem(groupName);
	groupItem->setData(Qt::UserRole, robotName);
	table->setItem(insertRow, 0, groupItem);

	table->setItem(insertRow, 1, new QTableWidgetItem(pathName));

	auto* stationItem = new QTableWidgetItem(stationLabel);
	stationItem->setData(Qt::UserRole, position.z());
	table->setItem(insertRow, 2, stationItem);

	table->setItem(insertRow, 3, new QTableWidgetItem(QString::number(position.x(), 'f', precision)));
	table->setItem(insertRow, 4, new QTableWidgetItem(QString::number(position.y(), 'f', precision)));
	table->setItem(insertRow, 5, new QTableWidgetItem(QString::number(theta, 'f', precision)));

	return insertRow;
}

void AGVpath::onComboBox2CurrentIndexChanged(int index)
{
	if (index < 0) return;

	QSignalBlocker blocker(ui->comboBox_3);
	ui->comboBox_3->clear();

	// 获取当前选择的机器人和轨迹组
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_2->currentText();
	ULONG robotID = m_robotMap.key(robotName, 0);

	if (robotID == 0 || groupName.isEmpty()) {
		return;
	}

	// 获取轨迹名称并添加到comboBox_3
	QStringList pathNames = getPathNames(robotID, groupName);
	ui->comboBox_3->addItems(pathNames);

	if (!pathNames.isEmpty()) {
		onComboBox3CurrentIndexChanged(0);
	}
	
}

void AGVpath::onComboBox4CurrentIndexChanged(int index)
{
	if (index < 0) return;

	QSignalBlocker blocker(ui->comboBox_5);
	ui->comboBox_5->clear();

	// 获取当前选择的机器人和轨迹组
	QString robotName = ui->comboBox_1->currentText();
	QString groupName = ui->comboBox_4->currentText();
	ULONG robotID = m_robotMap.key(robotName, 0);

	if (robotID == 0 || groupName.isEmpty()) {
		return;
	}

	// 获取轨迹名称并添加到comboBox_3
	QStringList pathNames = getPathNames(robotID, groupName);
	ui->comboBox_5->addItems(pathNames);

	if (!pathNames.isEmpty()) {
		onComboBox5CurrentIndexChanged(0);
	}
}

void AGVpath::onComboBox3CurrentIndexChanged(int index)
{
	if (index < 0) return;

	const QString robotName = ui->comboBox_1->currentText();
	const QString groupName = ui->comboBox_2->currentText();
	const QString pathName = ui->comboBox_3->currentText();

	QString stationName = findStationNames(robotName, groupName, pathName);
	if (!stationName.isEmpty() && stationName != QStringLiteral("无")) {
		ui->textBrowser_2->setPlainText(stationName);
	}
	else {
		ui->textBrowser_2->clear();
	}

	updateVisibleStationRange();
}

void AGVpath::onComboBox5CurrentIndexChanged(int index)
{
	if (index < 0) return;

	const QString robotName = ui->comboBox_1->currentText();
	const QString groupName = ui->comboBox_4->currentText();
	const QString pathName = ui->comboBox_5->currentText();

	QString stationName = findStationNames(robotName, groupName, pathName);
	if (!stationName.isEmpty() && stationName != QStringLiteral("无")) {
		ui->textBrowser_3->setPlainText(stationName);
	}
	else {
		ui->textBrowser_3->clear();
	}

	updateVisibleStationRange();
}

QMap<ULONG, QString> AGVpath::getObjectsByType(PQDataType objType)
{
	QMap<ULONG, QString> objectMap;

	VARIANT namesVariant;
	VariantInit(&namesVariant);
	namesVariant.parray = NULL;

	VARIANT idsVariant;
	VariantInit(&idsVariant);
	idsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Doc_get_obj_bytype(objType, &namesVariant, &idsVariant);
	if (FAILED(hr)) {
		qDebug() << "获取对象列表失败，类型:" << objType << "错误码:" << hr;
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap;
	}

	// 提取名称数组
	QStringList names = extractStringArrayFromVariant(namesVariant);
	// 提取ID数组
	QList<long> ids = extractLongArrayFromVariant(idsVariant);

	// 构建映射关系
	int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; i++) {
		objectMap[ids[i]] = names[i];
	}

	VariantClear(&namesVariant);
	VariantClear(&idsVariant);

	qDebug() << "成功获取对象列表，类型:" << objType << "数量:" << objectMap.size();
	return objectMap;
}

QStringList AGVpath::extractStringArrayFromVariant(const VARIANT& variant)
{
	QStringList result;

	if ((variant.vt & VT_ARRAY) == 0 || variant.vt != (VT_ARRAY | VT_BSTR)) {
		qDebug() << "VARIANT 类型错误，期望VT_ARRAY|VT_BSTR，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		qDebug() << "SAFEARRAY 无效或维度不正确";
		return result;
	}

	// 获取数组边界
	long lowerBound, upperBound;
	HRESULT hr = SafeArrayGetLBound(array, 1, &lowerBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组下边界失败，错误码:" << hr;
		return result;
	}

	hr = SafeArrayGetUBound(array, 1, &upperBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组上边界失败，错误码:" << hr;
		return result;
	}

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		qDebug() << "数组元素数量为0或负数:" << elementCount;
		return result;
	}

	// 访问数组数据
	BSTR* data = nullptr;
	hr = SafeArrayAccessData(array, (void**)&data);
	if (FAILED(hr)) {
		qDebug() << "SafeArrayAccessData 失败，错误码:" << hr;
		return result;
	}

	// 提取所有字符串元素
	for (long i = 0; i < elementCount; i++) {
		if (data[i] != nullptr) {
			QString str = QString::fromWCharArray(data[i]);
			result.append(str);
		}
		else {
			result.append(QString()); // 空字符串处理
		}
	}

	// 取消数据访问
	SafeArrayUnaccessData(array);

	return result;
}

QList<long> AGVpath::extractLongArrayFromVariant(const VARIANT& variant)
{
	QList<long> result;

	if ((variant.vt & VT_ARRAY) == 0) {
		qDebug() << "VARIANT 类型错误，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		qDebug() << "SAFEARRAY 无效或维度不正确";
		return result;
	}

	long lowerBound, upperBound;
	SafeArrayGetLBound(array, 1, &lowerBound);
	SafeArrayGetUBound(array, 1, &upperBound);

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		return result;
	}

	result.reserve(elementCount);

	VARTYPE vt;
	SafeArrayGetVartype(array, &vt);

	HRESULT hr = SafeArrayLock(array);
	if (FAILED(hr)) {
		qDebug() << "锁定数组失败，错误码:" << hr;
		return result;
	}

	void* data = array->pvData;
	for (long i = 0; i < elementCount; i++) {
		long value = 0;
		switch (vt) {
		case VT_I4:
			value = static_cast<LONG*>(data)[i];
			break;
		case VT_I2:
			value = static_cast<SHORT*>(data)[i];
			break;
		case VT_UI4:
			value = static_cast<ULONG*>(data)[i];
			break;
		default:
			qDebug() << "不支持的数组元素类型:" << vt;
			break;
		}
		result.append(value);
	}

	SafeArrayUnlock(array);
	return result;
}


QStringList AGVpath::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
{
	QStringList robotNames;

	if (robotMap.isEmpty()) {
		return robotNames; // 返回空列表
	}

	// 遍历机器人映射表，筛选指定类型的机器人
	for (auto it = robotMap.constBegin(); it != robotMap.constEnd(); ++it) {
		long id = it.key();    // 获取机器人ID
		QString name = it.value(); // 获取机器人名称

		PQRobotType robotType = PQ_MECHANISM_ROBOT;
		HRESULT hr = m_ptrKit->Robot_get_type(id, &robotType);

		if (SUCCEEDED(hr) && robotType == mechanismType) {
			robotNames.append(name);
		}
	}

	return robotNames;
}

void AGVpath::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
{
	VARIANT vNamePara;
	vNamePara.parray = NULL;
	VARIANT vIDPara;
	vIDPara.parray = NULL;
	m_ptrKit->Doc_get_obj_bytype(i_nType, &vNamePara, &vIDPara);
	if (NULL == vNamePara.parray || NULL == vIDPara.parray)
	{
		return;
	}
	//缓存指定对象名称
	BSTR* bufName;
	long lenName = vNamePara.parray->rgsabound[0].cElements;
	SafeArrayAccessData(vNamePara.parray, (void**)&bufName);
	int nTarIndex = 0;
	if (!i_wsName.empty())
	{
		for (int i = 0; i < lenName; i++)
		{
			if (0 == i_wsName.compare(bufName[i]))
			{
				nTarIndex = i;
			}
		}
	}
	SafeArrayUnaccessData(vNamePara.parray);


	//缓存指定对象ID
	ULONG* bufID;
	long lenID = vIDPara.parray->rgsabound[0].cElements;
	SafeArrayAccessData(vIDPara.parray, (void**)&bufID);
	o_uID = bufID[nTarIndex];
	SafeArrayUnaccessData(vIDPara.parray);
}
