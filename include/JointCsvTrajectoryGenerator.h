#pragma once

#pragma execution_character_set("utf-8")
#define NOMINMAX

#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

#include <atlbase.h>
#include <array>
#include <vector>

#include <QMap>
#include <QString>
#include <QStringList>

class JointCsvTrajectoryGenerator
{
public:
	enum class PoseFormat
	{
		Quaternion,
		Euler
	};

	struct CsvRow
	{
		int sourceLine = 0;
		QString index;
		QString time;
		double pathVelocity = 0.0;
		bool hasPathVelocity = false;
		std::array<double, 6> robotJointsDeg = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		std::array<double, 3> guideJoints = { 0.0, 0.0, 0.0 };
	};

	struct ParseResult
	{
		int totalDataRows = 0;
		int skippedRows = 0;
		QStringList errors;
		std::vector<CsvRow> rows;
	};

	struct GenerateRequest
	{
		QString csvPath;
		QString trajectoryName;
		ULONG machiningRobotId = 0;
		QString machiningRobotName;
		ULONG guideRobotId = 0;
		QString guideRobotName;
		double ratedVelocity = 50.0;
		double speedPercent = 100.0;
		int approach = 0;
		PoseFormat poseFormat = PoseFormat::Quaternion;
	};

	struct GenerateResult
	{
		bool success = false;
		HRESULT hr = S_OK;
		ULONG generatedPathId = 0;
		int totalRows = 0;
		int validRows = 0;
		int skippedRows = 0;
		int poseReadbackFailures = 0;
		QString verificationCsvPath;
		QString message;
	};

	struct PoseRow
	{
		bool ok = false;
		ULONG pointId = 0;
		std::vector<double> values;
		QString error;
	};

	explicit JointCsvTrajectoryGenerator(CComPtr<IPQPlatformComponent> ptrKit);

	QMap<ULONG, QString> loadRobotObjects(QString* errorMessage = nullptr) const;
	int getRobotJointCount(ULONG robotId, bool* ok = nullptr) const;
	ParseResult parseCsv(const QString& csvPath, double ratedVelocity) const;
	QString validateRequest(const GenerateRequest& request, const ParseResult& parseResult) const;
	GenerateResult generate(const GenerateRequest& request);

private:
	static QString normalizeHeader(const QString& value);
	static bool parseDouble(const QString& value, double& output);
	static QString uniqueVerificationPath(const QString& trajectoryName);
	static double degreesToRadians(double degrees);

	GenerateResult callAddAbsJointPath(const GenerateRequest& request, const ParseResult& parseResult);
	std::vector<PoseRow> readGeneratedPoses(ULONG pathId, PoseFormat format) const;
	bool exportVerificationCsv(const GenerateRequest& request,
		const ParseResult& parseResult,
		GenerateResult& result) const;

private:
	CComPtr<IPQPlatformComponent> m_ptrKit;
};

