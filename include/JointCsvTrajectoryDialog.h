#pragma once

#pragma execution_character_set("utf-8")
#define NOMINMAX

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

#include <atlbase.h>

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMap>
#include <QRadioButton>
#include <QSpinBox>

#include "JointCsvTrajectoryGenerator.h"

class JointCsvTrajectoryDialog : public QDialog
{
public:
	explicit JointCsvTrajectoryDialog(CComPtr<IPQPlatformComponent> ptrKit, QWidget* parent = nullptr);

private:
	void buildUi();
	void loadRobots();
	void browseCsv();
	void generateTrajectory();
	ULONG currentRobotId(QComboBox* combo) const;

private:
	CComPtr<IPQPlatformComponent> m_ptrKit;
	JointCsvTrajectoryGenerator m_generator;
	QMap<ULONG, QString> m_robotMap;

	QLineEdit* m_csvPathEdit = nullptr;
	QLineEdit* m_trajectoryNameEdit = nullptr;
	QComboBox* m_machiningRobotCombo = nullptr;
	QComboBox* m_guideRobotCombo = nullptr;
	QDoubleSpinBox* m_ratedVelocitySpin = nullptr;
	QDoubleSpinBox* m_speedPercentSpin = nullptr;
	QSpinBox* m_approachSpin = nullptr;
	QRadioButton* m_quaternionRadio = nullptr;
	QRadioButton* m_eulerRadio = nullptr;
};

