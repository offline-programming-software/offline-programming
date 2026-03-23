#pragma execution_character_set("utf-8")	

#include "MainWindow.h"
#include "SARibbon.h"
#include <QComboBox>
#include <QTimer>
#include <QFileDialog>
#include <QSplitter>
#include <QWindow>
#include <QChart>
#include <string>
#include"ui\PositionCorrectWidget.h"
#include"ui\BendingManagerWidget.h"
#include"model\CorrectionModel.h"
#include"core\Correction.h"
#include"ui\PositionCorrectWidget.h"
#include"robxFileIO.h"
#include "test\RobxFileIOManagerWidget.h"


MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
	setWindowIcon(QIcon(":/image/resource/APP.svg"));
	SARibbonBar* bar = ribbonBar();

	//******************//
	//添加编程仿真**菜单**//
	//******************//
	SARibbonCategory* sence = bar->addCategoryPage("  编程仿真  ");
	//添加文件**Pannel**
	SARibbonPannel*  file = sence->addPannel("文件");
	QAction* action1 = file->addAction("打开", QIcon(":/image/resource/open.png"), QToolButton::InstantPopup);
	QAction* action2 = file->addAction("保存", QIcon(":/image/resource/save.png"), QToolButton::InstantPopup);
		connect(action1, SIGNAL(triggered()), this, SLOT(OnOpenRobx()));//打开
		connect(action2, SIGNAL(triggered()), this, SLOT(OnSaveRobx()));//保存
	//添加模型导入**pannel**
	SARibbonPannel* impoertModel = sence->addPannel("模型导入");
	QAction* action5 = impoertModel->addAction("外部模型导入", QIcon(":/image/resource/icons8-folder-32.png"), QToolButton::InstantPopup);
	QAction* action24 = impoertModel->addAction("导入工具", QIcon(":/image/resource/icons8-imtool.png"), QToolButton::InstantPopup);
	QAction* action20 = impoertModel->addAction("导入机构", QIcon(":/image/resource/2.png"), QToolButton::InstantPopup);
	QAction* action22 = impoertModel->addAction("导入零件", QIcon(":/image/resource/icons-gear.png"), QToolButton::InstantPopup);
		connect(action20, SIGNAL(triggered()), this, SLOT(on_import_institutions()));//导入机构
		connect(action22, SIGNAL(triggered()), this, SLOT(on_import_part()));//导入零件
		connect(action24, SIGNAL(triggered()), this, SLOT(on_import_tool()));//导入工具
		connect(action5, SIGNAL(triggered()), this, SLOT(on_input()));//输入
	//添加场景搭建**Pannel**
	SARibbonPannel*  import = sence->addPannel("场景搭建");
	QAction* action6 = import->addAction("三维球", QIcon(":/image/resource/5.png"), QToolButton::InstantPopup);
		connect(action6, SIGNAL(triggered()), this, SLOT(on_proxy()));//三维球
	QAction* action8 = import->addAction("新建坐标系", QIcon(":/image/resource/icons8-coordinate-system-32.png"), QToolButton::InstantPopup);
		connect(action8, SIGNAL(triggered()), this, SLOT(on_create_frame()));//新建坐标系
	//添加运动机构定义**Pannel**
	SARibbonPannel* def = sence->addPannel("运动机构定义");
	QAction* action19 = def->addAction("定义机构", QIcon(":/image/resource/1.png"), QToolButton::InstantPopup);
		connect(action19, SIGNAL(triggered()), this, SLOT(on_defining_institutions()));//定义机构
	//添加喷涂工具**pannel**
	SARibbonPannel* spy = sence->addPannel("喷涂工具");
	QAction* action25 = spy->addAction("定义喷涂工具", QIcon(":/image/resource/pentu.png"), QToolButton::InstantPopup);
		connect(action25, SIGNAL(triggered()), this, SLOT(on_rocreate_sprayingtool()));//定义喷涂工具
	QAction* action26 = spy ->addAction("喷刷管理", QIcon(":/image/resource/icons8-spray.png"), QToolButton::InstantPopup);
		connect(action26, SIGNAL(triggered()), this, SLOT(on_roobjassistor_manage()));//喷刷管理
	//添加轨迹规划**Pannel**
	SARibbonPannel*  path = sence->addPannel("轨迹规划");
	QAction* pathGen = path->addAction("轨迹生成", QIcon(""),QToolButton::InstantPopup);
		connect(pathGen, SIGNAL(triggered()), this, SLOT(on_create_path()));
	QAction* action69 = path->addAction("区域划分", QIcon(":/image/resource/33.png"), QToolButton::InstantPopup);
		connect(action69, SIGNAL(triggered()), this, SLOT(on_curse_part()));//喷涂区域划分
	QAction* action10 = path->addAction("喷涂路径生成", QIcon(":/image/resource/30.png"), QToolButton::InstantPopup);
		connect(action10, SIGNAL(triggered()), this, SLOT(on_campath_flat_surface()));//随形喷涂
	//添加轨迹修正**Pannel**
	SARibbonPannel* correction = sence->addPannel("轨迹修正");
	QAction* action101 = correction->addAction("变形修正设置", QIcon(":/image/resource/bendingFunc.png"), QToolButton::InstantPopup);
	QAction* action102 = correction->addAction("对象变形修正", QIcon(":/image/resource/bendingmanager .png"), QToolButton::InstantPopup);
	QAction* action103 = correction->addAction("对象偏移修正", QIcon(":/image/resource/positioncorrect.png"), QToolButton::InstantPopup);
		connect(action101, SIGNAL(triggered()), this, SLOT(on_trajCorrectdock_open()));//输出动画
		connect(action102, &QAction::triggered, this, &MainWindow::on_bendingManagerWidget_open);
		connect(action103, &QAction::triggered, this, &MainWindow::on_PositionCorrectWidget_open);
	//添加仿真**pannel**
	SARibbonPannel*  complie = sence->addPannel("仿真调试");
	QAction* action13 = complie->addAction("编译", QIcon(":/image/resource/compile.png"), QToolButton::InstantPopup);
	QAction* action14 = complie->addAction("仿真", QIcon(":/image/resource/simulate.png"), QToolButton::InstantPopup);
	QAction* action15 = complie->addAction("后置", QIcon(":/image/resource/26.png"), QToolButton::InstantPopup);
		connect(action13, SIGNAL(triggered()), this, SLOT(OnCompile()));//编译
		connect(action14, SIGNAL(triggered()), this, SLOT(on_simulate()));//仿真
		connect(action15, SIGNAL(triggered()), this, SLOT(on_post()));//后置
	QAction* action100 = complie->addAction("批量后置", QIcon(":/image/resource/26.png"), QToolButton::InstantPopup);
		connect(action100, SIGNAL(triggered()), this, SLOT(on_numPost()));
	//添加设置
	SARibbonPannel*  set = sence->addPannel("设置");
	QAction* action16 = set->addAction("地面", QIcon(":/image/resource/floor.png"), QToolButton::InstantPopup);
	QAction* action17 = set->addAction("选项", QIcon(":/image/resource/icons8-automatic-100.png"), QToolButton::InstantPopup);
	QAction* action18 = set->addAction("碰撞设置", QIcon(":/image/resource/28.png"), QToolButton::InstantPopup);
		connect(action16, SIGNAL(triggered()), this, SLOT(on_floor()));//地面
		connect(action17, SIGNAL(triggered()), this, SLOT(on_motion_option()));//选项
		connect(action18, SIGNAL(triggered()), this, SLOT(on_collision_option()));//碰撞设置



	//******************//
	//添加定义机构**菜单**//
	//******************//
	SARibbonCategory* definition = bar->addCategoryPage("定义机构");
	SARibbonPannel* pannel = definition->addPannel("机构");
	SARibbonPannel* part = definition->addPannel("零件");
	SARibbonPannel* tool = definition->addPannel("工具");
	//添加菜单栏
	QAction* action21 = part->addAction("定义零件", QIcon(":/image/resource/icons8-tool.png"), QToolButton::InstantPopup);
		connect(action21, SIGNAL(triggered()), this, SLOT(on_defining_part()));//定义零件
	QAction* action23 = tool->addAction("定义工具", QIcon(":/image/resource/icons8-detool.png"), QToolButton::InstantPopup);
		connect(action23, SIGNAL(triggered()), this, SLOT(on_defining_tool()));//导入工具

	//******************//
	//添加自由绘制**菜单**//
	//******************//
	SARibbonCategory* sketch = bar->addCategoryPage("自由绘制");
	SARibbonPannel* add = sketch->addPannel("创建基本图元");
	SARibbonPannel* create = sketch->addPannel("创建草图");
	
	QAction* action27 = create->addAction("创建草图", QIcon(":/image/resource/9.png"), QToolButton::InstantPopup);
	connect(action27, SIGNAL(triggered()), this, SLOT(on_create_newsketch()));//创建草图
	QAction* action28 = create->addAction("编辑草图", QIcon(":/image/resource/icons8-create-32.png"), QToolButton::InstantPopup);
	connect(action28, SIGNAL(triggered()), this, SLOT(on_edit_wkptsketch()));//编辑草图
	QAction* action29 = add->addAction("点", QIcon(":/image/resource/10.png"), QToolButton::InstantPopup);
	connect(action29, SIGNAL(triggered()), this, SLOT(on_sketcher_createpoint()));//点
	QAction* action30 = add->addAction("直线", QIcon(":/image/resource/11.png"), QToolButton::InstantPopup);
	connect(action30, SIGNAL(triggered()), this, SLOT(on_sketcher_line()));//直线
	QAction* action31 = add->addAction("圆", QIcon(":/image/resource/12.png"), QToolButton::InstantPopup);
	connect(action31, SIGNAL(triggered()), this, SLOT(on_sketcher_circ()));//圆
	QAction* action32 = add->addAction("圆弧", QIcon(":/image/resource/13.png"), QToolButton::InstantPopup);
	connect(action32, SIGNAL(triggered()), this, SLOT(on_sketcher_arc()));//圆弧
	QAction* action33 = add->addAction("椭圆弧", QIcon(":/image/resource/14.png"), QToolButton::InstantPopup);
	connect(action33, SIGNAL(triggered()), this, SLOT(on_sketcher_ellipticalarc()));//椭圆弧
	QAction* action34 = add->addAction("椭圆", QIcon(":/image/resource/21.png"), QToolButton::InstantPopup);
	connect(action34, SIGNAL(triggered()), this, SLOT(on_sketcher_ellipse()));//椭圆
	QAction* action35 = add->addAction("矩形", QIcon(":/image/resource/15.png"), QToolButton::InstantPopup);
	connect(action35, SIGNAL(triggered()), this, SLOT(on_sketcher_rectangle()));//矩形
	QAction* action36 = add->addAction("正多边形", QIcon(":/image/resource/16.png"), QToolButton::InstantPopup);
	connect(action36, SIGNAL(triggered()), this, SLOT(on_sketcher_regpolygon()));//正多边形
	QAction* action37 = add->addAction("键槽", QIcon(":/image/resource/17.png"), QToolButton::InstantPopup);
	connect(action37, SIGNAL(triggered()), this, SLOT(on_sketcher_slot()));//键槽
	QAction* action38 = add->addAction("多段线", QIcon(":/image/resource/18.png"), QToolButton::InstantPopup);
	connect(action38, SIGNAL(triggered()), this, SLOT(on_sketcher_polyline()));//多段线
	QAction* action39 = add->addAction("圆角", QIcon(":/image/resource/19.png"), QToolButton::InstantPopup);
	connect(action39, SIGNAL(triggered()), this, SLOT(on_sketcher_fillet()));//圆角
	QAction* action40 = add->addAction("剪裁", QIcon(":/image/resource/20.png"), QToolButton::InstantPopup);
	connect(action40, SIGNAL(triggered()), this, SLOT(on_sketcher_triming()));//裁剪



	//******************//
	//添加在线控制**菜单**//
	//******************//
	SARibbonCategory* online = bar->addCategoryPage("在线仿真");
	SARibbonPannel* online_adjust = online->addPannel("在线修正");
	//设置菜单栏名称
	SARibbonPannel* online_setup = online->addPannel("在线设置");
	QAction* action56 = online_setup->addAction("加工进度同步", QIcon(":/image/resource/icons8-cloud-sync-32.png"), QToolButton::InstantPopup);
		connect(action56, SIGNAL(triggered()), this, SLOT(on_sync()));//加工进度同步
	QAction* action58 = online_adjust->addAction("在线修正", QIcon(":/image/resource/icons8-cloud-sync-32.png"), QToolButton::InstantPopup);
		connect(action58, SIGNAL(triggered()), this, SLOT(on_action()));//在线修正

	//添加
	SARibbonCategory* otherCate = bar->addCategoryPage(tr("Other"));
	SARibbonPannel* pannel2 = otherCate->addPannel(tr("other"));
	QAction* action59 = pannel2->addAction("机器人库", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action59, SIGNAL(triggered()), this, SLOT(on_choose_robot()));//机器人库
	QAction* action60 = pannel2->addAction("设备库", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action60, SIGNAL(triggered()), this, SLOT(on_choosetool()));//工具库
	QAction* action61 = pannel2->addAction("机器人求解",QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action61, SIGNAL(triggered()), this, SLOT(on_kinematics()));//机器人逆解
	QAction* action62 = pannel2->addAction("联动求解求解", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action62, SIGNAL(triggered()), this, SLOT(on_linkage()));//机器人联动求解
	QAction* action63 = pannel2->addAction("输出动画到本地", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action63, SIGNAL(triggered()), this, SLOT(on_animation()));//输出视频到本地
	QAction* action64 = pannel2->addAction("输出视频", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(action64, SIGNAL(triggered()), this, SLOT(on_video()));//输出动画
	QAction* openRobxFileIOManagerAction = pannel2->addAction("Robx文件管理器", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
		connect(openRobxFileIOManagerAction, SIGNAL(triggered()), this,SLOT(on_robxFileIOManager_open()));//Robx文件管理器
	resize(800, 600);

	InitPQKit();
}
	

MainWindow::~MainWindow()
{
}

VARIANT StringToVariant(const std::string& str)
{
	VARIANT var;
	VariantInit(&var); // 初始化 VARIANT

	// 将 std::string 转换为宽字符字符串
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	if (len == 0)
	{
		var.vt = VT_EMPTY; // 转换失败返回空 VARIANT
		return var;
	}

	// 分配宽字符缓冲区
	BSTR bstr = SysAllocStringLen(nullptr, len - 1);
	if (bstr == nullptr)
	{
		var.vt = VT_EMPTY; // 内存分配失败返回空 VARIANT
		return var;
	}

	// 填充 BSTR 数据
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, bstr, len);

	// 设置 VARIANT 类型为 BSTR
	var.vt = VT_BSTR;
	var.bstrVal = bstr;

	return var;
}

BSTR QStringToBSTR(const QString& str)
{
	// 使用 QString::utf16() 提供的 UTF-16 数据创建 BSTR
	return SysAllocString(reinterpret_cast<const OLECHAR*>(str.utf16()));
}


void MainWindow::OnOpenRobx()
{
	
	//提示保存当前文件
	CComBSTR currentRobxName = " ";
	m_ptrKit->Doc_get_name(&currentRobxName);
	if (currentRobxName != L"设计")
	{
		//打开了robx文件，需要提示保存
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "提示", "当前文件未保存，是否保存？",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if(reply == QMessageBox::Yes)
		{
			OnSaveRobx();
			m_ptrKit->pq_CloseDocument(currentRobxName);
			RobxFileIO::uploadJson(RobxFileIO::GlobalPath());
			RobxFileIO::clearFolder(L"temp");
		}
		else if (reply == QMessageBox::Cancel)
		{
			return;
		}
		else if(reply == QMessageBox::No)
		{
			m_ptrKit->pq_CloseDocument(currentRobxName);
			RobxFileIO::clearFolder(L"temp");
		}
	}

	QString fileName = QFileDialog::getOpenFileName(this, tr("open robx file"), "",
		tr("Robx files(*.robx)"));
	QString qstr = fileName;
	qstr.replace("\\", "/");
	std::wstring wpath = qstr.toStdWString();
	RobxFileIO::setPath(wpath);
	if (fileName.isEmpty())
	{
		return;
	}
	fileName = QDir::toNativeSeparators(fileName);
	std::wstring wsFilePath = fileName.toStdWString();
	long long lResult = 0;
	CComVariant varPara(wsFilePath.c_str());
	CComBSTR bsPara = "";
	CComBSTR bsCmd = "RO_CMD_FILE_OPEN";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsPara, varPara, &lResult);
	RobxFileIO::downloadJson(RobxFileIO::GlobalPath());



	//#文件打开时，将json文件中的修正数据注入到模型中
	RobxIO io;
	QVector<Correction> myCorList;
	io.updateData(myCorList, "correctionList.json");
	m_correctionModel = new CorrectionModel(this);
	m_correctionModel->setCorrections(myCorList);

}

void MainWindow::OnSaveRobx()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_FILE_SAVE";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
	//保存数据到json文件
	QVector<Correction> corList = m_correctionModel->corrections();
	RobxIO io;
	io.writeData(corList, "correctionList.json");
	RobxFileIO::updateRobxData(RobxFileIO::GlobalPath());
}

void MainWindow::OnSaveasRobx()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_FILE_SAVE_AS";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_close_robx()
{
	CComBSTR i_bsDocName = "";
	m_ptrKit->pq_CloseDocument(i_bsDocName);
}

void MainWindow::on_kinematics()
{
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> robotMap = getObjectsByType(robotType);
	ULONG RobotID = robotMap.firstKey();

	int nCount = 0;
	double* dLinks = nullptr;
	m_ptrKit->Doc_get_obj_links(RobotID, &nCount, &dLinks);
}

void MainWindow::on_linkage() 
{
	
}

void MainWindow::on_input()
{
	wchar_t whMoudle[256] = L"RO_CMD_IMPORT_ACCESSORY_PART";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_proxy()
{
	wchar_t whMoudle[256] = L"RO_CMD_ARCBALL_PROXY";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_measurement()
{
	wchar_t whMoudle[256] = L"RO_CMD_MEASUREMENT";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_create_frame()
{
	wchar_t whMoudle[256] = L"RO_CMD_CREATE_FRAME";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_create_path()
{
	//wchar_t whMoudle[256] = L"RO_CMD_GENERATE_PATH";
	//m_ptrKit->Doc_start_module((LPWSTR)whMoudle);

	wchar_t whMoudle[] = _T("RO_CMD_GENERATE_PATH");
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::OnCompile()
{
	long long lResult = 0;
	CComBSTR bsPara = "";
	CComBSTR bsCmd = "RO_CMD_COMPILE";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsPara, CComVariant(), &lResult);
}

void MainWindow::on_simulate()
{
	wchar_t whMoudle[256] = L"RO_CMD_SIMULATE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_post()
{
	wchar_t whMoudle[256] = L"RO_CMD_POST";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}



void MainWindow::on_numPost() {
	// 创建后处理对话框
	postProcessing postDialog;

	// 获取机器人列表
	PQDataType robotType = PQ_ROBOT; // 机器人类型代码
	VARIANT robotNamesVariant;
	VariantInit(&robotNamesVariant);
	robotNamesVariant.parray = NULL;

	VARIANT robotIDsVariant;
	VariantInit(&robotIDsVariant);
	robotIDsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Doc_get_obj_bytype(robotType, &robotNamesVariant, &robotIDsVariant);
	if (FAILED(hr)) {
		QMessageBox::warning(this, QString::fromStdString("错误"), QString::fromStdString("获取机器人列表失败！"));
		VariantClear(&robotNamesVariant);
		VariantClear(&robotIDsVariant);
		return;
	}

	// 检查机器人数据有效性
	SAFEARRAY* nameArray = robotNamesVariant.parray;
	SAFEARRAY* idArray = robotIDsVariant.parray;

	if (!nameArray || nameArray->cDims != 1 ||
		!idArray || idArray->cDims != 1 ||
		nameArray->rgsabound[0].cElements == 0) {
		QMessageBox::information(this, QString::fromStdString("提示"), QString::fromStdString("当前没有可用的机器人！"));
		VariantClear(&robotNamesVariant);
		VariantClear(&robotIDsVariant);
		return;
	}

	// 获取机器人数量
	long lowerBound, upperBound;
	SafeArrayGetLBound(nameArray, 1, &lowerBound);
	SafeArrayGetUBound(nameArray, 1, &upperBound);
	long robotCount = upperBound - lowerBound + 1;

	// 提取机器人数据
	BSTR* nameData = NULL;
	LONG* idData = NULL;

	HRESULT hrName = SafeArrayAccessData(nameArray, (void**)&nameData);
	HRESULT hrId = SafeArrayAccessData(idArray, (void**)&idData);

	if (SUCCEEDED(hrName) && SUCCEEDED(hrId)) {
		// 遍历所有机器人，添加为父节点
		for (long i = 0; i < robotCount; i++) {
			QString robotName;
			if (nameData[i] != NULL) {
				robotName = QString::fromWCharArray(nameData[i]);
			}
			else {
				robotName = QString::fromStdString("机器人_%1").arg(i + 1);
			}

			long robotId = idData[i];

			// 设置机器人名称
			postDialog.setRobotName(robotName);

			// 添加机器人作为父节点
			int parentNodeId = postDialog.addParentNode(robotName, robotId);

			// 获取当前机器人的路径组
			VARIANT pathGroupVariant;
			VariantInit(&pathGroupVariant);
			pathGroupVariant.parray = NULL;

			hr = m_ptrKit->Doc_get_pathgroup_name(robotId, &pathGroupVariant);
			if (SUCCEEDED(hr)) {
				SAFEARRAY* pathGroupArray = pathGroupVariant.parray;
				if (pathGroupArray && pathGroupArray->cDims == 1) {
					ULONG pathGroupCount = pathGroupArray->rgsabound[0].cElements;

					// 遍历路径组，添加为子节点
					for (ULONG pgIndex = 0; pgIndex < pathGroupCount; pgIndex++) {

						BSTR pathGroupId = 0;
						SafeArrayGetElement(pathGroupArray, (LONG*)&pgIndex, &pathGroupId);

						// 获取路径组名称
						QString pathGroupName = QString::fromWCharArray(pathGroupId);

						// 设置路径组名称
						postDialog.setPathGroupName(pathGroupName);

						// 生成唯一的子节点ID（使用机器人ID和路径组索引组合）
						int childNodeId = robotId * 1000 + pgIndex;

						// 生成该路径组的后置文件内容
						QString postContent = generatePathGroupPostContent(robotId, pathGroupId, robotName, pathGroupName);

						// 添加路径组作为子节点
						postDialog.addChildNode(parentNodeId, pathGroupName, childNodeId, postContent);

						// 设置group站位信息
						QString positionName = "POS" + pathGroupName;
						int positionId = robotId * 10000 + pgIndex * 100 + 1;

						// 生成站位信息的内容
						//String positionContent = generatePositionContent(robotId, pathGroupId, positionName);

						// 添加子子节点（站位信息）
						postDialog.addSubChildNode(childNodeId, positionName, positionId,"");
					}
				}
				VariantClear(&pathGroupVariant);
			}
		}

		// 释放数组访问
		SafeArrayUnaccessData(nameArray);
		SafeArrayUnaccessData(idArray);
	}
	else {
		// 清理资源
		if (SUCCEEDED(hrName)) SafeArrayUnaccessData(nameArray);
		if (SUCCEEDED(hrId)) SafeArrayUnaccessData(idArray);

		QMessageBox::warning(this, QString::fromStdString("错误"), QString::fromStdString("无法读取机器人数据！"));
		VariantClear(&robotNamesVariant);
		VariantClear(&robotIDsVariant);
		return;
	}

	// 清理VARIANT资源
	VariantClear(&robotNamesVariant);
	VariantClear(&robotIDsVariant);

	// 显示后处理对话框
	postDialog.exec();
}


std::vector<double> MainWindow::getAxisVector(const std::vector<std::vector<double>>& axis, const QString& name)
{
	if (axis.size() < 3) {
		return {};
	}

	// 检查是否包含"负"字
	bool isNegative = name.contains("负", Qt::CaseInsensitive);

	// 获取原始向量
	std::vector<double> result;

	if (name.contains("X", Qt::CaseInsensitive)) {
		result = axis[0]; // 获取第一行方向向量
	}
	else if (name.contains("Y", Qt::CaseInsensitive)) {
		result = axis[1]; // 获取第二行方向向量
	}
	else if (name.contains("Z", Qt::CaseInsensitive)) {
		result = axis[2]; // 获取第三行方向向量
	}
	else {
		// 如果没有匹配的坐标轴，返回空向量
		return {};
	}

	// 如果结果为空，直接返回
	if (result.empty()) {
		return {};
	}

	// 如果包含"负"字，将向量反向
	if (isNegative) {
		for (auto& component : result) {
			component = -component; // 每个分量取负
		}
	}

	return result;
}

QString MainWindow::generatePathGroupPostContent(long robotId, BSTR pathGroupId, const QString& robotName, const QString& pathGroupName) {
	QString content;
	QTextStream stream(&content);

	// 获取外部轴信息
	ULONG uExternalID = 0;
	QString railname = robotName + "_rail";
	GetObjIDByName(PQ_ROBOT, railname.toStdWString(), uExternalID);
	int railCount = 0;
	m_ptrKit->Doc_get_obj_joint_count(uExternalID, &railCount);
	int robotCount = 0;
	m_ptrKit->Doc_get_obj_joint_count(robotId, &robotCount);
	// 创建轨迹文件生成器
	QString outputFilename = QString("%1_%2.txt").arg(robotName).arg(pathGroupName);
	TrajectoryFileGenerator generator(outputFilename.toStdString(), robotName.toStdString());

	// 设置机器人关节数
	generator.setRobotJointsCount(robotCount);

	if (uExternalID != 0) {
		generator.setExternalAxesCount(railCount);
	}

	int globalPointIndex = 1;
	int totalPoints = 0;

	// 获取当前路径组中的路径
	VARIANT pathNamesVariant, pathIDsVariant;
	VariantInit(&pathNamesVariant);
	VariantInit(&pathIDsVariant);
	pathNamesVariant.parray = NULL;
	pathIDsVariant.parray = NULL;

	HRESULT hr = m_ptrKit->Path_get_group_path(robotId, pathGroupId, &pathNamesVariant, &pathIDsVariant);
	if (SUCCEEDED(hr)) {
		SAFEARRAY* pathIDArray = pathIDsVariant.parray;
		if (pathIDArray && pathIDArray->cDims == 1) {
			ULONG pathCount = pathIDArray->rgsabound[0].cElements;
			//stream << "包含路径数量: %1\n").arg(pathCount);

			// 提取路径ID数据
			LONG* pathIdData = NULL;
			HRESULT hrPath = SafeArrayAccessData(pathIDArray, (void**)&pathIdData);

			if (SUCCEEDED(hrPath)) {
				// 遍历所有路径
				for (ULONG pathIndex = 0; pathIndex < pathCount; pathIndex++) {
					ULONG pathId = pathIdData[pathIndex];
					//stream << "\n路径 %1 (ID: %2):\n").arg(pathIndex + 1).arg(pathId);

					// 获取当前路径中的点
					VARIANT pointIDsVariant;
					VariantInit(&pointIDsVariant);
					pointIDsVariant.parray = NULL;

					hr = m_ptrKit->PQAPIGetPointsID(pathId, &pointIDsVariant);
					if (SUCCEEDED(hr)) {
						SAFEARRAY* pointIDArray = pointIDsVariant.parray;
						if (pointIDArray && pointIDArray->cDims == 1) {
							ULONG pointCount = pointIDArray->rgsabound[0].cElements;
							totalPoints += pointCount;
							//stream << "  点数: %1\n").arg(pointCount);

							// 提取点ID数据
							LONG* pointIdData = NULL;
							HRESULT hrPoint = SafeArrayAccessData(pointIDArray, (void**)&pointIdData);

							if (SUCCEEDED(hrPoint)) {
								// 遍历所有点
								for (ULONG ptIndex = 0; ptIndex < pointCount; ptIndex++) {
									ULONG pointId = pointIdData[ptIndex];

									// 获取机器人关节数据
									VARIANT jointDataVariant;
									VariantInit(&jointDataVariant);
									hr = m_ptrKit->PQAPIGetRobotJointsFromPoints(pointId, &jointDataVariant);

									// 获取外部轴关节数据
									VARIANT externalJointsVariant;
									VariantInit(&externalJointsVariant);
									if (uExternalID != 0) {
										m_ptrKit->PQAPIGetExternalJointsFromPoints(pointId, uExternalID, &externalJointsVariant);
									}

									// 获取点信息
									PQPostureType nType = QUATERNION;
									int nPostureCount = 0;
									double* dPointPosture = nullptr;
									double dVelocity = 0.0;
									double dSpeedPercent = 0.0;
									PQPointInstruction iInstruct = PQ_LINE;
									int iApproach = 0;

									m_ptrKit->PQAPIGetPointInfo(pointId, nType, &nPostureCount, &dPointPosture, &dVelocity, &dSpeedPercent, &iInstruct, &iApproach);

									// 创建轨迹点信息
									TrajectoryPointInfo pointInfo;
									pointInfo.pointIndex = globalPointIndex++;

									// 设置运动类型
									if (ptIndex == 0) {
										pointInfo.motionType = "MOVJ";
										pointInfo.motionPercentage = static_cast<int>(dSpeedPercent);
									}
									else {
										pointInfo.motionType = "MOVD";
									}

									pointInfo.velocity = dVelocity;

									// 解析机器人关节数据
									if (SUCCEEDED(hr) && jointDataVariant.parray != NULL) {
										SAFEARRAY* jointArray = jointDataVariant.parray;
										if (jointArray->cDims == 1) {
											double* jointData = NULL;
											SafeArrayAccessData(jointArray, (void**)&jointData);

											ULONG jointCount = jointArray->rgsabound[0].cElements;
											for (ULONG j = 0; j < jointCount; j++) {
												pointInfo.robotJoints.push_back(jointData[j]);
											}

											SafeArrayUnaccessData(jointArray);
										}
									}

									// 解析外部轴关节数据
									if (uExternalID != 0 && externalJointsVariant.parray != NULL) {
										SAFEARRAY* externalArray = externalJointsVariant.parray;
										if (externalArray->cDims == 1) {
											double* externalData = NULL;
											SafeArrayAccessData(externalArray, (void**)&externalData);

											ULONG externalCount = externalArray->rgsabound[0].cElements;
											for (ULONG e = 0; e < externalCount; e++) {
												pointInfo.externalAxes.push_back(externalData[e]);
											}

											SafeArrayUnaccessData(externalArray);
										}
									}

									// 添加轨迹点到生成器
									generator.addTrajectoryPoint(pointInfo);
									
									// 清理点相关资源
									VariantClear(&jointDataVariant);
									VariantClear(&externalJointsVariant);
								}
								SafeArrayUnaccessData(pointIDArray);
							}
						}
						VariantClear(&pointIDsVariant);
					}
				}
				SafeArrayUnaccessData(pathIDArray);
			}
		}
		VariantClear(&pathNamesVariant);
		VariantClear(&pathIDsVariant);
	}
	else {
		stream << "错误: 无法获取路径信息\n";
	}

	// 自动计算NPOS参数
	generator.calculateAndSetNposParams();

	try {
		// 生成轨迹文件内容
		std::string trajectoryContent = generator.generateToString();
		stream << QString::fromStdString(trajectoryContent);
	}
	catch (const std::exception& e) {
		stream << "\n错误: 生成轨迹文件失败: %1\n";
	}

	return content;
}

void MainWindow::CreateBoundingBox()
{
	double start[6] = { 0.0 };
	double dEnd[6] = { 100,100,100,200,200,200 };
	double dRGB[3] = { 255, 0, 0 };
	ULONG i_uCoordinateID = 0;
	ULONG o_uCylinderID = 0;
	m_ptrKit->Doc_draw_cylinder(start, 6, dEnd, 6, 16,
		dRGB, 3, i_uCoordinateID, &o_uCylinderID,false);
}

void MainWindow::updateRailOptions(const QString & robotName, const QMap<ULONG, QString>& robotMap)
{
	if (!curseDialog || robotName.isEmpty()) {
		return;
	}
	// 获取轨道信息
	ULONG uExternalID = 0;
	QString railname = robotName + "_rail";
	GetObjIDByName(PQ_ROBOT, railname.toStdWString(), uExternalID);

	int railCount = 0;
	HRESULT hr = m_ptrKit->Doc_get_obj_joint_count(uExternalID, &railCount);

	QStringList rail;
	if (SUCCEEDED(hr) && railCount > 0) {
		for (int i = 0; i < railCount; i++) {
			QString railName = "J" + QString::number(i + 1);
			rail.append(railName);
		}
	}
	else {
		// 如果轨道数量为0或调用失败，确保rail为空列表
		rail.clear();
	}

	// 更新对话框中的轨道选项
	curseDialog->setRailOptions(rail);
}

void MainWindow::on_defining_institutions()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_DEFINE_MECHANISM";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_import_institutions()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("open robr file"), "",
		tr("Robx files(*.robr)"));
	if (fileName.isEmpty())
	{
		return;
	}
	LONGLONG* lResult = 0;
	CComBSTR bsParam = QStringToBSTR(fileName);
	CComBSTR bsCmd = "RO_CMD_DEFROBOT_SETUP";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_defining_tool()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_CREATE_TOOL";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_import_tool()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("open robt file"), "",
		tr("Robx files(*.robt)"));
	if (fileName.isEmpty())
	{
		return;
	}
	LONGLONG* lResult = 0;
	CComBSTR bsParam = QStringToBSTR(fileName);
	CComBSTR bsCmd = "RO_CMD_IMPORT_TOOL";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_defining_part()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_CREATE_WORKINGPART";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_import_part()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("open tool file"), "",
		tr("tool files(*.robp *.robs *.robm);; STEP file(*.step *.stp)"));
	if (fileName.isEmpty())
	{
		return;
	}
	LONGLONG* lResult = 0;
	CComBSTR bsParam = QStringToBSTR(fileName);
	CComBSTR bsCmd = "RO_CMD_IMPORT_PART";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
}

void MainWindow::on_create_newsketch()
{
	wchar_t whMoudle[256] = L"RO_CMD_CREATE_NEWSKETCH";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_edit_wkptsketch()
{
	wchar_t whMoudle[256] = L"RO_CMD_ROEDIT_WKPTSKETCH";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_createpoint()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_CREATEPOINT";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_line()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_LINE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_arc()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_ARC_1";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_circ()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_CIRC_1";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_ellipticalarc()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_ELLIPTICALARC";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_ellipse()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_ELLIPSE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_rectangle()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_RECTANGLE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_regpolygon()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_REGPOLYGON";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_slot()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_SLOT";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}


void MainWindow::on_sketcher_polyline()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_POLYLINE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_fillet()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_FILLET";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sketcher_triming()
{
	wchar_t whMoudle[256] = L"RO_CMD_SKETCHER_TRIMING";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_choose_robot()
{
	wchar_t whMoudle[256] = L"RO_CMD_CHOOSE_ROBOT";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_floor()
{
	wchar_t whMoudle[256] = L"RO_CMD_FLOOR";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_motion_option()
{
	wchar_t whMoudle[256] = L"RO_CMD_MOTION_OPTION";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_choosetool()
{
	wchar_t whMoudle[256] = L"RO_CMD_CHOOSETOOL";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_campath_flat_surface()
{
	//wchar_t whMoudle[256] = L"RO_CMD_CAMPATH_FLAT_SURFACE";//平行加工
	wchar_t whMoudle[256] = L"RO_CMD_CAMPATH_SPRAY_SURFACE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_roobjassistor_manage()
{
	wchar_t whMoudle[256] = L"RO_CMD_ROOBJASSISTORMANAGE";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_rocreate_sprayingtool()
{
	wchar_t whMoudle[256] = L"RO_CMD_ROCREATESPRAYINGTOOL";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_collision_option()
{
	wchar_t whMoudle[256] = L"RO_CMD_COLLISION_OPTION";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_sync()
{
	wchar_t whMoudle[256] = L"RO_CMD_EDITOR_SYNC_SHELL";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_animation()
{
	wchar_t whMoudle[256] = L"RO_CMD_EXPORT_ANIMATION_LOCAL";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_video()
{
	wchar_t whMoudle[256] = L"RO_CMD_EXPORT_VIDEO";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_insert_path()
{
	InsertPathDlg dlg;
	if (QDialog::Rejected == dlg.exec())
	{
		return;
	}

	//默认取第一个机器人
	ULONG uRobotID = 0;
	GetObjIDByName(PQ_ROBOT, _T(""), uRobotID);

	//
	int nType = 1;
	double dPosition[6] = { 0.0 };
	dlg.GetPosture(dPosition[0], dPosition[1], dPosition[2], dPosition[3], dPosition[4], dPosition[5], nType);

	PQPostureType eType = (PQPostureType)nType;
	PQPointInstruction nInstruction[1] = { PQ_LINE };
	double dVel[1] = { 200.0 };
	double dSpeedP[1] = { 100.0 };
	int nApproach[1] = { -1 };
	wchar_t whPathName[] = _T("Test_Path");
	wchar_t whPathGroupName[] = _T("Test_Group");

	ULONG uPathID = 0;
	m_ptrKit->Path_insert_from_point(uRobotID, 1, dPosition, eType, nInstruction, dVel, dSpeedP, nApproach, whPathName, whPathGroupName, 0, FALSE, &uPathID, TRUE);
}

void MainWindow::on_kinetic_analysis()
{
	//kinetic_analysis dlg;
	//kinetic_analysis dlg;
	//dlg.setModal(true);
	//dlg.exec();

	long lCount = 0;
	ulong key = 0;
	GetObjIDByName(PQ_WORKINGPART, L"零件2", key);
	m_ptrKit->PQAPIGetWorkPartVertexCount(key, &lCount);//获取顶点个数
	//std::vector<double> dSrc(3 * lCount, 0);
	//double* dSrcPosition = dSrc.data();//顶点位置（通过一维数组表示）
	double dSrcPosition[69] = {};
	BSTR sName;
	m_ptrKit->Doc_get_obj_name(key, &sName);

	

	m_ptrKit->PQAPIGetWorkPartVertex(key, 0, lCount, dSrcPosition);//获取顶点位置

	for (int i = 0; i < 69; i++) {
		m_vPosition.push_back(dSrcPosition[i]);
	}

	CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
	HRESULT hr = m_ptrKit->Doc_start_module(cmd);

}

void MainWindow::on_define_space()
{
	//if (spaceDialog) {
	//	spaceDialog->close();
	//	spaceDialog->deleteLater();
	//	spaceDialog = nullptr;
	//}

	spaceDialog = new robotSpaceDefine(this,m_ptrKit,m_ptrKitCallback);

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		delete spaceDialog;
		spaceDialog = nullptr;
		return;
	}

	// 将机器人名称设置到对话框中
	for (const QString& name : robotNames) {
		spaceDialog->setRobotOptions(name);
	}

	////根据文件名称读取配置文件
	////待完成

	//实现计算θ和xyz的关系
	spacePoint center(0, 0, 0);
	spacePoint size(0, 0, 0);

	//Workspace spaceModel(center, size, m_ptrKit, m_ptrKitCallback);
	//spacePoint centerPoint = spaceModel.calculateRobotWorkspaceCenter(robotMap.firstKey());


	QString name = robotNames[0];

	spaceDialog->setModal(false);
	spaceDialog->setAttribute(Qt::WA_DeleteOnClose);
	spaceDialog->show();
}

void MainWindow::on_curse_part()
{
	if (curseDialog) {
		curseDialog->close();
		curseDialog->deleteLater();
		curseDialog = nullptr;
	}

	curseDialog = new cursePart(this);
	isPickupActive = false; // 重置拾取状态
	isPreview = false;//是否进行预览

	// 使用封装好的函数获取机器人列表
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> robotMap = getObjectsByType(robotType);

	// 使用封装函数获取喷涂机器人名称列表
	QStringList robotNames = getSprayRobotNames(PQ_MECHANISM_ROBOT, robotMap);

	if (robotNames.isEmpty()) {
		QMessageBox::information(this, "提示", "当前没有可用的喷涂机器人！");
		delete curseDialog;
		curseDialog = nullptr;
		return;
	}

	// 将机器人名称设置到对话框中
	for (const QString& name : robotNames) {
		curseDialog->setRobotOptions(name);
	}


	// 连接机器人选择改变信号
	connect(curseDialog, &cursePart::robotSelectionChanged, this, [this, robotMap](const QString& robotName) {
		updateRailOptions(robotName, robotMap);
	});

	// 初始设置轨道信息
	QString currentRobot = robotNames.isEmpty() ? "" : robotNames.first();
	updateRailOptions(currentRobot, robotMap);


	// 对于曲面选取
	curseDialog->setModal(true);
	curseDialog->setWindowModality(Qt::ApplicationModal);

	// 连接拾取按钮信号 - 启动拾取模块
	connect(curseDialog, &cursePart::pickUpSignal, this, [this]() {
		if (!isPickupActive && !isPreview) {
			// 启动拾取模块
			CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
			HRESULT hr = m_ptrKit->Doc_start_module(cmd);
			if (SUCCEEDED(hr)) {
				isPickupActive = true;
				isPreview = false; // 确保预览模式关闭
				curseDialog->setModal(false);
				curseDialog->setWindowModality(Qt::NonModal);
				qDebug() << "曲面拾取模块已启动，请在3D窗口中点击元素";
			}
			else {
				QMessageBox::warning(this, "错误", "启动曲面拾取模块失败！");
			}
		}
		else {
			QString mode = isPreview ? "预览模式" : "曲面拾取模式";
			qDebug() << mode << "已在运行中";
		}
	});

	// 连接关闭拾取按钮信号
	connect(curseDialog, &cursePart::closeSignal, this, [this]() {
		if (isPickupActive || isPreview) {
			isPickupActive = false;
			isPreview = false;
			curseDialog->setModal(true);
			CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
			HRESULT hr = m_ptrKit->Doc_end_module(cmd);
			curseDialog->setWindowModality(Qt::ApplicationModal);
			qDebug() << "拾取模块已停止";
		}
	});

	// 删除信号
	connect(curseDialog, &cursePart::deleteSelectedSurfaces, this, &MainWindow::onDeleteSelectedSurfaces);

	// 计算选中曲面的最小包围盒
	connect(curseDialog, &cursePart::calculateAABB, this, [this]() {
		for (const auto& pair : pickupMap) {
			unsigned long key = pair.first;
			const std::vector<std::wstring>& values = pair.second;
			long lCount = 0;
			m_ptrKit->PQAPIGetWorkPartVertexCount(key, &lCount);//获取顶点个数
			std::vector<double> dSrc(3 * lCount, 0);
			double* dSrcPosition = dSrc.data();//顶点位置（通过一维数组表示）
			BSTR sName;
			m_ptrKit->Doc_get_obj_name(key, &sName);
			m_ptrKit->PQAPIGetWorkPartVertex(key, 0, lCount, dSrcPosition);//获取顶点位置
			for (const auto& value : values) {
				for (long i = 0; i < lCount; i++) {
					double dPosition[3] = { dSrcPosition[3 * i],dSrcPosition[3 * i + 1],dSrcPosition[3 * i + 2] };
					double dTol = 10;
					LONG bPointOnSurface = 0;
					std::vector<wchar_t> buffer(value.begin(), value.end());
					buffer.push_back(L'\0'); // 添加终止符
					LPWSTR name = buffer.data();
					m_ptrKit->Part_cheak_point_on_surface(name, dPosition, dTol,&bPointOnSurface);//检查顶点是否在曲面上

					if (bPointOnSurface) {
						m_vPosition.push_back(dPosition[0]);
						m_vPosition.push_back(dPosition[1]);
						m_vPosition.push_back(dPosition[2]);
					}
				}
			}
		}
		box.minPoint = { 0,0,0 };
		box.maxPoint = { 0,0,0 };

		std::vector<Point3D> curse;
		for (int i = 0; i < m_vPosition.size(); i += 3) {
			Point3D p;
			p.x = m_vPosition[i];
			p.y = m_vPosition[i + 1];
			p.z = m_vPosition[i + 2];
			curse.push_back(p);
		}

		box = calculateAABB(curse);

		std::vector<Point3D> box_8 = box.getCorners();

		for (int i = 0; i < 8; i++) {
			ABBPosition.push_back(box_8[i].x);
			ABBPosition.push_back(box_8[i].y);
			ABBPosition.push_back(box_8[i].z);
		}
	});

	// 设置坐标系
	PQDataType CoodernateType = PQ_COORD;
	QMap<ULONG, QString> CoodernateMap = getObjectsByType(CoodernateType);

	// 创建一个新的QMap，先插入"世界坐标系"，再插入原有的数据
	QMap<ULONG, QString> newCoodernateMap;
	newCoodernateMap.insert(0, "世界坐标系");  // 先插入首位

	// 将原有数据插入到后面（键值从1开始）
	for (auto it = CoodernateMap.begin(); it != CoodernateMap.end(); ++it) {
		newCoodernateMap.insert(it.key(), it.value());
	}

	CoodernateMap = newCoodernateMap;  // 替换原来的map
	QStringList CoodernateNames = CoodernateMap.values();
	curseDialog->setCoodernateOptions(CoodernateNames);

	//创建点阵
	connect(curseDialog, &cursePart::spaceSetting, this, [this, CoodernateMap]() {

		//获取设置好的坐标轴

		QString coordanateName = curseDialog->getCoodernateSelection();
		ULONG selectCoorID = CoodernateMap.key(coordanateName);
		

		//记录坐标系的位置和姿态  采用欧拉角XYZ表示
		double coordanate[6];
		if (selectCoorID == 0) {
			for (int i = 0; i < 6; i++) {
				coordanate[i] = 0;
			}
		}
		else {

			int nCount = 0;
			double* dPosture = nullptr;
			m_ptrKit->Doc_get_coordinate_posture(selectCoorID, EULERANGLEXYZ, &nCount, &dPosture, 0);

			for (int i = 0; i < 6; i++) {
				coordanate[i] = dPosture[i];
			}

			m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
		}


		std::vector<std::vector<double>> axisVector;
		axisVector = getCoordinateAxesFromEuler(coordanate);

		//根据选择的主法矢选择坐标轴方向向量
		QString mainVectorText = curseDialog->getComboBox_4();
		std::vector<double> mainVector = getAxisVector(axisVector,mainVectorText);
		
		//根据选择的主划分方向
		QString mainDivisionDirectionText = curseDialog->getComboBox_5();
		std::vector<double> mainDivisionDirection = getAxisVector(axisVector, mainDivisionDirectionText);

		// 2. 生成点阵参数                
		double spacing = curseDialog->geteditSelection().toDouble();
		
		Point3D viewDirection(mainVector[0], mainVector[1], mainVector[2]); // 从Y轴方向观看
		std::vector<Point3D> corners = box.getCorners();
		// 自动选择最近的面并生成点阵
		std::vector<Point3D> result = createGridOnClosestSurface(corners,spacing, spacing, viewDirection);
		double* dIntersetionpoint = nullptr;

		double maxtheta = 0;
		for (const auto& key : pickupMap) {
			unsigned long k = key.first;
			for (const auto value : key.second) {
				CComBSTR whSurfaceName = value.c_str();
				double* dIntersetionpoint = nullptr;
				int nArrsize = 1;
				for (Point3D P : result) {
					double dPosition[3] = { P.x,P.y,P.z };
					m_ptrKit->Part_get_ray_surface_intersetion(whSurfaceName, dPosition, mainVector.data(),
						&dIntersetionpoint, &nArrsize);
						Eigen::Vector3d v1(mainVector[0], mainVector[1], mainVector[2]);
						Eigen::Vector3d v2(dIntersetionpoint[3], dIntersetionpoint[4], dIntersetionpoint[5]);
						Eigen::Vector3d v1_norm = v1.normalized();
						Eigen::Vector3d v2_norm = v2.normalized();

						// 计算点积并限制范围
						double dot = v1_norm.dot(v2_norm);
						dot = std::max(-1.0, std::min(1.0, dot));

						maxtheta = std::max(std::acos(dot), maxtheta);	
				}
				m_ptrKit->PQAPIFree((LONG_PTR*)dIntersetionpoint);
			}
		}
		maxtheta = maxtheta * 180 / M_PI;
		curseDialog->setTextBrowser2(QString("%1").arg(maxtheta) + "°");
		
	});

	connect(curseDialog, &cursePart::calculateSpace, this, [this]() {
		

		Point3D direction(0, 1, 0);
		curseDialog->setTextEdit("500");
		curseDialog->setTextEdit2("500");
		int length = curseDialog->getTextEdit().toDouble();
		int width = curseDialog->getTextEdit2().toDouble();
		auto grid = createGridOnClosestSurface(box, length, width, direction);

		for (auto p : grid) {
			points.push_back(p.x);
			points.push_back(p.y);
			points.push_back(p.z);
		}

		QString value1 = QString("%1").arg(points[0]);
		QString value2 = QString("%1").arg(points[1]);
		QString value3 = QString("%1").arg(points[2]);

		curseDialog->setTextEditValues(value1, value2, value3);
	});

	connect(curseDialog, &cursePart::previewSignal, this, [this]() {
		// 启动拾取模块
		CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
		HRESULT hr = m_ptrKit->Doc_start_module(cmd);
		if (SUCCEEDED(hr)) {
			isPoint = true;
		}

	});

	connect(curseDialog, &cursePart::areaPosition, this, [this]() {
		std::vector<double> areaPosition;
		areaPosition = curseDialog->getVertexValues();
		std::vector<double> difference;
		for (int i = 0; i < areaPosition.size(); i++) {
			double diff = areaPosition[i] - points[i];
			difference.push_back(diff);
		}

		for (int i = 0; i < points.size(); i+=3) {
			points[i] = points[i] + difference[0];
			points[i+1] = points[i+1] + difference[1];
			points[i+2] = points[i+2] + difference[2];
		}
	});

	// 对话框关闭时清理资源
	connect(curseDialog, &cursePart::finished, this, [this](int result) {
		Q_UNUSED(result)
			// 停止任何正在运行的模块
			if (isPickupActive || isPreview) {
				CComBSTR cmd = "RO_CMD_PICKUP_ELEMENT";
				m_ptrKit->Doc_end_module(cmd);
			}
		isPickupActive = false;
		isPreview = false;
		curseDialog = nullptr;
	});

	// 设置为非模态对话框
	curseDialog->setModal(false);
	curseDialog->setAttribute(Qt::WA_DeleteOnClose);
	curseDialog->show();

}

//处理删除曲面的槽函数
void MainWindow::onDeleteSelectedSurfaces(const QStringList& surfaceNames)
{
	if (surfaceNames.isEmpty()) {
		qDebug() << "没有需要删除的曲面";
		return;
	}

	// 从 pickupMap 中删除指定的曲面
	int deletedCount = 0;

	for (const QString& surfaceName : surfaceNames) {
		// 将 QString 转换为 std::wstring
		std::wstring wSurfaceName = surfaceName.toStdWString();

		// 遍历 pickupMap 查找并删除对应的曲面
		auto it = pickupMap.begin();
		while (it != pickupMap.end()) {
			auto& surfaces = it->second;
			auto surfaceIt = std::find(surfaces.begin(), surfaces.end(), wSurfaceName);

			if (surfaceIt != surfaces.end()) {
				// 找到曲面，从向量中删除
				surfaces.erase(surfaceIt);
				deletedCount++;
				qDebug() << "已从 pickupMap 中删除曲面:" << surfaceName;

				// 如果该键对应的向量为空，可以选择删除整个键值对
				if (surfaces.empty()) {
					it = pickupMap.erase(it);
				}
				else {
					++it;
				}

				// 假设曲面名称在 pickupMap 中唯一，找到后跳出内层循环
				break;
			}
			else {
				++it;
			}
		}
	}

	//qDebug() << "总共删除了" << deletedCount << "个曲面";

}

std::vector<double> MainWindow::calculateAABBCornersFromPickupMap(const std::map<unsigned long,
	std::vector<std::wstring>>& pickupMap)
{
	// 将 ABBPosition 替换为 surfacePoints
	std::vector<double> surfacePoints;
	std::vector<double> resultPositions;

	// 遍历pickupMap中的每个部件
	for (const auto& pair : pickupMap) {
		unsigned long key = pair.first;
		const std::vector<std::wstring>& values = pair.second;

		// 获取部件顶点数量
		long lCount = 0;
		m_ptrKit->PQAPIGetWorkPartVertexCount(key, &lCount);

		if (lCount <= 0) {
			continue;
		}

		// 获取部件顶点坐标
		std::vector<double> dSrc(3 * lCount, 0);
		double* dSrcPosition = dSrc.data();
		m_ptrKit->PQAPIGetWorkPartVertex(key, 0, lCount, dSrcPosition);

		// 检查每个表面上的点
		for (const auto& value : values) {
			for (long i = 0; i < lCount; i++) {
				double dPosition[3] = {
					dSrcPosition[3 * i],
					dSrcPosition[3 * i + 1],
					dSrcPosition[3 * i + 2]
				};

				double dTol = 10;
				LONG bPointOnSurface = 0;

				// 转换表面名称格式
				std::vector<wchar_t> buffer(value.begin(), value.end());
				buffer.push_back(L'\0');
				LPWSTR name = buffer.data();

				// 检查点是否在表面上
				m_ptrKit->Part_cheak_point_on_surface(name, dPosition, dTol, &bPointOnSurface);

				if (bPointOnSurface) {
					ABBPosition.push_back(dPosition[0]);
					ABBPosition.push_back(dPosition[1]);
					ABBPosition.push_back(dPosition[2]);
				}
			}
		}
	}

	// 计算AABB包围盒
	AABB box;
	box.minPoint = { 0, 0, 0 };
	box.maxPoint = { 0, 0, 0 };

	// 将点转换为Point3D格式
	std::vector<Point3D> cursePoints;
	for (size_t i = 0; i < surfacePoints.size(); i += 3) { 
		if (i + 2 < surfacePoints.size()) { 
			Point3D p;
			p.x = surfacePoints[i]; 
			p.y = surfacePoints[i + 1]; 
			p.z = surfacePoints[i + 2]; 
			cursePoints.push_back(p);
		}
	}

	// 计算AABB并获取8个角点
	if (!cursePoints.empty()) {
		box = calculateAABB(cursePoints);
		std::vector<Point3D> boxCorners = box.getCorners();

		// 将角点坐标展平为连续数组
		for (const auto& corner : boxCorners) {
			resultPositions.push_back(corner.x);
			resultPositions.push_back(corner.y);
			resultPositions.push_back(corner.z);
		}
	}
	return resultPositions;
}


void MainWindow::on_pos_cal()
{
	posCal dlg;
	dlg.setModal(true);
	dlg.exec();
}

void MainWindow::on_AGV_path()
{
	AGVpath dlg;
	dlg.setModal(true);
	dlg.exec();
}

void MainWindow::on_trajCorrectdock_open()
{
	CComBSTR robxName = " ";
	m_ptrKit->Doc_get_name(&robxName);
	if (robxName == L"设计") {
		//代表当前没有打开的robx文件，不能打开轨迹修正面板
		QMessageBox::information(this, "提示", "请先打开robx文件！");
		return;
	}
	TrajCorrectDock* dock = new TrajCorrectDock(m_ptrKit, m_ptrKitCallback, m_correctionModel);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
}


void MainWindow::on_bendingManagerWidget_open()
{
	BendingManagerWidget* widget = new BendingManagerWidget(m_ptrKit, m_ptrKitCallback, m_correctionModel,this);
	widget->show();
}

void MainWindow::on_PositionCorrectWidget_open()
{
	PositionCorrectWidget* widget = new PositionCorrectWidget(m_ptrKit, m_ptrKitCallback);
	widget->show();
}

void MainWindow::on_robxFileIOManager_open()
{
	if (!m_robxIOMgr)
	{
		m_robxIOMgr = new RobxFileIOManagerWidget();

	}
	m_robxIOMgr->show();
	m_robxIOMgr->raise();
	m_robxIOMgr->activateWindow();
}

void MainWindow::on_effectiveness_analysis()
{
	effectiveness_analysis dlg;
	dlg.setModal(true);
	dlg.exec();
}

void MainWindow::on_AlignPart3Point()
{
	wchar_t whMoudle[256] = L"RO_CMD_AlignPart3Point";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_AlignPartPointAxis()
{
	wchar_t whMoudle[256] = L"RO_CMD_AlignPartPointAxis";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_path_partion()
{
	path_partion path_partion;
	path_partion.setModal(true);
	path_partion.exec();
}

void MainWindow::on_path_edit()
{
	path_edit path_edit;
	path_edit.setModal(true);
	path_edit.exec();
}

void MainWindow::on_path_optimize()
{
	path_optimize path_optimize;
	path_optimize.setModal(true);
	path_optimize.exec();
}

void MainWindow::on_path_set_up()
{
	path_set_up path_set_up;
	path_set_up.setModal(true);
	path_set_up.exec();
}

void MainWindow::on_path_sort()
{
	path_sort path_sort;
	path_sort.setModal(true);
	path_sort.exec();
}

void MainWindow::on_addlinearspraypath()
{
	wchar_t whMoudle[256] = L"RO_CMD_ADDLINEARSPRAYPATH";
	m_ptrKit->Doc_start_module((LPWSTR)whMoudle);
}

void MainWindow::on_deformation_correction()
{
	deformation_correction deformation_correction;
	deformation_correction.setModal(true);
	deformation_correction.exec();
}

void MainWindow::on_action()
{
	mydialog mDialog;
	mDialog.setModal(true);
	mDialog.exec();
}

// 根据类型获取对象列表，返回ID（long）和名称的映射
QMap<ULONG, QString> MainWindow::getObjectsByType(PQDataType objType)
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

	//qDebug() << "成功获取类型" << objType << "的对象列表，共" << objectMap.size() << "个对象";
	return objectMap;
}

QStringList MainWindow::getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap)
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

// 提取long类型数组的函数
QList<long> MainWindow::extractLongArrayFromVariant(const VARIANT& variant)
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
QStringList MainWindow::extractStringArrayFromVariant(const VARIANT& variant)
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

void MainWindow::InitPQKit()
{
	//
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	HRESULT hr = m_ptrKit.CoCreateInstance(__uuidof(PQPlatformComponent));
	if (S_OK != hr)
	{
		QString strError = "PQKit创建失败！\n请排查当前启动或调试exe同目录是否有PQKit环境。\nPQkit环境包含PQkit文件夹、PQKit.manifest文件、启动或调试exe的manifest文件。";
		QMessageBox::information(NULL, "PQKit", strError, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}

	//
	m_ptrKitCallback = new CPQKitCallback(this);
	connect(m_ptrKitCallback, &CPQKitCallback::signalInitializeResult, this, &MainWindow::OnInitializeResult);
	connect(m_ptrKitCallback, &CPQKitCallback::signalRunCMDResult, this, &MainWindow::OnRunCMDResult);
	connect(m_ptrKitCallback, &CPQKitCallback::signalGetDataResult, this, &MainWindow::OnGetDataResult);
	connect(m_ptrKitCallback, &CPQKitCallback::signalRaiseDockwindow, this, &MainWindow::OnRaiseDockwindow);
	connect(m_ptrKitCallback, &CPQKitCallback::signalLoginResult, this, &MainWindow::OnLoginResult);
	connect(m_ptrKitCallback, &CPQKitCallback::signalPathGenerateResult, this, &MainWindow::OnPathGenerateResult);
	connect(m_ptrKitCallback, &CPQKitCallback::signalElementPickup, this, &MainWindow::OnElementPickup);
	connect(m_ptrKitCallback, &CPQKitCallback::signalLButtonUp, this, &MainWindow::OnLButtonUp);
	connect(m_ptrKitCallback, &CPQKitCallback::signalRButtonUp, this, &MainWindow::OnRButtonUp);
	connect(m_ptrKitCallback, &CPQKitCallback::signalMenuPop, this, &MainWindow::OnMenuPop);
	connect(m_ptrKitCallback, &CPQKitCallback::signalDraw, this, &MainWindow::OnDraw);

	//
	m_ptrKitInitThread = new PQKitInitThread;
	connect(m_ptrKitInitThread, &PQKitInitThread::signalInitializeKit, this, &MainWindow::OnInitializeKitThread);
	m_ptrKitInitThread->start();
}

void MainWindow::OnInitializeKitThread()
{
	//initialize pqkit
	CComBSTR bsName = L"ra_tsinghua_whk01";
	CComBSTR bsPWD = L"tsinghua_whk01";
	HRESULT hr = m_ptrKit->pq_InitPlatformComponent(m_ptrKitCallback, (int)(this->winId()), bsName, bsPWD);
	if (S_OK != hr)
	{
		QString strError = "PQKit初始化失败！\n请参考Fire_Initialize_Result或Fire_Login_Result函数中的具体lResult值。";
		QMessageBox::information(NULL, "PQKit", strError, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
}

void MainWindow::ShowPQKitWindown()
{
	int nHWND = 0;
	m_ptrKit->pq_GetPlatformView(&nHWND);
	HWND hWnd = (HWND)(UINT_PTR)nHWND;
	if (nullptr == hWnd)
	{
		return;
	}
	m_pPQPlatformView = QWidget::createWindowContainer(QWindow::fromWinId((WId)hWnd), this);

	ULONG_PTR nModelDoc = NULL;
	m_ptrKit->pq_GetModelTreeView(&nModelDoc);
	HWND hModelWnd = (HWND)nModelDoc;
	if (nullptr != hModelWnd)
	{
		m_pPQModeTreeView = QWidget::createWindowContainer(QWindow::fromWinId((WId)hModelWnd), this);
	}

	ULONG_PTR nDebugDoc = NULL;
	m_ptrKit->pq_GetDebugTreeView(&nDebugDoc);
	HWND hDebugWnd = (HWND)nDebugDoc;
	if (nullptr != hDebugWnd)
	{
		m_pPQDebugView = QWidget::createWindowContainer(QWindow::fromWinId((WId)hDebugWnd), this);
	}

	auto splitter = new QSplitter;
	splitter->addWidget(m_pPQModeTreeView);
	splitter->addWidget(m_pPQPlatformView);
	splitter->addWidget(m_pPQDebugView);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 5);
	splitter->setStretchFactor(2, 1);
	setCentralWidget(splitter);
}

void MainWindow::closeEvent(QCloseEvent* event)   //清除原版，副本更名
{

	CComBSTR robxName = " ";
	m_ptrKit->Doc_get_name(&robxName);
	if (m_ptrKit)
	{

		m_ptrKit->pq_CloseComponent();
	}
	//无打开的robx文档，直接关闭窗口
	if (robxName == L"设计") {
		event->accept();
	}
	else
	{
		m_ptrKit->pq_CloseDocument(robxName);
		QVector<Correction> myCorrections = m_correctionModel->getCorrectionCopy();
		RobxIO io;
		io.writeData(myCorrections, "correctionList.json");
		RobxFileIO::uploadJson(RobxFileIO::GlobalPath());
	}
}

void MainWindow::GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID)
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

void MainWindow::OnInitializeResult(long lResult)
{
	if (lResult > 0)
	{
		//show kit
		ShowPQKitWindown();
	}
	else
	{
		QString strError = "组件初始化失败:\n";
		switch (lResult)
		{
		case -1:
			strError.append("未知错误");
			break;
		case -2:
			strError.append("初始化超时");
			break;
		case -3:
		case -4:
		case -6:
			strError.append("加载组件动态库错误");
			break;
		case -7:
			strError.append("组件重复初始化");
			break;
		case -9:
		case -10:
		case -11:
		case -12:
			strError.append("内部数据错误");
			break;
		case -14:
			strError.append("非开发版账号不能登录开发版");
			break;
		default:
			strError.append("错误码: ");
			strError.append(QString::number(lResult, 10));
			break;
		}

		QMessageBox::information(NULL, "PQKit Info", strError, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
}

std::vector<std::vector<double>> MainWindow::getCoordinateAxesFromEuler(double * eulerAngles)
{
	double alpha = eulerAngles[3];
	double beta = eulerAngles[4];
	double gamma = eulerAngles[5];

	// 计算三角函数值
	double cosA = cos(alpha), sinA = sin(alpha);
	double cosB = cos(beta), sinB = sin(beta);
	double cosG = cos(gamma), sinG = sin(gamma);

	// 计算旋转矩阵的元素（XYZ顺序）
	// 旋转矩阵 R = Rx * Ry * Rz
	double r11 = cosB * cosG;
	double r12 = cosG * sinA * sinB - cosA * sinG;
	double r13 = cosA * cosG * sinB + sinA * sinG;

	double r21 = cosB * sinG;
	double r22 = cosA * cosG + sinA * sinB * sinG;
	double r23 = cosA * sinB * sinG - cosG * sinA;

	double r31 = -sinB;
	double r32 = cosB * sinA;
	double r33 = cosA * cosB;

	// 创建结果二维数组
	std::vector<std::vector<double>> result(3, std::vector<double>(3));

	// 旋转矩阵的列向量就是坐标轴方向向量
	// 第一列是X轴方向
	result[0][0] = r11; // X轴的X分量
	result[0][1] = r21; // X轴的Y分量
	result[0][2] = r31; // X轴的Z分量

	// 第二列是Y轴方向
	result[1][0] = r12; // Y轴的X分量
	result[1][1] = r22; // Y轴的Y分量
	result[1][2] = r32; // Y轴的Z分量

	// 第三列是Z轴方向
	result[2][0] = r13; // Z轴的X分量
	result[2][1] = r23; // Z轴的Y分量
	result[2][2] = r33; // Z轴的Z分量

	return result;
}

void MainWindow::OnRunCMDResult(long lResult)
{

}

void MainWindow::OnGetDataResult(long lResult)
{

}

void MainWindow::OnRaiseDockwindow(int i_nType)
{

}

void MainWindow::OnLoginResult(int i_nLoginType)
{
	if (i_nLoginType < 0)
	{
		QString strError = "用户校验失败:\n";
		switch (i_nLoginType)
		{
		case -1:
			strError.append("密码错误");
			break;
		case -2:
			strError.append("用户不存在");
			break;
		case -4:
			strError.append("用户已过期");
			break;
		case -6:
			strError.append("网络连接失败");
			break;
		case -7:
			strError.append("用户未激活");
			break;
		case -14:
			strError.append("账号与产品不匹配");
			break;
		default:
			strError.append("错误码: ");
			strError.append(QString::number(i_nLoginType));
			break;
		}

		QMessageBox::information(NULL, "PQKit Info", strError, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

	}
}

void MainWindow::OnPathGenerateResult(long i_bSuccess, int i_nPathCount, int i_nIndex, unsigned long i_ulPathID)
{

}

void MainWindow::OnElementPickup(ULONG i_ulObjID, LPWSTR i_lEntityID, int i_nEntityType,
	double i_dPointX, double i_dPointY, double i_dPointZ)
{

	//// 然后处理拾取结果（如果处于拾取模式且有活动的对话框）
	if (isPickupActive && curseDialog && curseDialog->isVisible()) {
		// 可以添加更多条件检查，比如只处理特定类型的元素
		if (i_nEntityType == 2) { // 例如，只处理类型1的元素
			QString entityName = QString::fromWCharArray(i_lEntityID);
			curseDialog->addItemToListView(entityName);
			qDebug() << "拾取到元素:" << entityName;
			//记录
			pickupMap[i_ulObjID].push_back(i_lEntityID ? i_lEntityID : L"");
		}
	}

}

void MainWindow::OnRButtonUp(long i_lPosX, long i_lPosY)
{
	
}

void MainWindow::OnLButtonUp(long i_lPosX, long i_lPosY)
{

}

void MainWindow::OnMenuPop(unsigned long i_ulObjID, long i_lPosX, long i_lPosY, int* o_nHandled1)
{
	
}

void MainWindow::OnDraw()
{
	/*if (isPreview && curseDialog && curseDialog->isVisible()) {
		m_vPosition.clear();
		std::vector<Point3D> curse;
		for (int i = 0; i < ABBPosition.size(); i += 3) {
			Point3D p;
			p.x = ABBPosition[i];
			p.y = ABBPosition[i + 1];
			p.z = ABBPosition[i + 2];
			curse.push_back(p);
		}
		AABB box;
		box = calculateAABB(curse);

		std::vector<Point3D> box_8 = box.getCorners();

		for (int i = 0; i < 8; i++) {
			m_vPosition.push_back(box_8[i].x);
			m_vPosition.push_back(box_8[i].y);
			m_vPosition.push_back(box_8[i].z);
		}

	}*/


	CComBSTR strText = "point";
	double dPos[3] = { 0.0 };
	int counter = 0;
	for (size_t i = 0; i < m_vPosition.size(); i++)
	{
		dPos[counter++] = m_vPosition[i];
		if ((counter % 3) == 0)
		{
			m_ptrKit->View_draw_point(dPos, 0, 3, RGB(10, 100, 200), strText, RGB(20, 200, 20));
			counter = 0;
		}

	}

	////绘制工作空间
	//std::map<int, std::array<double, 3>> pointMap;
	//std::array<double, 3> tempPoint;

	//for (size_t i = 0; i < m_vPosition.size(); i++)
	//{
	//	tempPoint[i % 3] = m_vPosition[i];

	//	if ((i % 3) == 2 && i >= 2) // 每凑齐3个点
	//	{
	//		int pointIndex = i / 3;
	//		pointMap.emplace(pointIndex, tempPoint);
	//	}
	//}


	//// 定义边的连接关系：pair<起点索引, 终点索引>
	//std::vector<std::pair<int, int>> edgeDefinitions = {
	//	{0, 1}, {1, 2}, {2, 3}, {3, 0}, // 前面的四条边
	//	{4, 5}, {5, 6}, {6, 7}, {7, 4}, // 后面的四条边
	//	{0, 4}, {1, 5}, {2, 6}, {3, 7}  // 连接前后的四条边
	//};

	//// 您的数组，大小设为36，足以容纳12条边（每条边3个分量）
	//double start[36] = { 0.0 };
	//double dEnd[36] = { 0.0 };

	//// 填充数组
	//int arrayIndex = 0;
	//for (const auto& edge : edgeDefinitions) {
	//	int startIndex = edge.first;
	//	int endIndex = edge.second;

	//	// 从pointMap中获取起点和终点的坐标数组
	//	const std::array<double, 3>& startPoint = pointMap.at(startIndex);
	//	const std::array<double, 3>& endPoint = pointMap.at(endIndex);

	//	// 将每个点的三个坐标分量(x, y, z)分别存入数组
	//	for (int coord = 0; coord < 3; ++coord) {
	//		start[arrayIndex] = startPoint[coord];   // 起点的坐标分量
	//		dEnd[arrayIndex] = endPoint[coord];     // 终点的坐标分量
	//		arrayIndex++;
	//	}
	//}


	//double dRGB[3] = { 255, 0, 0 };
	//ULONG i_uCoordinateID = 0;
	//ULONG o_uCylinderID = 0;

	//for (int i = 0; i < 36; i += 3) {
	//	double start_new[3] = { start[i],start[i + 1],start[i + 2] };
	//	double end_new[3] = { dEnd[i],dEnd[i + 1],dEnd[i + 2] };
	//	m_ptrKit->Doc_draw_cylinder(start_new, 3, end_new, 3, 16,
	//		dRGB, 3, i_uCoordinateID, &o_uCylinderID, false);
	//}
	
	if (isPoint && curseDialog) {
		//CComBSTR strText = "result";
		//double dPos[3] = { 0.0 };
		//int counter = 0;
		//for (size_t i = 0; i < points.size(); i++)
		//{
		//	dPos[counter++] = points[i];
		//	if ((counter % 3) == 0)
		//	{
		//		m_ptrKit->View_draw_point(dPos, 0, 3, RGB(10, 100, 200), strText, RGB(20, 200, 20));
		//		counter = 0;
		//	}
		//}

		std::map<int, std::array<double, 3>> pointMap;
		std::array<double, 3> tempPoint;

		for (size_t i = 0; i < points.size(); i++)
		{
			tempPoint[i % 3] = points[i];

			if ((i % 3) == 2 && i >= 2)
			{
				int pointIndex = i / 3; // 顶点索引是连续的
				pointMap.emplace(pointIndex, tempPoint);
			}
		}

		// 假设每个长方形由4个顶点构成，按顺序排列
		int rectanglesCount = pointMap.size() / 4; // 计算能组成的长方形数量

		// 定义单个长方体的边连接关系（本地偏移）
		std::vector<std::pair<int, int>> singleRectangleEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 0} // 单个长方形的四条边
		};

		// 为所有长方体生成边定义
		std::vector<std::pair<int, int>> edgeDefinitions;
		for (int rectIndex = 0; rectIndex < rectanglesCount; ++rectIndex) {
			int vertexOffset = rectIndex * 4; // 每个长方形占用4个顶点
			for (const auto& edge : singleRectangleEdges) {
				// 将本地顶点索引转换为全局顶点索引
				int globalStart = vertexOffset + edge.first;
				int globalEnd = vertexOffset + edge.second;
				edgeDefinitions.push_back({ globalStart, globalEnd });
			}
		}

		// 计算需要的数组大小：边的数量 * 每个点3个坐标分量 * 2（起点和终点）
		int totalEdges = edgeDefinitions.size();
		double* start = new double[totalEdges * 3]; // 起点坐标数组
		double* dEnd = new double[totalEdges * 3];  // 终点坐标数组

		// 填充数组
		int arrayIndex = 0;
		for (const auto& edge : edgeDefinitions) {
			int startIndex = edge.first;
			int endIndex = edge.second;

			if (pointMap.find(startIndex) != pointMap.end() &&
				pointMap.find(endIndex) != pointMap.end()) {
				const std::array<double, 3>& startPoint = pointMap.at(startIndex);
				const std::array<double, 3>& endPoint = pointMap.at(endIndex);

				for (int coord = 0; coord < 3; ++coord) {
					start[arrayIndex] = startPoint[coord];
					dEnd[arrayIndex] = endPoint[coord];
					arrayIndex++;
				}
			}
		}

		double dRGB[3] = { 255, 0, 0 };
		ULONG i_uCoordinateID = 0;
		ULONG o_uCylinderID = 0;

		// 绘制所有边
		for (int i = 0; i < totalEdges * 3; i += 3) {
			double start_new[3] = { start[i], start[i + 1], start[i + 2] };
			double end_new[3] = { dEnd[i], dEnd[i + 1], dEnd[i + 2] };
			m_ptrKit->Doc_draw_cylinder(start_new, 3, end_new, 3, 3,
				dRGB, 3, i_uCoordinateID, &o_uCylinderID, false);
		}

		delete[] start;
		delete[] dEnd;
	}

	
}

