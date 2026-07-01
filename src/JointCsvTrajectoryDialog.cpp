#include "JointCsvTrajectoryDialog.h"
#pragma execution_character_set("utf-8")


#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>

JointCsvTrajectoryDialog::JointCsvTrajectoryDialog(
	CComPtr<IPQPlatformComponent> ptrKit,
	QWidget* parent)
	: QDialog(parent)
	, m_ptrKit(ptrKit)
	, m_generator(ptrKit)
{
	buildUi();
	loadRobots();
}

void JointCsvTrajectoryDialog::buildUi()
{
	setWindowTitle(QString::fromUtf8(u8"关节CSV生成轨迹"));
	resize(560, 320);

	auto* root = new QVBoxLayout(this);
	auto* form = new QFormLayout();

	m_csvPathEdit = new QLineEdit(this);
	auto* browseButton = new QPushButton(QString::fromUtf8(u8"浏览"), this);
	auto* csvRow = new QWidget(this);
	auto* csvLayout = new QHBoxLayout(csvRow);
	csvLayout->setContentsMargins(0, 0, 0, 0);
	csvLayout->addWidget(m_csvPathEdit);
	csvLayout->addWidget(browseButton);
	form->addRow(QString::fromUtf8(u8"CSV文件"), csvRow);

	m_trajectoryNameEdit = new QLineEdit(this);
	form->addRow(QString::fromUtf8(u8"新建轨迹名称"), m_trajectoryNameEdit);

	m_machiningRobotCombo = new QComboBox(this);
	form->addRow(QString::fromUtf8(u8"加工机器人"), m_machiningRobotCombo);

	m_guideRobotCombo = new QComboBox(this);
	form->addRow(QString::fromUtf8(u8"变位导轨"), m_guideRobotCombo);

	m_ratedVelocitySpin = new QDoubleSpinBox(this);
	m_ratedVelocitySpin->setRange(0.001, 1000000.0);
	m_ratedVelocitySpin->setValue(50.0);
	m_ratedVelocitySpin->setDecimals(3);
	form->addRow(QString::fromUtf8(u8"额定速度"), m_ratedVelocitySpin);

	m_speedPercentSpin = new QDoubleSpinBox(this);
	m_speedPercentSpin->setRange(0.001, 1000.0);
	m_speedPercentSpin->setValue(100.0);
	m_speedPercentSpin->setDecimals(3);
	form->addRow(QString::fromUtf8(u8"速度百分比"), m_speedPercentSpin);

	m_approachSpin = new QSpinBox(this);
	m_approachSpin->setRange(0, 1000000);
	m_approachSpin->setValue(0);
	form->addRow(QString::fromUtf8(u8"过渡轨迹参数"), m_approachSpin);

	auto* poseRow = new QWidget(this);
	auto* poseLayout = new QHBoxLayout(poseRow);
	poseLayout->setContentsMargins(0, 0, 0, 0);
	m_quaternionRadio = new QRadioButton(QString::fromUtf8(u8"四元数"), this);
	m_eulerRadio = new QRadioButton(QString::fromUtf8(u8"欧拉角"), this);
	m_quaternionRadio->setChecked(true);
	poseLayout->addWidget(m_quaternionRadio);
	poseLayout->addWidget(m_eulerRadio);
	form->addRow(QString::fromUtf8(u8"校验姿态格式"), poseRow);

	root->addLayout(form);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8(u8"生成"));
	buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8(u8"取消"));
	root->addWidget(buttons);

	connect(browseButton, &QPushButton::clicked, this, &JointCsvTrajectoryDialog::browseCsv);
	connect(buttons, &QDialogButtonBox::accepted, this, &JointCsvTrajectoryDialog::generateTrajectory);
	connect(buttons, &QDialogButtonBox::rejected, this, &JointCsvTrajectoryDialog::reject);
}

void JointCsvTrajectoryDialog::loadRobots()
{
	QString error;
	m_robotMap = m_generator.loadRobotObjects(&error);
	if (!error.isEmpty())
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), error);

	for (auto it = m_robotMap.begin(); it != m_robotMap.end(); ++it)
	{
		m_machiningRobotCombo->addItem(it.value(), QVariant::fromValue<qulonglong>(it.key()));
		m_guideRobotCombo->addItem(it.value(), QVariant::fromValue<qulonglong>(it.key()));
	}
}

ULONG JointCsvTrajectoryDialog::currentRobotId(QComboBox* combo) const
{
	return static_cast<ULONG>(combo->currentData().toULongLong());
}

void JointCsvTrajectoryDialog::browseCsv()
{
	const QString path = QFileDialog::getOpenFileName(
		this,
		QString::fromUtf8(u8"选择CSV文件"),
		QStringLiteral("csvData"),
		QStringLiteral("CSV Files (*.csv);;All Files (*.*)"));
	if (!path.isEmpty())
		m_csvPathEdit->setText(path);
}

void JointCsvTrajectoryDialog::generateTrajectory()
{
	JointCsvTrajectoryGenerator::GenerateRequest request;
	request.csvPath = m_csvPathEdit->text().trimmed();
	request.trajectoryName = m_trajectoryNameEdit->text().trimmed();
	request.machiningRobotId = currentRobotId(m_machiningRobotCombo);
	request.machiningRobotName = m_machiningRobotCombo->currentText();
	request.guideRobotId = currentRobotId(m_guideRobotCombo);
	request.guideRobotName = m_guideRobotCombo->currentText();
	request.ratedVelocity = m_ratedVelocitySpin->value();
	request.speedPercent = m_speedPercentSpin->value();
	request.approach = m_approachSpin->value();
	request.poseFormat = m_eulerRadio->isChecked()
		? JointCsvTrajectoryGenerator::PoseFormat::Euler
		: JointCsvTrajectoryGenerator::PoseFormat::Quaternion;

	bool robotCountOk = false;
	const int robotJointCount = m_generator.getRobotJointCount(request.machiningRobotId, &robotCountOk);
	if (robotCountOk && robotJointCount != 6)
	{
		if (QMessageBox::question(
			this,
			QString::fromUtf8(u8"确认"),
			QString::fromUtf8(u8"加工机器人轴数不是6，是否继续？")) != QMessageBox::Yes)
		{
			return;
		}
	}

	bool guideCountOk = false;
	const int guideJointCount = m_generator.getRobotJointCount(request.guideRobotId, &guideCountOk);
	if (guideCountOk && guideJointCount != 3)
	{
		if (QMessageBox::question(
			this,
			QString::fromUtf8(u8"确认"),
			QString::fromUtf8(u8"变位导轨轴数不是3，是否继续？")) != QMessageBox::Yes)
		{
			return;
		}
	}

	const JointCsvTrajectoryGenerator::GenerateResult result = m_generator.generate(request);
	if (!result.success)
	{
		QMessageBox::warning(this, QString::fromUtf8(u8"生成失败"), result.message);
		return;
	}

	QMessageBox::information(
		this,
		QString::fromUtf8(u8"生成完成"),
		QString::fromUtf8(u8"轨迹ID: %1\n总行数: %2\n有效点数: %3\n跳过行数: %4\n校验CSV: %5")
		.arg(result.generatedPathId)
		.arg(result.totalRows)
		.arg(result.validRows)
		.arg(result.skippedRows)
		.arg(result.verificationCsvPath));
	accept();
}
