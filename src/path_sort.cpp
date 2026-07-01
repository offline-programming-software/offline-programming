#include "path_sort.h"

path_sort::path_sort(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent)
	, ui(new Ui::path_sortClass())
	, m_ptrKit(ptrKit)
	, m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);

	setWindowTitle("AGV定义");

	init();

	m_io = new RobxIO();

	m_io->updateData(m_list, "relations.json");

	// 初始化表格
	initTable();

	// 连接按钮信号
	connect(ui->pushButton, &QPushButton::clicked, this, &path_sort::onConfirm);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &path_sort::onCancel);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &path_sort::onAdd);
	connect(ui->pushButton_4, &QPushButton::clicked, this, &path_sort::onDelete);
}

path_sort:: ~path_sort()
{
	delete ui;
}

void path_sort::initTable()
{
	// 设置表格列数
	ui->tableWidget->setColumnCount(3);

	// 设置表头
	QStringList headers;
	headers << "机器人名称" << "导轨名称" << "AGV名称";
	ui->tableWidget->setHorizontalHeaderLabels(headers);

	// 设置表格选择模式为单行选择
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	// 设置表格自适应列宽
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	// 更新表格显示
	updateTable();
}

void path_sort::updateTable()
{
	// 清空表格内容
	ui->tableWidget->setRowCount(0);

	// 添加数据到表格
	for (const auto& relation : m_list) {
		int row = ui->tableWidget->rowCount();
		ui->tableWidget->insertRow(row);

		QString robotName = std::get<0>(relation);
		QString guideName = std::get<1>(relation);
		QString agvName = std::get<2>(relation);

		ui->tableWidget->setItem(row, 0, new QTableWidgetItem(robotName));
		ui->tableWidget->setItem(row, 1, new QTableWidgetItem(guideName));
		ui->tableWidget->setItem(row, 2, new QTableWidgetItem(agvName));
	}
}

bool path_sort::isRelationExists(const QString& robotName, const QString& guideName, const QString& agvName)
{
	// 检查机器人是否已存在关联
	for (const auto& relation : m_list) {
		if (std::get<0>(relation) == robotName) {
			QString existingGuide = std::get<1>(relation);
			QString existingAGV = std::get<2>(relation);

			if (existingGuide == guideName && existingAGV == agvName) {
				return true; // 完全相同的关系已存在
			}

			// 检查是否机器人已关联其他设备
			if (existingGuide != guideName || existingAGV != agvName) {
				QMessageBox::information(this, "提示",
					QString("机器人 '%1' 已经关联了导轨 '%2' 和AGV '%3'，无法重复关联！").arg(
						robotName, existingGuide, existingAGV));
				return true; // 机器人已关联其他设备
			}
		}

		// 检查导轨是否已存在关联
		if (std::get<1>(relation) == guideName && guideName != "无") {
			QString existingRobot = std::get<0>(relation);
			QString existingAGV = std::get<2>(relation);

			if (existingRobot != robotName || existingAGV != agvName) {
				QMessageBox::information(this, "提示",
					QString("导轨 '%1' 已经关联了机器人 '%2' 和AGV '%3'，无法重复关联！").arg(
						guideName, existingRobot, existingAGV));
				return true; // 导轨已关联其他设备
			}
		}

		// 检查AGV是否已存在关联
		if (std::get<2>(relation) == agvName && agvName != "无") {
			QString existingRobot = std::get<0>(relation);
			QString existingGuide = std::get<1>(relation);

			if (existingRobot != robotName || existingGuide != guideName) {
				QMessageBox::information(this, "提示",
					QString("AGV '%1' 已经关联了机器人 '%2' 和导轨 '%3'，无法重复关联！").arg(
						agvName, existingRobot, existingGuide));
				return true; // AGV已关联其他设备
			}
		}
	}

	return false; // 没有冲突
}

void path_sort::init()
{
	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	m_robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, m_robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		return;
	}

	// 添加机器人名称到comboBox_1
	ui->comboBox_1->addItems(robotNames);

	QStringList guideNames = getSprayRobotNames(PQ_MECHANISM_GUIDE, m_robotMap);

	guideNames.push_back("无");

	ui->comboBox_2->addItems(guideNames);

	QMap<ULONG, QString> AGVMap = getObjectsByType(PQ_WORKINGPART);

	QStringList AGVNames;
	for (auto it = AGVMap.begin(); it != AGVMap.end(); ++it) {
		ULONG AGVID = it.key();
		QString AGVName = it.value();
		AGVNames.push_back(AGVName);
	}
	AGVNames.push_back("无");

	ui->comboBox_3->addItems(AGVNames);
}

QMap<ULONG, QString> path_sort::getObjectsByType(PQDataType objType)
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

QStringList path_sort::extractStringArrayFromVariant(const VARIANT& variant)
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

QList<long> path_sort::extractLongArrayFromVariant(const VARIANT& variant)
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

QStringList path_sort::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

void path_sort::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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

void path_sort::onConfirm()
{
	// 将当前数据写入json文件
	m_io->writeData(m_list, "relations.json");
	reject();
}

void path_sort::onCancel()
{
	reject();
}

void path_sort::onAdd()
{
	QString robotName = ui->comboBox_1->currentText();
	QString guideName = ui->comboBox_2->currentText();
	QString AGVName = ui->comboBox_3->currentText();

	// 检查是否有空值
	if (robotName.isEmpty() || guideName.isEmpty() || AGVName.isEmpty()) {
		QMessageBox::warning(this, "警告", "请选择完整的数据再添加！");
		return;
	}

	// 检查关系是否已存在
	if (isRelationExists(robotName, guideName, AGVName)) {
		return; // 如果关系存在，则不添加
	}

	std::tuple<QString, QString, QString> relation(robotName, guideName, AGVName);

	// 添加数据到列表
	m_list.push_back(relation);

	// 更新表格显示
	updateTable();

	// 提示用户
	QMessageBox::information(this, "提示", "数据添加成功！");
}

void path_sort::onDelete()
{
	// 获取当前选中的行
	QList<QTableWidgetItem*> selectedItems = ui->tableWidget->selectedItems();
	if (selectedItems.isEmpty()) {
		QMessageBox::information(this, "提示", "请先选择要删除的行！");
		return;
	}

	// 获取选中行的索引
	int selectedRow = selectedItems.first()->row();

	// 确认删除
	QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除",
		QString("确定要删除第%1行的数据吗？").arg(selectedRow + 1),
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		// 从列表中删除对应数据
		m_list.remove(selectedRow);

		// 从表格中删除对应行
		ui->tableWidget->removeRow(selectedRow);

		// 提示用户
		QMessageBox::information(this, "提示", "数据删除成功！");
	}
}