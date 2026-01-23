#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QStringListModel>
#include <QMessageBox>
#include <qDebug>
#include <QTimer>
#include "ui_cursePart.h"
#include "PQKitCallback.h"

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

QT_BEGIN_NAMESPACE
namespace Ui { class cursePartClass; };
QT_END_NAMESPACE

class cursePart : public QDialog
{
	Q_OBJECT

public:
	cursePart(QWidget *parent = nullptr,
		CComPtr<IPQPlatformComponent> ptrKit = nullptr,
		CPQKitCallback* ptrKitCallback = nullptr);
	~cursePart();

	// 机器人选项设置
	void setRobotOptions(const QString& robotOption);
	void setRobotOptions(const QStringList& robotOptions);
	QString getRobotSelection() const;

	// 坐标系选项设置
	void setCoodernateOptions(const QString& coordinateOption);
	void setCoodernateOptions(const QStringList& coordinateOptions);
	QString getCoodernateSelection() const;

	// 轨道选项设置
	void setRailOptions(const QString& railOption);
	void setRailOptions(const QStringList& railOptions);

	// 列表视图操作
	void addItemToListView(const QString& item);
	void addItemsToListView(const QStringList& items);

	void setTextBrowser2(const QString& text);//设置最大偏差角计算结果值

	// 设置textEdit的值
	void setTextEdit(const QString& text);
	void setTextEdit2(const QString& text);

	// 获取textEdit的值
	QString getTextEdit() const;
	QString getTextEdit2() const;

	// 获取comboBox的值
	QString getComboBox_4() const;
	QString getComboBox_5() const;

	QString geteditSelection() const;
	void setTextEditValues(const QString& text3, const QString& text5,const QString& text6);

	 std::vector<double> getVertexValues();


signals:
	void pickUpSignal();  // 获取数据信号
	void closeSignal();  // 关闭获取数据信号
	void dialogShown();  // 对话框显示信号
	void cancel(int result);//关闭界面
	void calculateAABB();
	void robotSelectionChanged(const QString& robotName);  // 机器人选择变化信号
	void deleteSelectedSurfaces(const QStringList& deletedSurfaceNames);  // 删除选中项信号
	void previewSignal();  // 预览信号
	void spaceSetting(int number);  // 修改为带参数的信号
	void calculateSpace();
	void confirm();
	void areaPosition();

protected:

private slots:
	void on_next_page_clicked();  // 下一页
	void on_prev_page_clicked();  // 上一页
	void on_cancel_clicked();     // 取消

	void on_pickUpButton_clicked();    // 信号1
	void on_finishButton_clicked();  // 关闭信号

	void on_deleteButton_clicked();  // 删除选中项

	void on_comboBox_currentTextChanged(const QString& text);  // 机器人组合框变化
	void on_previewButton_clicked();  // 预览按钮

	void on_spaceSettingButton_clicked();  // 文本确认按钮
	void on_textEdit_4_textChanged();  // 修改为无参数

	void on_horizontalSlider_valueChanged(int value);//修改划分起点的长度方向位置
	void on_verticalSlider_valueChanged(int value);//修改划分起点的宽度方向位置

	void on_coordanateTextChanged(int state);//选择主划分方向
	void on_confirm_clicked(); //最后确认按钮

private:
	Ui::cursePartClass *ui;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	bool isPickupActive = false;       // 使用成员变量而不是局部变量
	bool isPreview = false;
	bool isPoint = false;

	int indx = 0;

	double x_value;
	double y_value;
	double z_value;

private:
	void setupConnections();    // 初始化连接
	void setupGraphicsScenes(); // 初始化图形场景
	void setStepsExplanation();
	void init();

	// 根据类型获取对象列表
	QMap<ULONG, QString> getObjectsByType(PQDataType objType);

	// 从VARIANT中提取字符串数组
	QStringList extractStringArrayFromVariant(const VARIANT& variant);

	// 从VARIANT中提取长整型数组
	QList<long> extractLongArrayFromVariant(const VARIANT& variant);

	//获取机器人列表
	QStringList getSprayRobotNames(PQRobotType mechanismType, const QMap<ULONG, QString>& robotMap);

	//获取id
	void GetObjIDByName(PQDataType i_nType, std::wstring i_wsName, ULONG &o_uID);

};