#include "robotSpaceDefine.h"

robotSpaceDefine::robotSpaceDefine(QWidget *parent,
	CComPtr<IPQPlatformComponent> ptrKit,
	CPQKitCallback* ptrKitCallback)
	: QDialog(parent),
	ui(new Ui::robotSpaceDefineClass()),
	axisModel(new QStandardItemModel(this)),
	m_ptrKit(ptrKit),
	m_ptrKitCallback(ptrKitCallback)
{
	ui->setupUi(this);

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSpraydRobotNames(PQ_MECHANISM_ROBOT, robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		delete ui;
		return; // 早期返回
	}

	// 将机器人名称设置到对话框中
	for (const QString& name : robotNames) {
		ui->comboBox->addItem(name);
	}

	ui->comboBox->setCurrentIndex(0);
	// 初始化表格视图
	setupTableView();

	// 连接信号槽（假设UI中有添加和删除按钮）
	connect(ui->pushButton, &QPushButton::clicked, this, &robotSpaceDefine::onAddAxis);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &robotSpaceDefine::onDeleteAxis);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &robotSpaceDefine::onConfirm);
	connect(ui->pushButton_4, &QPushButton::clicked, this, &robotSpaceDefine::onClose);

	// 新增：连接机器人切换信号
	connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &robotSpaceDefine::onRobotChanged);

	m_io = new RobxIO();
	m_io->updateData(m_list, "workspace.json");

	// 加载所有机器人的数据
	for (const QString& robotName : robotNames) {
		QString filename = QString("workSpaceInformation_%1.json").arg(robotName);
		QVector<workSpaceInformation> robotData;
		m_io->updateData(robotData, filename.toStdString().c_str());
		m_spaceInformation[robotName] = robotData;
	}

	// 加载当前选中机器人的数据到表格
	QString currentRobot = ui->comboBox->currentText();
	loadRobotData(currentRobot);
}

robotSpaceDefine::~robotSpaceDefine()
{
	delete m_io; // 修复内存泄漏
	delete ui;
}

void robotSpaceDefine::setupTableView()
{
	// 设置表头
	QStringList headers;
	headers << "序号" << "坐标系名称" << "法向量" << "是否有导轨" << "导轨名称";
	axisModel->setHorizontalHeaderLabels(headers);

	// 设置模型
	ui->tableView->setModel(axisModel);

	// 设置选择行为
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

	// 设置列宽
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->setColumnWidth(0, 60);
	ui->tableView->setColumnWidth(1, 100);
	ui->tableView->setColumnWidth(2, 100);
	ui->tableView->setColumnWidth(3, 80);
	ui->tableView->setColumnWidth(4, 120);
}

void robotSpaceDefine::addAxisInfo(int number, const QString& axisName,
	const QString& mainNormalVector, bool hasGuideRail, const QString guideName)
{
	AxisData data;
	data.number = number;
	data.axisName = axisName;
	data.mainNormalVector = mainNormalVector;
	data.hasGuideRail = hasGuideRail;
	data.guideName = guideName;

	axisList.append(data);
	updateTableView();
}

QString robotSpaceDefine::getRobotName()
{
	QString RobotName = ui->comboBox->currentText();
	return RobotName;
}

void robotSpaceDefine::updateTableView()
{
	axisModel->removeRows(0, axisModel->rowCount());

	for (int i = 0; i < axisList.size(); ++i) {
		const AxisData& data = axisList.at(i);

		QList<QStandardItem*> items;

		// 自动生成序号（从1开始）
		QStandardItem* numberItem = new QStandardItem(QString::number(i + 1));
		numberItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(numberItem);

		// 轴名称
		QStandardItem* nameItem = new QStandardItem(data.axisName);
		nameItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(nameItem);

		// 法向量
		QStandardItem* normalItem = new QStandardItem(data.mainNormalVector);
		normalItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(normalItem);

		// 是否有导轨
		QStandardItem* railItem = new QStandardItem(data.hasGuideRail ? "是" : "否");
		railItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐
		items.append(railItem);

		// 导轨名称
		QStandardItem* speedItem = new QStandardItem(data.guideName);
		speedItem->setTextAlignment(Qt::AlignCenter);  // 居中对齐（之前是右对齐）
		items.append(speedItem);

		axisModel->appendRow(items);
	}
}

void robotSpaceDefine::onAddAxis()
{
	// 检查m_ptrKit是否已初始化
	if (!m_ptrKit) {
		QMessageBox::warning(this, "错误", "平台组件未初始化");
		return;
	}

	addRobotSpace* dlg = new addRobotSpace();
	QString robotName = getRobotName();
	dlg->setRobotName(robotName);

	// 如果机器人名称为空，提示用户
	if (robotName.isEmpty()) {
		QMessageBox::information(this, "提示", "请先选择机器人");
		return;
	}

	PQDataType CoodernateType = PQ_COORD;
	QMap<ULONG, QString> CoodernateMap = getObjectsByType(CoodernateType);

	// 创建新的坐标系列表，添加"坐标系"选项
	QMap<ULONG, QString> newCoodernateMap;
	newCoodernateMap.insert(0, "世界坐标系");  // 默认选项

	// 将原有数据添加到新map中（从1开始）
	for (auto it = CoodernateMap.begin(); it != CoodernateMap.end(); ++it) {
		newCoodernateMap.insert(it.key(), it.value());
	}

	CoodernateMap = newCoodernateMap;
	QStringList CoodernateNames = CoodernateMap.values();
	dlg->setCoordinate(CoodernateNames); // 设置坐标系

	// 获取导轨信息
	ULONG uExternalID = 0;
	QString railname = robotName + "_rail";
	GetObjIDByName(PQ_ROBOT, railname.toStdWString(), uExternalID);

	QStringList rail;

	// 只有在成功获取到机器人ID时才尝试获取关节信息
	if (uExternalID != 0) {
		int railCount = 0;
		HRESULT hr = m_ptrKit->Doc_get_obj_joint_count(uExternalID, &railCount);

		if (SUCCEEDED(hr) && railCount > 0) {
			for (int i = 0; i < railCount; i++) {
				QString railName = "J" + QString::number(i + 1);
				rail.append(railName);
			}
		}
	}

	// 即使没有导轨也要设置空列表
	dlg->setRail(rail);


	connect(dlg, &addRobotSpace::calculateRequested, this, [this, robotName, dlg, CoodernateMap]() {

		spacePoint center(0, 0, 0);
		spacePoint size(0, 0, 0);

		Workspace spaceModel(center, size, m_ptrKit, m_ptrKitCallback);

		ULONG robotID = 0;
		GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);
		spacePoint centerPoint = spaceModel.calculateRobotWorkspaceCenter(robotID);
		int number = ui->tableView->model()->rowCount() + 1; // 序号从1开始

		//获取主法矢量
		QString axisName = dlg->getCoordinate();
		QString mainNormalVector = dlg->getMainDir();
		ULONG zuobiaoximingcheng;
		GetObjIDByName(PQ_COORD, axisName.toStdWString(), zuobiaoximingcheng);

		std::vector<double> direction = getDir(CoodernateMap.key(axisName), mainNormalVector);

		bool hasGuideRail = dlg->isLink();
		QString guideName = dlg->getRail();

		std::map<std::pair<double, double>, std::vector<spacePoint>> inputMap;
		inputMap = spaceModel.calculateRobotSpaceRange(robotID, centerPoint, 10, 50,
			1500, 0, 22.5, direction, 5, 10.0);

		for (const auto& keyValuePair : inputMap) { // 遍历map
			workSpace ws;
			ws.robotID = robotID; // 设置robotID
			ws.thickness = keyValuePair.first.first;  // 键(pair)的第一个double
			ws.theta = keyValuePair.first.second;     // 键(pair)的第二个double

			// 转换 vector<spacePoint> 到 vector<double>
			for (const auto& sp : keyValuePair.second) { // 遍历vector<spacePoint>
				// 假设我们取 spacePoint 的 x 成员
				ws.points.push_back(sp.x);
				ws.points.push_back(sp.y);
				ws.points.push_back(sp.z);
			}

			m_list.push_back(ws);
		}

		m_io->writeData(m_list, "workspace.json");

		if (hasGuideRail) {
			addAxisInfo(number, axisName, mainNormalVector, hasGuideRail, guideName);
		}
		else {
			addAxisInfo(number, axisName, mainNormalVector, hasGuideRail, " ");
		}

		dlg->close();
	});


	// 显示对话框并处理结果
	if (dlg->exec() == QDialog::Accepted) {
		// 这里可以添加从对话框获取数据并更新轴列表的逻辑
		// 例如：addAxisInfo(...);
	}
}

void robotSpaceDefine::onDeleteAxis()
{
	QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
	if (selected.isEmpty()) {
		QMessageBox::warning(this, "警告", "请选择要删除的坐标轴");
		return;
	}

	int row = selected.first().row();
	if (row >= 0 && row < axisList.size()) {
		axisList.removeAt(row);
		updateTableView();
	}
}

void robotSpaceDefine::onConfirm()
{
	QString robotName = ui->comboBox->currentText();

	// 清空当前机器人的数据
	m_spaceInformation[robotName].clear();

	int rows = ui->tableView->model()->rowCount();
	for (int row = 0; row < rows; ++row) {
		QVariant data = axisModel->data(axisModel->index(row, 0));
		int number = data.toInt(); // 如果明确是文本

		QVariant data_1 = axisModel->data(axisModel->index(row, 1));
		QString coordinate = data_1.toString(); // 如果明确是文本

		QVariant data_2 = axisModel->data(axisModel->index(row, 2));
		QString mainDir = data_2.toString(); // 如果明确是文本

		QVariant data_3 = axisModel->data(axisModel->index(row, 3));
		QString m_isLink = data_3.toString(); // 如果明确是文本

		bool isLink;
		if (m_isLink == "是") {
			isLink = 1;
		}
		else {
			isLink = 0;
		}

		QVariant data_4 = axisModel->data(axisModel->index(row, 4));
		QString railName = data_4.toString(); // 如果明确是文本

		workSpaceInformation spaceDate(robotName, number, coordinate, mainDir, isLink, railName);
		m_spaceInformation[robotName].push_back(spaceDate);
	}

	// 保存当前机器人的数据到对应文件
	QString filename = QString("workSpaceInformation_%1.json").arg(robotName);
	m_io->writeData(m_spaceInformation[robotName], filename.toStdString().c_str());

	this->reject();
}

void robotSpaceDefine::onClose()
{
	this->reject();
}

void robotSpaceDefine::onRobotChanged(int index)
{
	QString currentRobot = ui->comboBox->itemText(index);

	// 保存当前机器人的数据（如果需要的话）
	// 可以选择在切换时自动保存，或者只在确认时保存

	// 加载新选择机器人的数据
	loadRobotData(currentRobot);
}

void robotSpaceDefine::loadRobotData(const QString& robotName)
{
	// 清空当前表格数据
	axisList.clear();

	// 从m_spaceInformation中获取对应机器人的数据
	if (m_spaceInformation.contains(robotName)) {
		QVector<workSpaceInformation> robotData = m_spaceInformation[robotName];
		for (const workSpaceInformation& data : robotData) {
			addAxisInfo(data.number, data.coodinate, data.mainDir, data.isLink, data.railName);
		}
	}

	// 更新表格显示
	updateTableView();
}

std::vector<double> robotSpaceDefine::getDir(ULONG coordinateID, QString mainDir)
{
	std::vector<double> dir;

	// 如果 coordinateID 为 0，直接返回世界坐标系的基础方向
	if (coordinateID == 0) {
		if (mainDir == "X轴正方向") {
			dir = { 1.0, 0.0, 0.0 };
		}
		else if (mainDir == "X轴负方向") {
			dir = { -1.0, 0.0, 0.0 };
		}
		else if (mainDir == "Y轴正方向") {
			dir = { 0.0, 1.0, 0.0 };
		}
		else if (mainDir == "Y轴负方向") {
			dir = { 0.0, -1.0, 0.0 };
		}
		else if (mainDir == "Z轴正方向") {
			dir = { 0.0, 0.0, 1.0 };
		}
		else if (mainDir == "Z轴负方向") {
			dir = { 0.0, 0.0, -1.0 };
		}
		return dir;
	}

	int nCount = 0;
	double* dPosture = nullptr;
	m_ptrKit->Doc_get_coordinate_posture(coordinateID, QUATERNION, &nCount, &dPosture, 0);

	std::vector<double> coordinatePos;

	for (int i = 0; i < nCount; i++) {
		coordinatePos.push_back(dPosture[i]);
	}

	// 四元数顺序为 [w, x, y, z]
	if (coordinatePos.size() >= 4) {
		double w = coordinatePos[0]; // w
		double x = coordinatePos[1]; // x
		double y = coordinatePos[2]; // y
		double z = coordinatePos[3]; // z

		if (mainDir == "X轴正方向") {
			// 计算旋转后的X轴正方向向量
			double xx = 1 - 2 * y*y - 2 * z*z;     // 旋转后的X分量
			double xy = 2 * x*y + 2 * w*z;         // 旋转后的Y分量
			double xz = 2 * x*z - 2 * w*y;         // 旋转后的Z分量

			dir = { xx, xy, xz };
		}
		else if (mainDir == "X轴负方向") {
			// 计算旋转后的X轴负方向向量
			double xx = 1 - 2 * y*y - 2 * z*z;     // 旋转后的X分量
			double xy = 2 * x*y + 2 * w*z;         // 旋转后的Y分量
			double xz = 2 * x*z - 2 * w*y;         // 旋转后的Z分量

			dir = { -xx, -xy, -xz }; // 取负值
		}
		else if (mainDir == "Y轴正方向") {
			// 计算旋转后的Y轴正方向向量
			double yx = 2 * x*y - 2 * w*z;         // 旋转后的X分量
			double yy = 1 - 2 * x*x - 2 * z*z;     // 旋转后的Y分量
			double yz = 2 * y*z + 2 * w*x;         // 旋转后的Z分量

			dir = { yx, yy, yz };
		}
		else if (mainDir == "Y轴负方向") {
			// 计算旋转后的Y轴负方向向量
			double yx = 2 * x*y - 2 * w*z;         // 旋转后的X分量
			double yy = 1 - 2 * x*x - 2 * z*z;     // 旋转后的Y分量
			double yz = 2 * y*z + 2 * w*x;         // 旋转后的Z分量

			dir = { -yx, -yy, -yz }; // 取负值
		}
		else if (mainDir == "Z轴正方向") {
			// 计算旋转后的Z轴正方向向量
			double zx = 2 * x*z + 2 * w*y;         // 旋转后的X分量
			double zy = 2 * y*z - 2 * w*x;         // 旋转后的Y分量
			double zz = 1 - 2 * x*x - 2 * y*y;     // 旋转后的Z分量

			dir = { zx, zy, zz };
		}
		else if (mainDir == "Z轴负方向") {
			// 计算旋转后的Z轴负方向向量
			double zx = 2 * x*z + 2 * w*y;         // 旋转后的X分量
			double zy = 2 * y*z - 2 * w*x;         // 旋转后的Y分量
			double zz = 1 - 2 * x*x - 2 * y*y;     // 旋转后的Z分量

			dir = { -zx, -zy, -zz }; // 取负值
		}
	}

	m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);

	return dir;
}

QMap<ULONG, QString> robotSpaceDefine::getObjectsByType(PQDataType objType)
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
		qDebug() << "获取类型" << objType << "的对象列表失败！错误码:" << hr;
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap; // 返回空映射
	}

	SAFEARRAY* nameArray = namesVariant.parray;
	SAFEARRAY* idArray = idsVariant.parray;

	// 检查数组有效性
	if (!nameArray || nameArray->cDims != 1 ||
		!idArray || idArray->cDims != 1 ||
		nameArray->rgsabound[0].cElements == 0) {
		qDebug() << "类型" << objType << "的对象列表为空或格式错误";
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objectMap; // 返回空映射
	}

	// 提取名称数组（字符串）
	QStringList names = extractStringArrayFromVariant(namesVariant);
	// 提取ID数组（long类型）
	QList<long> ids = extractLongArrayFromVariant(idsVariant);

	// 构建映射（假设名称和ID数组长度相同）
	int minSize = qMin(names.size(), ids.size());
	for (int i = 0; i < minSize; i++) {
		objectMap[ids[i]] = names[i];
	}

	// 清理资源
	VariantClear(&namesVariant);
	VariantClear(&idsVariant);

	qDebug() << "成功获取类型" << objType << "的对象列表，共" << objectMap.size() << "个对象";
	return objectMap;
}

// 提取long类型数组的函数
QList<long> robotSpaceDefine::extractLongArrayFromVariant(const VARIANT& variant)
{
	QList<long> result;

	// 检查 VARIANT 是否为数组类型
	if ((variant.vt & VT_ARRAY) == 0) {
		qDebug() << "VARIANT 不是数组类型，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array) {
		qDebug() << "SAFEARRAY 指针为空";
		return result;
	}

	// 检查数组维度
	if (array->cDims != 1) {
		qDebug() << "SAFEARRAY 维度不正确，期望1维，实际:" << array->cDims;
		return result;
	}

	// 获取数组边界
	long lowerBound, upperBound;
	HRESULT hr = SafeArrayGetLBound(array, 1, &lowerBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组下界失败，错误码:" << hr;
		return result;
	}

	hr = SafeArrayGetUBound(array, 1, &upperBound);
	if (FAILED(hr)) {
		qDebug() << "获取数组上界失败，错误码:" << hr;
		return result;
	}

	long elementCount = upperBound - lowerBound + 1;
	if (elementCount <= 0) {
		qDebug() << "数组元素数量为0或负数:" << elementCount;
		return result;
	}

	// 预分配结果列表空间以提高性能
	result.reserve(elementCount);

	// 检查数组元素类型
	VARTYPE vt;
	hr = SafeArrayGetVartype(array, &vt);
	if (FAILED(hr)) {
		qDebug() << "获取数组元素类型失败，错误码:" << hr;
		return result;
	}

	// 锁定数组
	hr = SafeArrayLock(array);
	if (FAILED(hr)) {
		qDebug() << "锁定数组失败，错误码:" << hr;
		return result;
	}
	switch (vt) {
	case VT_I4: // 32位整数
	{
		LONG* data = static_cast<LONG*>(array->pvData);
		for (long i = 0; i < elementCount; i++) {
			result.append(data[i]);
		}
		break;
	}
	case VT_I2: // 16位整数
	{
		SHORT* data = static_cast<SHORT*>(array->pvData);
		for (long i = 0; i < elementCount; i++) {
			// 检查是否溢出
			if (data[i] < LONG_MIN || data[i] > LONG_MAX) {
				qDebug() << "16位整数转换到long可能溢出，值:" << data[i];
			}
			result.append(static_cast<long>(data[i]));
		}
		break;
	}
	case VT_I8: // 64位整数
	{
		LONGLONG* data = static_cast<LONGLONG*>(array->pvData);
		for (long i = 0; i < elementCount; i++) {
			// 检查是否溢出
			if (data[i] < LONG_MIN || data[i] > LONG_MAX) {
				qDebug() << "64位整数转换到long可能溢出，值:" << data[i];
				result.append(0); // 溢出时使用默认值
			}
			else {
				result.append(static_cast<long>(data[i]));
			}
		}
		break;
	}
	case VT_UI4: // 无符号32位整数
	{
		ULONG* data = static_cast<ULONG*>(array->pvData);
		for (long i = 0; i < elementCount; i++) {
			if (data[i] > static_cast<ULONG>(LONG_MAX)) {
				qDebug() << "无符号32位整数转换到long可能溢出，值:" << data[i];
			}
			result.append(static_cast<long>(data[i]));
		}
		break;
	}
	case VT_BSTR: // 字符串类型
	{
		BSTR* data = static_cast<BSTR*>(array->pvData);
		for (long i = 0; i < elementCount; i++) {
			if (data[i] != nullptr && SysStringLen(data[i]) > 0) {
				QString str = QString::fromWCharArray(data[i]);
				bool ok;
				long value = str.toLong(&ok);
				if (ok) {
					result.append(value);
				}
				else {
					qDebug() << "无法将字符串转换为long，索引:" << i << "值:" << str;
					result.append(0); // 默认值
				}
			}
			else {
				result.append(0); // 空字符串的默认值
			}
		}
		break;
	}
	case VT_R4: // 单精度浮点数
	case VT_R8: // 双精度浮点数
	{
		// 处理浮点数类型，四舍五入到最接近的long
		if (vt == VT_R4) {
			FLOAT* data = static_cast<FLOAT*>(array->pvData);
			for (long i = 0; i < elementCount; i++) {
				result.append(lround(data[i]));
			}
		}
		else {
			DOUBLE* data = static_cast<DOUBLE*>(array->pvData);
			for (long i = 0; i < elementCount; i++) {
				result.append(lround(data[i]));
			}
		}
		break;
	}
	default:
		qDebug() << "不支持的数组类型:" << vt;
		break;
	}
	// 解锁数组
	SafeArrayUnlock(array);

	return result;
}

// 提取字符串数组的函数（保持不变）
QStringList robotSpaceDefine::extractStringArrayFromVariant(const VARIANT& variant)
{
	QStringList result;

	// 检查 VARIANT 类型
	if ((variant.vt & VT_ARRAY) == 0) {
		qDebug() << "VARIANT 不是数组类型，实际类型:" << variant.vt;
		return result;
	}

	SAFEARRAY* array = variant.parray;
	if (!array || array->cDims != 1) {
		qDebug() << "SAFEARRAY 无效或维度不正确";
		return result;
	}

	// 检查数组元素类型
	VARTYPE vt;
	SafeArrayGetVartype(array, &vt);

	if (vt != VT_BSTR) {
		qDebug() << "期望的数组类型是 VT_BSTR，但实际类型是:" << vt;
		return result;
	}

	// 锁定数组
	BSTR* data = nullptr;
	HRESULT hr = SafeArrayAccessData(array, (void**)&data);
	if (FAILED(hr)) {
		qDebug() << "SafeArrayAccessData 失败，错误码:" << hr;
		return result;
	}

	long lowerBound, upperBound;
	SafeArrayGetLBound(array, 1, &lowerBound);
	SafeArrayGetUBound(array, 1, &upperBound);

	long elementCount = upperBound - lowerBound + 1;

	// 提取所有元素
	for (long i = 0; i < elementCount; i++) {
		BSTR bstr = data[i];
		if (bstr != nullptr) {
			// 将 BSTR 转换为 QString
			QString str = QString::fromWCharArray(bstr);
			result.append(str);
		}
		else {
			result.append(QString()); // 空字符串
		}
	}

	// 解锁数组
	SafeArrayUnaccessData(array);

	return result;
}

void robotSpaceDefine::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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

QStringList robotSpaceDefine::getSpraydRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

QStringList robotSpaceDefine::getPathGroupNames(ULONG robotID)
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

QStringList robotSpaceDefine::getPathNames(ULONG robotID, const QString & groupName)
{
	QStringList pathNames;

	VARIANT sNames;
	VARIANT sIDs;
	VariantInit(&sNames);
	VariantInit(&sIDs);
	sNames.parray = NULL;
	sIDs.parray = NULL;

	// 将QString转换为std::wstring，再获取C风格宽字符串指针
	std::wstring wstrGroupName = groupName.toStdWString();
	BSTR bstrGroupName = SysAllocString(wstrGroupName.c_str());

	// 或者更简洁的一行写法：
	// BSTR bstrGroupName = SysAllocString(groupName.toStdWString().c_str());

	HRESULT hr = m_ptrKit->Path_get_group_path(robotID, bstrGroupName, &sNames, &sIDs);

	SysFreeString(bstrGroupName);

	if (SUCCEEDED(hr)) {
		pathNames = extractStringArrayFromVariant(sNames);
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