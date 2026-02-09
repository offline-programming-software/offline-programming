#include "MainWindow.h"
#include "SARibbon.h"
#include <QComboBox>
#include <QTimer>
#include <QFileDialog>
#include <QSplitter>
#include <QWindow>
#include <QChart>
#include <string>
#include"robxFileIO.h"
#include "test\RobxFileIOManagerWidget.h"


MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
	//软件图标设置
	setWindowIcon(QIcon(":/image/resource/APP.svg"));

	//添加菜单栏
	SARibbonBar* bar = ribbonBar();

	////设置按钮
	//bar->applicationButton()->setText(QString::fromLocal8Bit ("  文件  "));


	//添加菜单轨迹编程//
	SARibbonCategory* sence = bar->addCategoryPage("  编程仿真  ");

	//添加文件Pannel
	SARibbonPannel*  file = sence->addPannel("文件");
	QAction* action1 = file->addAction("打开", QIcon(":/image/resource/open.png"), QToolButton::InstantPopup);
	QAction* action2 = file->addAction("", QIcon(":/image/resource/save.png"), QToolButton::InstantPopup, SARibbonPannelItem::Small);
	QAction* action3 = file->addAction(tr(""), QIcon(":/image/resource/new_1.png"), QToolButton::InstantPopup, SARibbonPannelItem::Small);
	QAction* action4 = file->addAction(tr(""), QIcon(":/image/resource/icons8-close-32.png"), QToolButton::InstantPopup, SARibbonPannelItem::Small);
	action4->setToolTip("关闭"); // 设置提示信息为 "Open File"

	//添加场景搭建Pannel
	SARibbonPannel*  import = sence->addPannel("场景搭建");
	QAction* action5 = import->addAction("输入", QIcon(":/image/resource/icons8-folder-32.png"), QToolButton::InstantPopup);
	QAction* action6 = import->addAction("三维球", QIcon(":/image/resource/5.png"), QToolButton::InstantPopup);
	QAction* action7 = import->addAction("测量", QIcon(":/image/resource/icons8-Measure.png"), QToolButton::InstantPopup);
	QAction* action8 = import->addAction("新建坐标系", QIcon(":/image/resource/icons8-coordinate-system-32.png"), QToolButton::InstantPopup);

	//添加轨迹规划Pannel
	SARibbonPannel*  path = sence->addPannel("轨迹规划");
	QAction* action68 = path->addAction("工作空间定义", QIcon(":/image/resource/robot_space.svg"), QToolButton::InstantPopup);
	QAction* action69 = path->addAction("区域划分", QIcon(":/image/resource/33.png"), QToolButton::InstantPopup);
	QAction* action9 = path->addAction("生成轨迹", QIcon(":/image/resource/24.png"), QToolButton::InstantPopup);
	QAction* action10 = path->addAction("随型喷涂轨迹", QIcon(":/image/resource/30.png"), QToolButton::InstantPopup);
	QAction* action11 = path->addAction("线性喷涂轨迹", QIcon(":/image/resource/29.png"), QToolButton::InstantPopup);
	QAction* action12 = path->addAction("路径导入", QIcon(":/image/resource/open.png"), QToolButton::InstantPopup);
	QAction* action70 = path->addAction("站位计算", QIcon(":/image/resource/100.png"), QToolButton::InstantPopup);

	//添加站位计算

	//添加AGV
	SARibbonPannel*  AGV = sence->addPannel("路径优化");
	QAction* action65 = AGV->addAction("AGV路径优化", QIcon(":/image/resource/icons8-agv-32.png"), QToolButton::InstantPopup);


	//添加轨迹修正
	SARibbonPannel* correction = sence->addPannel("轨迹修正");
	QAction* action101 = correction->addAction("弯曲变形配置", QIcon(":/image/resource/bendingFunc.png"), QToolButton::InstantPopup);
	QAction* action102 = correction->addAction("弯曲变形管理", QIcon(":/image/resource/bendingmanager .png"), QToolButton::InstantPopup);
	QAction* action103 = correction->addAction("对象位置修正", QIcon(":/image/resource/positioncorrect.png"), QToolButton::InstantPopup);

	//添加仿真
	SARibbonPannel*  complie = sence->addPannel("仿真调试");
	QAction* action13 = complie->addAction("编译", QIcon(":/image/resource/compile.png"), QToolButton::InstantPopup);
	QAction* action14 = complie->addAction("仿真", QIcon(":/image/resource/simulate.png"), QToolButton::InstantPopup);
	QAction* action15 = complie->addAction("后置", QIcon(":/image/resource/26.png"), QToolButton::InstantPopup);
	QAction* action100 = complie->addAction("批量后置", QIcon(":/image/resource/26.png"), QToolButton::InstantPopup);


	//添加设置
	SARibbonPannel*  set = sence->addPannel("设置");
	QAction* action16 = set->addAction("地面", QIcon(":/image/resource/floor.png"), QToolButton::InstantPopup);
	QAction* action17 = set->addAction("选项", QIcon(":/image/resource/icons8-automatic-100.png"), QToolButton::InstantPopup);
	QAction* action18 = set->addAction("碰撞设置", QIcon(":/image/resource/28.png"), QToolButton::InstantPopup);

	//添加定义机构
	SARibbonCategory* definition = bar->addCategoryPage("定义机构");

	//设置菜单栏名称
	SARibbonPannel* pannel = definition->addPannel("机构");
	//添加菜单栏
	QAction* action19 = pannel->addAction("定义机构", QIcon(":/image/resource/1.png"), QToolButton::InstantPopup);
	QAction* action20 = pannel->addAction("导入机构", QIcon(":/image/resource/2.png"), QToolButton::InstantPopup);


	//设置菜单栏名称
	SARibbonPannel* part = definition->addPannel("零件");
	//添加菜单栏
	QAction* action21 = part->addAction("定义零件", QIcon(":/image/resource/icons8-tool.png"), QToolButton::InstantPopup);
	QAction* action22 = part->addAction("导入零件", QIcon(":/image/resource/icons-gear.png"), QToolButton::InstantPopup);
	QAction* action47 = part->addAction("定义AGV", QIcon(":/image/resource/icons8-agv-32.png"), QToolButton::InstantPopup);

	//设置菜单栏名称
	SARibbonPannel* tool = definition->addPannel("工具");
	//添加菜单栏
	QAction* action23 = tool->addAction("定义工具", QIcon(":/image/resource/icons8-detool.png"), QToolButton::InstantPopup);
	QAction* action24 = tool->addAction("导入工具", QIcon(":/image/resource/icons8-imtool.png"), QToolButton::InstantPopup);

	//添加喷涂工具
	SARibbonPannel* spy = definition->addPannel("喷涂工具");
	QAction* action25 = spy->addAction("定义喷涂工具", QIcon(":/image/resource/pentu.png"), QToolButton::InstantPopup);
	QAction* action26 = spy ->addAction("喷刷管理", QIcon(":/image/resource/icons8-spray.png"), QToolButton::InstantPopup);

	//添加菜单定义//
	SARibbonCategory* sketch = bar->addCategoryPage("自由绘制");

	//设置菜单栏名称
	SARibbonPannel* create = sketch->addPannel("创建草图");
	//添加菜单栏
	QAction* action27 = create->addAction("创建草图", QIcon(":/image/resource/9.png"), QToolButton::InstantPopup);
	QAction* action28 = create->addAction("编辑草图", QIcon(":/image/resource/icons8-create-32.png"), QToolButton::InstantPopup);
	//创建基本图元
	SARibbonPannel* add = sketch->addPannel("创建基本图元");
	QAction* action29 = add->addAction("点", QIcon(":/image/resource/10.png"), QToolButton::InstantPopup);
	QAction* action30 = add->addAction("直线", QIcon(":/image/resource/11.png"), QToolButton::InstantPopup);
	QAction* action31 = add->addAction("圆", QIcon(":/image/resource/12.png"), QToolButton::InstantPopup);
	QAction* action32 = add->addAction("圆弧", QIcon(":/image/resource/13.png"), QToolButton::InstantPopup);
	QAction* action33 = add->addAction("椭圆弧", QIcon(":/image/resource/14.png"), QToolButton::InstantPopup);
	QAction* action34 = add->addAction("椭圆", QIcon(":/image/resource/21.png"), QToolButton::InstantPopup);
	QAction* action35 = add->addAction("矩形", QIcon(":/image/resource/15.png"), QToolButton::InstantPopup);
	QAction* action36 = add->addAction("正多边形", QIcon(":/image/resource/16.png"), QToolButton::InstantPopup);
	QAction* action37 = add->addAction("键槽", QIcon(":/image/resource/17.png"), QToolButton::InstantPopup);
	QAction* action38 = add->addAction("多段线", QIcon(":/image/resource/18.png"), QToolButton::InstantPopup);
	QAction* action39 = add->addAction("圆角", QIcon(":/image/resource/19.png"), QToolButton::InstantPopup);
	QAction* action40 = add->addAction("剪裁", QIcon(":/image/resource/20.png"), QToolButton::InstantPopup);


	//添加菜单校准
	SARibbonCategory* optimize = bar->addCategoryPage("路径优化");

	SARibbonPannel* simulation = optimize->addPannel("仿真设置");
	QAction* action41 = simulation->addAction("仿真", QIcon(":/image/resource/simulate.png"), QToolButton::InstantPopup);
	QAction* action42 = simulation->addAction("动力学分析", QIcon(":/image/resource/33.png"), QToolButton::InstantPopup);
	QAction* action43 = simulation->addAction("效能分析", QIcon(":/image/resource/32.png"), QToolButton::InstantPopup);

	SARibbonPannel* path1 = optimize->addPannel("路径优化");
	QAction* action44 = path1->addAction("路径分块", QIcon(":/image/resource/31.png"), QToolButton::InstantPopup);
	QAction* action45 = path1->addAction("路径编辑", QIcon(":/image/resource/new_1.png"), QToolButton::InstantPopup);
	QAction* action46 = path1->addAction("路径设置", QIcon(":/image/resource/icons8-settings-32.png"), QToolButton::InstantPopup);
	
	QAction* action48 = path1->addAction("移动路径优化", QIcon(":/image/resource/34.png"), QToolButton::InstantPopup);

	//添加菜单校准
	SARibbonCategory* calibration = bar->addCategoryPage("校准");

	//设置菜单栏名称
	SARibbonPannel* adjust = calibration->addPannel("校准");
	QAction* action49 = adjust->addAction("三维球", QIcon(":/image/resource/5.png"), QToolButton::InstantPopup);
	QAction* action50 = adjust->addAction("建立坐标系", QIcon(":/image/resource/7.png"), QToolButton::InstantPopup);
	QAction* action51 = adjust->addAction("三点校准", QIcon(":/image/resource/6.png"), QToolButton::InstantPopup);
	QAction* action52 = adjust->addAction("点轴校准", QIcon(":/image/resource/8.png"), QToolButton::InstantPopup);
	QAction* action53 = adjust->addAction("自动校准对象位置", QIcon(":/image/resource/icons8-approval-32.png"), QToolButton::InstantPopup);
	QAction* action54 = adjust->addAction("自动校准AGV位置", QIcon(":/image/resource/icons8-agv-32.png"), QToolButton::InstantPopup);
	QAction* action55 = adjust->addAction("校准实测曲面位置", QIcon(":/image/resource/27.png"), QToolButton::InstantPopup);

	//添加菜单校准
	SARibbonCategory* online = bar->addCategoryPage("在线仿真");

	//设置菜单栏名称
	SARibbonPannel* online_setup = online->addPannel("在线设置");
	QAction* action56 = online_setup->addAction("加工进度同步", QIcon(":/image/resource/icons8-cloud-sync-32.png"), QToolButton::InstantPopup);
	QAction* action57 = online_setup->addAction("变形修正定义", QIcon(":/image/resource/icons8-metamorphose-32.png"), QToolButton::InstantPopup);

	SARibbonPannel* online_adjust = online->addPannel("在线修正");
	QAction* action58 = online_adjust->addAction("在线修正", QIcon(":/image/resource/icons8-cloud-sync-32.png"), QToolButton::InstantPopup);

	//添加
	SARibbonCategory* otherCate = bar->addCategoryPage(tr("Other"));
	SARibbonPannel* pannel2 = otherCate->addPannel(tr("other"));
	QAction* action59 = pannel2->addAction("机器人库", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* action60 = pannel2->addAction("设备库", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* action61 = pannel2->addAction("机器人求解",QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* action62 = pannel2->addAction("联动求解求解", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* action63 = pannel2->addAction("输出动画到本地", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* action64 = pannel2->addAction("输出视频", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	QAction* openRobxFileIOManagerAction = pannel2->addAction("Robx文件管理器", QIcon(":/image/icon/action.svg"), QToolButton::InstantPopup);
	resize(800, 600);

	connect(action1, SIGNAL(triggered()), this, SLOT(OnOpenRobx()));//打开
	connect(action2, SIGNAL(triggered()), this, SLOT(OnSaveRobx()));//保存
	connect(action3, SIGNAL(triggered()), this, SLOT(OnSaveasRobx()));//另存为
	connect(action4, SIGNAL(triggered()), this, SLOT(on_close_robx()));//关闭
	connect(action5, SIGNAL(triggered()), this, SLOT(on_input()));//输入
	connect(action6, SIGNAL(triggered()), this, SLOT(on_proxy()));//三维球
	connect(action7, SIGNAL(triggered()), this, SLOT(on_measurement()));//测量
	connect(action8, SIGNAL(triggered()), this, SLOT(on_create_frame()));//新建坐标系

	connect(action68, SIGNAL(triggered()), this, SLOT(on_define_space()));//机器人工作空间定义
	connect(action69, SIGNAL(triggered()), this, SLOT(on_curse_part()));//喷涂区域划分
	connect(action9, SIGNAL(triggered()), this, SLOT(on_create_path()));//生成轨迹
	connect(action10, SIGNAL(triggered()), this, SLOT(on_campath_flat_surface()));//随性喷涂
	connect(action11, SIGNAL(triggered()), this, SLOT(on_addlinearspraypath()));//线性喷涂
	connect(action12, SIGNAL(triggered()), this, SLOT(on_insert_path()));//导入路径
	connect(action70, SIGNAL(triggered()), this, SLOT(on_pos_cal()));//站位计算

	connect(action65, SIGNAL(triggered()), this, SLOT(on_AGV_path()));//导入路径

	connect(action13, SIGNAL(triggered()), this, SLOT(OnCompile()));//编译
	connect(action14, SIGNAL(triggered()), this, SLOT(on_simulate()));//仿真
	connect(action15, SIGNAL(triggered()), this, SLOT(on_post()));//后置
	connect(action100, SIGNAL(triggered()), this, SLOT(on_numPost()));

	connect(action16, SIGNAL(triggered()), this, SLOT(on_floor()));//地面
	connect(action17, SIGNAL(triggered()), this, SLOT(on_motion_option()));//选项
	connect(action18, SIGNAL(triggered()), this, SLOT(on_collision_option()));//碰撞设置

	connect(action19, SIGNAL(triggered()), this, SLOT(on_defining_institutions()));//定义机构
	connect(action20, SIGNAL(triggered()), this, SLOT(on_import_institutions()));//导入机构
	connect(action21, SIGNAL(triggered()), this, SLOT(on_defining_part()));//定义零件
	connect(action22, SIGNAL(triggered()), this, SLOT(on_import_part()));//导入零件
	connect(action23, SIGNAL(triggered()), this, SLOT(on_defining_tool()));//导入工具
	connect(action24, SIGNAL(triggered()), this, SLOT(on_import_tool()));//导入工具
	connect(action25, SIGNAL(triggered()), this, SLOT(on_rocreate_sprayingtool()));//定义喷涂工具
	connect(action26, SIGNAL(triggered()), this, SLOT(on_roobjassistor_manage()));//喷刷管理

	connect(action27, SIGNAL(triggered()), this, SLOT(on_create_newsketch()));//创建草图
	connect(action28, SIGNAL(triggered()), this, SLOT(on_edit_wkptsketch()));//编辑草图
	connect(action29, SIGNAL(triggered()), this, SLOT(on_sketcher_createpoint()));//点
	connect(action30, SIGNAL(triggered()), this, SLOT(on_sketcher_line()));//直线
	connect(action31, SIGNAL(triggered()), this, SLOT(on_sketcher_circ()));//圆
	connect(action32, SIGNAL(triggered()), this, SLOT(on_sketcher_arc()));//圆弧
	connect(action33, SIGNAL(triggered()), this, SLOT(on_sketcher_ellipticalarc()));//椭圆弧
	connect(action34, SIGNAL(triggered()), this, SLOT(on_sketcher_ellipse()));//椭圆
	connect(action35, SIGNAL(triggered()), this, SLOT(on_sketcher_rectangle()));//矩形
	connect(action36, SIGNAL(triggered()), this, SLOT(on_sketcher_regpolygon()));//正多边形
	connect(action37, SIGNAL(triggered()), this, SLOT(on_sketcher_slot()));//键槽
	connect(action38, SIGNAL(triggered()), this, SLOT(on_sketcher_polyline()));//多段线
	connect(action39, SIGNAL(triggered()), this, SLOT(on_sketcher_fillet()));//圆角
	connect(action40, SIGNAL(triggered()), this, SLOT(on_sketcher_triming()));//裁剪

	connect(action41, SIGNAL(triggered()), this, SLOT(on_simulate()));//仿真
	connect(action42, SIGNAL(triggered()), this, SLOT(on_kinetic_analysis()));//动力学分析
	connect(action43, SIGNAL(triggered()), this, SLOT(on_effectiveness_analysis()));//效能分析

	connect(action44, SIGNAL(triggered()), this, SLOT(on_path_partion()));//路径分块
	connect(action45, SIGNAL(triggered()), this, SLOT(on_path_edit()));//路径编辑
	connect(action46, SIGNAL(triggered()), this, SLOT(on_path_set_up()));//路径设置
	connect(action47, SIGNAL(triggered()), this, SLOT(on_path_sort()));//路径块排序
	connect(action48, SIGNAL(triggered()), this, SLOT(on_path_optimize()));//移动路径优化

	connect(action49, SIGNAL(triggered()), this, SLOT(on_proxy()));//三维球
	connect(action50, SIGNAL(triggered()), this, SLOT(on_create_frame()));//创建坐标系
	connect(action51, SIGNAL(triggered()), this, SLOT(on_AlignPart3Point()));//三点校准
	connect(action52, SIGNAL(triggered()), this, SLOT(on_AlignPartPointAxis()));//点轴校准
	connect(action53, SIGNAL(triggered()), this, SLOT(on_proxy()));//自动校准对象位置
	connect(action54, SIGNAL(triggered()), this, SLOT(on_proxy()));//自动校准AGV位置
	connect(action55, SIGNAL(triggered()), this, SLOT(on_proxy()));//校准实测曲面位置

	connect(action56, SIGNAL(triggered()), this, SLOT(on_sync()));//加工进度同步
	connect(action57, SIGNAL(triggered()), this, SLOT(on_deformation_correction()));//变形修正定义
	connect(action58, SIGNAL(triggered()), this, SLOT(on_action()));//在线修正

	connect(action59, SIGNAL(triggered()), this, SLOT(on_choose_robot()));//机器人库
	connect(action60, SIGNAL(triggered()), this, SLOT(on_choosetool()));//工具库
	connect(action61, SIGNAL(triggered()), this, SLOT(on_kinematics()));//机器人逆解
	connect(action62, SIGNAL(triggered()), this, SLOT(on_linkage()));//机器人联动求解
	connect(action63, SIGNAL(triggered()), this, SLOT(on_animation()));//输出视频到本地
	connect(action64, SIGNAL(triggered()), this, SLOT(on_video()));//输出动画
	connect(action101, SIGNAL(triggered()), this, SLOT(on_trajCorrectdock_open()));//输出动画
	connect(openRobxFileIOManagerAction, SIGNAL(triggered()), this,SLOT(on_robxFileIOManager_open()));//Robx文件管理器

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
}

void MainWindow::OnSaveRobx()
{
	LONGLONG* lResult = 0;
	CComBSTR bsParam = "";
	CComBSTR bsCmd = "RO_CMD_FILE_SAVE";
	m_ptrKit->pq_RunCommand(bsCmd, NULL, NULL, bsParam, CComVariant(), lResult);
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
	PQDataType robotType = PQ_ROBOT;
	QMap<ULONG, QString> institutionMap = getObjectsByType(robotType);

	if (institutionMap.empty()) {
		QMessageBox::information(this, "提示", "当前没有可用的机器人！");
		return;
	}

	// 过滤机械臂机器人
	QMap<ULONG, QString> robotMap;
	for (auto it = institutionMap.begin(); it != institutionMap.end(); ++it) {
		ULONG robotID = it.key();
		PQRobotType eRobotType = PQ_MECHANISM_ROBOT;
		if (SUCCEEDED(m_ptrKit->Robot_get_type(robotID, &eRobotType))){
			if (eRobotType == PQ_MECHANISM_ROBOT) {
				robotMap.insert(robotID, it.value());
			}
		}
	}

	if (robotMap.empty()) {
		QMessageBox::information(this, "提示", "当前没有可用的机械臂机器人！");
			return;
	}

	// 添加所有机器人和路径组
	bool hasValidData = false;
	for (auto it = robotMap.begin(); it != robotMap.end(); ++it) {
		ULONG robotId = it.key();
		QString robotName = it.value();

		// 添加机器人父节点
		int parentNodeId = postDialog.addParentNode(robotName, robotId);
		if (parentNodeId == -1) continue;

		// 获取路径组
		VARIANT pathGroupVariant;
		VariantInit(&pathGroupVariant);

		if (SUCCEEDED(m_ptrKit->Doc_get_pathgroup_name(robotId, &pathGroupVariant))) {
			SAFEARRAY* pathGroupArray = pathGroupVariant.parray;
			if (pathGroupArray && pathGroupArray->cDims == 1) {
				ULONG pathGroupCount = pathGroupArray->rgsabound[0].cElements;

				for (ULONG pgIndex = 0; pgIndex < pathGroupCount; pgIndex++) {
					BSTR pathGroupId = 0;
					if (SUCCEEDED(SafeArrayGetElement(pathGroupArray, (LONG*)&pgIndex, &pathGroupId))) {
						QString pathGroupName = QString::fromWCharArray(pathGroupId);

						int groupNodeId = robotId * 1000 + pgIndex;
						postDialog.addChildNode(parentNodeId, pathGroupName, groupNodeId, "");

						// 生成轨迹内容
						QString postContent = generatePathGroupPostContent(robotId, pathGroupId, robotName, pathGroupName);

						// 添加轨迹节点
						int trajectoryNodeId = robotId * 10000 + pgIndex * 100 + 1;
						QString trajectoryNodeName = QString("轨迹_%1").arg(pathGroupName);
						postDialog.addSubChildNode(groupNodeId, trajectoryNodeName, trajectoryNodeId, postContent);

						// 添加位置节点
						int positionNodeId = robotId * 10000 + pgIndex * 100 + 2;
						QString positionNodeName = QString("位置_%1").arg(pathGroupName);
						postDialog.addSubChildNode(groupNodeId, positionNodeName, positionNodeId, "");

						SysFreeString(pathGroupId);
						hasValidData = true;
					}
				}
			}
			VariantClear(&pathGroupVariant);
		}
	}

	if (!hasValidData) {
		QMessageBox::information(this, "提示", "没有找到有效的路径组数据！");
		return;
	}

	// 显示对话框
	postDialog.exec();
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
	kinetic_analysis *dlg = new kinetic_analysis(this, m_ptrKit, m_ptrKitCallback);

	dlg->setModal(false);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();

}

void MainWindow::on_define_space()
{
	robotSpaceDefine* spaceDialog = new robotSpaceDefine(this,m_ptrKit,m_ptrKitCallback);

	spaceDialog->setModal(false);
	spaceDialog->setAttribute(Qt::WA_DeleteOnClose);
	spaceDialog->show();
}

void MainWindow::on_curse_part()
{
	cursePart* curseDialog = new cursePart(this,m_ptrKit,m_ptrKitCallback);

	// 设置为非模态对话框
	curseDialog->setModal(false);
	curseDialog->setAttribute(Qt::WA_DeleteOnClose);
	curseDialog->show();

}




void MainWindow::on_pos_cal()
{
	posCal *dlg = new posCal(this,m_ptrKit,m_ptrKitCallback);


	dlg->setModal(false);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
}

void MainWindow::on_AGV_path()
{
	AGVpath *dlg = new AGVpath(this, m_ptrKit, m_ptrKitCallback);

	dlg->setModal(false);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
}

void MainWindow::on_trajCorrectdock_open()
{
TrajCorrectDock* dock = new TrajCorrectDock(m_ptrKit, m_ptrKitCallback);
	addDockWidget(Qt::LeftDockWidgetArea, dock);

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
	effectiveness_analysis *dlg = new effectiveness_analysis(this, m_ptrKit, m_ptrKitCallback);
	dlg->setModal(false);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
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
	path_sort  *dlg = new path_sort(this, m_ptrKit, m_ptrKitCallback);

	dlg->setModal(false);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
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

	qDebug() << "成功获取类型" << objType << "的对象列表，共" << objectMap.size() << "个对象";
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
	CComBSTR bsName = L"ra_tsinghua_whk04";
	CComBSTR bsPWD = L"tsinghua_whk04";
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
	if (m_ptrKit)
	{

		m_ptrKit->pq_CloseComponent();
	}

	if (robxName == L"设计") {
		event->accept();
	}
	else
	{
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
	
}

