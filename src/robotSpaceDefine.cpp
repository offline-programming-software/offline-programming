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


	// 初始化表格视图
	setupTableView();

	// 连接信号槽（假设UI中有添加和删除按钮）
	connect(ui->pushButton, &QPushButton::clicked, this, &robotSpaceDefine::onAddAxis);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &robotSpaceDefine::onDeleteAxis);
	connect(ui->pushButton_3, &QPushButton::clicked, this, &robotSpaceDefine::onConfirm);
	connect(ui->pushButton_4, &QPushButton::clicked, this, &robotSpaceDefine::onClose);

	m_io = new RobxIO();
	m_io->updateData(m_list, "workspace.json");
	m_io->updateData(m_spaceInformation, "workSpaceInformation.json");

	for (workSpaceInformation data : m_spaceInformation) {
		addAxisInfo(data.number, data.coodinate, data.mainDir, data.isLink, data.railName);;
	}
}

robotSpaceDefine::~robotSpaceDefine()
{
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

void robotSpaceDefine::addAxisInfo(int number,const QString& axisName, 
	const QString& mainNormalVector,bool hasGuideRail, const QString guideName)
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

void robotSpaceDefine::setRobotOptions(const QString & robotOption)
{
	if (!robotOption.isEmpty()) {
		ui->comboBox->addItem(robotOption);
		ui->comboBox->setCurrentIndex(0);
	}
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
		newCoodernateMap.insert(it.key() + 1, it.value());
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


	connect(dlg, &addRobotSpace::calculateRequested, this, [this, robotName, dlg]() {
		
		spacePoint center(0, 0, 0);
		spacePoint size(0, 0, 0);

		Workspace spaceModel(center, size, m_ptrKit, m_ptrKitCallback);

		ULONG robotID = 0;
		GetObjIDByName(PQ_ROBOT, robotName.toStdWString(), robotID);
		spacePoint centerPoint = spaceModel.calculateRobotWorkspaceCenter(robotID);
		std::vector<double> direction = {-1,0,0};

		std::map<std::pair<double, double>, std::vector<spacePoint>> inputMap;
		inputMap = spaceModel.calculateRobotSpaceRange(robotID, centerPoint, 50, 50,
			1500,0 , 22.5, direction, 5, 1.0);

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

		//如何实现对于number 获取
		int number = ui->tableView->model()->rowCount();
		QString axisName = dlg->getCoordinate();
		QString mainNormalVector = dlg->getMainDir();
		bool hasGuideRail = dlg->isLink();
		QString guideName = dlg->getRail();
		
		if (hasGuideRail) {

			addAxisInfo(number, axisName, mainNormalVector,hasGuideRail, guideName);
			
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

	m_spaceInformation.clear();

	QString robotName = ui->comboBox->currentText();

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
		m_spaceInformation.push_back(spaceDate);

	}

	m_io->writeData(m_spaceInformation, "workSpaceInformation.json");

	this->reject();
}

void robotSpaceDefine::onClose()
{
	this->reject();
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

