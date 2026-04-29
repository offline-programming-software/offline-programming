#include "JointCsvTrajectoryGenerator.h"

#include <cmath>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>

namespace
{
	QStringList splitCsvLine(const QString& line)
	{
		QStringList result;
		QString current;
		bool inQuotes = false;

		for (int i = 0; i < line.size(); ++i)
		{
			const QChar ch = line.at(i);
			if (ch == '"')
			{
				if (inQuotes && i + 1 < line.size() && line.at(i + 1) == '"')
				{
					current.append('"');
					++i;
				}
				else
				{
					inQuotes = !inQuotes;
				}
			}
			else if (ch == ',' && !inQuotes)
			{
				result.append(current.trimmed());
				current.clear();
			}
			else
			{
				current.append(ch);
			}
		}

		result.append(current.trimmed());
		return result;
	}

	bool valueAt(const QStringList& values, int index, QString& output)
	{
		if (index < 0 || index >= values.size())
			return false;
		output = values.at(index).trimmed();
		return !output.isEmpty();
	}

	QStringList extractStringArrayFromVariantLocal(const VARIANT& variant)
	{
		QStringList result;
		if ((variant.vt & VT_ARRAY) == 0 || variant.vt != (VT_ARRAY | VT_BSTR))
			return result;

		SAFEARRAY* array = variant.parray;
		if (!array || array->cDims != 1)
			return result;

		long lowerBound = 0;
		long upperBound = -1;
		if (FAILED(SafeArrayGetLBound(array, 1, &lowerBound)) ||
			FAILED(SafeArrayGetUBound(array, 1, &upperBound)))
		{
			return result;
		}

		BSTR* data = nullptr;
		if (FAILED(SafeArrayAccessData(array, reinterpret_cast<void**>(&data))))
			return result;

		for (long i = lowerBound; i <= upperBound; ++i)
			result.append(data[i] ? QString::fromWCharArray(data[i]) : QString());

		SafeArrayUnaccessData(array);
		return result;
	}

	QList<long> extractLongArrayFromVariantLocal(const VARIANT& variant)
	{
		QList<long> result;
		if ((variant.vt & VT_ARRAY) == 0 || !variant.parray || variant.parray->cDims != 1)
			return result;

		SAFEARRAY* array = variant.parray;
		long lowerBound = 0;
		long upperBound = -1;
		if (FAILED(SafeArrayGetLBound(array, 1, &lowerBound)) ||
			FAILED(SafeArrayGetUBound(array, 1, &upperBound)))
		{
			return result;
		}

		VARTYPE vt = VT_EMPTY;
		SafeArrayGetVartype(array, &vt);
		if (FAILED(SafeArrayLock(array)))
			return result;

		void* data = array->pvData;
		for (long i = lowerBound; i <= upperBound; ++i)
		{
			const long offset = i - lowerBound;
			long value = 0;
			switch (vt)
			{
			case VT_I4:
				value = static_cast<LONG*>(data)[offset];
				break;
			case VT_I2:
				value = static_cast<SHORT*>(data)[offset];
				break;
			case VT_UI4:
				value = static_cast<ULONG*>(data)[offset];
				break;
			default:
				value = 0;
				break;
			}
			result.append(value);
		}

		SafeArrayUnlock(array);
		return result;
	}
}

JointCsvTrajectoryGenerator::JointCsvTrajectoryGenerator(CComPtr<IPQPlatformComponent> ptrKit)
	: m_ptrKit(ptrKit)
{
}

QString JointCsvTrajectoryGenerator::normalizeHeader(const QString& value)
{
	return value.trimmed().toLower();
}

bool JointCsvTrajectoryGenerator::parseDouble(const QString& value, double& output)
{
	bool ok = false;
	const double parsed = value.trimmed().toDouble(&ok);
	if (!ok || !std::isfinite(parsed))
		return false;
	output = parsed;
	return true;
}

double JointCsvTrajectoryGenerator::degreesToRadians(double degrees)
{
	return degrees * 3.14159265358979323846 / 180.0;
}

QMap<ULONG, QString> JointCsvTrajectoryGenerator::loadRobotObjects(QString* errorMessage) const
{
	QMap<ULONG, QString> objects;
	if (!m_ptrKit)
	{
		if (errorMessage)
			*errorMessage = QStringLiteral("PQKit is not initialized.");
		return objects;
	}

	VARIANT namesVariant;
	VARIANT idsVariant;
	VariantInit(&namesVariant);
	VariantInit(&idsVariant);

	HRESULT hr = m_ptrKit->Doc_get_obj_bytype(PQ_ROBOT, &namesVariant, &idsVariant);
	if (FAILED(hr))
	{
		if (errorMessage)
			*errorMessage = QStringLiteral("Failed to load robot objects. HRESULT=%1").arg(hr);
		VariantClear(&namesVariant);
		VariantClear(&idsVariant);
		return objects;
	}

	const QStringList names = extractStringArrayFromVariantLocal(namesVariant);
	const QList<long> ids = extractLongArrayFromVariantLocal(idsVariant);
	const int count = qMin(names.size(), ids.size());
	for (int i = 0; i < count; ++i)
		objects.insert(static_cast<ULONG>(ids.at(i)), names.at(i));

	VariantClear(&namesVariant);
	VariantClear(&idsVariant);
	return objects;
}

int JointCsvTrajectoryGenerator::getRobotJointCount(ULONG robotId, bool* ok) const
{
	if (ok)
		*ok = false;
	if (!m_ptrKit || robotId == 0)
		return 0;

	INT count = 0;
	HRESULT hr = m_ptrKit->Robot_get_joints_count(robotId, &count);
	if (FAILED(hr))
		return 0;

	if (ok)
		*ok = true;
	return count;
}

JointCsvTrajectoryGenerator::ParseResult JointCsvTrajectoryGenerator::parseCsv(
	const QString& csvPath,
	double ratedVelocity) const
{
	ParseResult result;
	QFile file(csvPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		result.errors.append(QStringLiteral("CSV file cannot be opened: %1").arg(csvPath));
		return result;
	}

	QTextStream in(&file);
	in.setCodec("UTF-8");

	if (in.atEnd())
	{
		result.errors.append(QStringLiteral("CSV file is empty: %1").arg(csvPath));
		return result;
	}

	const QString headerLine = in.readLine();
	const QStringList headers = splitCsvLine(headerLine);
	QMap<QString, int> headerIndex;
	for (int i = 0; i < headers.size(); ++i)
		headerIndex.insert(normalizeHeader(headers.at(i)), i);

	const QStringList required = {
		"j1_deg", "j2_deg", "j3_deg", "j4_deg", "j5_deg", "j6_deg",
		"r1_pos_mm", "r2_pos_mm", "r3_pos_mm"
	};

	for (const QString& key : required)
	{
		if (!headerIndex.contains(key))
			result.errors.append(QStringLiteral("CSV missing required column: %1").arg(key));
	}

	if (!result.errors.isEmpty())
		return result;

	int lineNumber = 1;
	while (!in.atEnd())
	{
		++lineNumber;
		const QString line = in.readLine();
		if (line.trimmed().isEmpty())
			continue;

		++result.totalDataRows;
		const QStringList values = splitCsvLine(line);
		CsvRow row;
		row.sourceLine = lineNumber;

		bool valid = true;
		QString raw;
		const char* jointKeys[6] = { "j1_deg", "j2_deg", "j3_deg", "j4_deg", "j5_deg", "j6_deg" };
		for (int i = 0; i < 6; ++i)
		{
			if (!valueAt(values, headerIndex.value(jointKeys[i]), raw) ||
				!parseDouble(raw, row.robotJointsDeg[static_cast<size_t>(i)]))
			{
				valid = false;
				break;
			}
		}

		const char* guideKeys[3] = { "r1_pos_mm", "r2_pos_mm", "r3_pos_mm" };
		for (int i = 0; valid && i < 3; ++i)
		{
			if (!valueAt(values, headerIndex.value(guideKeys[i]), raw) ||
				!parseDouble(raw, row.guideJoints[static_cast<size_t>(i)]))
			{
				valid = false;
				break;
			}
		}

		if (!valid)
		{
			++result.skippedRows;
			continue;
		}

		if (headerIndex.contains("index") && valueAt(values, headerIndex.value("index"), raw))
			row.index = raw;
		if (headerIndex.contains("time") && valueAt(values, headerIndex.value("time"), raw))
			row.time = raw;

		row.pathVelocity = ratedVelocity;
		if (headerIndex.contains("pathvel") && valueAt(values, headerIndex.value("pathvel"), raw))
		{
			double parsedVelocity = 0.0;
			if (parseDouble(raw, parsedVelocity))
			{
				row.pathVelocity = parsedVelocity;
				row.hasPathVelocity = true;
			}
		}

		result.rows.push_back(row);
	}

	return result;
}

QString JointCsvTrajectoryGenerator::validateRequest(
	const GenerateRequest& request,
	const ParseResult& parseResult) const
{
	if (!m_ptrKit)
		return QStringLiteral("PQKit is not initialized.");
	if (request.csvPath.trimmed().isEmpty())
		return QStringLiteral("CSV path is empty.");
	if (!QFileInfo::exists(request.csvPath))
		return QStringLiteral("CSV file does not exist.");
	if (request.trajectoryName.trimmed().isEmpty())
		return QStringLiteral("Trajectory name is empty.");
	if (request.machiningRobotId == 0)
		return QStringLiteral("Machining robot is not selected.");
	if (request.guideRobotId == 0)
		return QStringLiteral("Positioning guide is not selected.");
	if (request.machiningRobotId == request.guideRobotId)
		return QStringLiteral("Machining robot and positioning guide must be different objects.");
	if (request.ratedVelocity <= 0.0)
		return QStringLiteral("Rated velocity must be greater than zero.");
	if (request.speedPercent <= 0.0)
		return QStringLiteral("Speed percent must be greater than zero.");
	if (request.approach < 0)
		return QStringLiteral("Approach must be zero or greater.");
	if (!parseResult.errors.isEmpty())
		return parseResult.errors.join(QStringLiteral("\n"));
	if (parseResult.rows.empty())
		return QStringLiteral("CSV contains no valid data rows.");
	return {};
}

QString JointCsvTrajectoryGenerator::uniqueVerificationPath(const QString& trajectoryName)
{
	QDir dir(QStringLiteral("csvData"));
	if (!dir.exists())
		dir.mkpath(QStringLiteral("."));

	QString safeName = trajectoryName.trimmed();
	safeName.replace(QRegExp(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
	if (safeName.isEmpty())
		safeName = QStringLiteral("joint_csv_trajectory");

	QString candidate = dir.filePath(QStringLiteral("%1_fk_verify.csv").arg(safeName));
	int suffix = 1;
	while (QFileInfo::exists(candidate))
	{
		candidate = dir.filePath(QStringLiteral("%1_fk_verify_%2.csv").arg(safeName).arg(suffix));
		++suffix;
	}
	return candidate;
}

//JointCsvTrajectoryGenerator::GenerateResult JointCsvTrajectoryGenerator::callAddAbsJointPath(
//	const GenerateRequest& request,
//	const ParseResult& parseResult)
//{
//	GenerateResult result;
//	result.totalRows = parseResult.totalDataRows;
//	result.validRows = static_cast<int>(parseResult.rows.size());
//	result.skippedRows = parseResult.skippedRows;
//
//	std::vector<DOUBLE> robotJoints;
//	std::vector<DOUBLE> guideJoints;
//	std::vector<DOUBLE> velocities;
//	std::vector<DOUBLE> speedPercents;
//	std::vector<INT> approaches;
//
//	robotJoints.reserve(parseResult.rows.size() * 6);
//	guideJoints.reserve(parseResult.rows.size() * 3);
//	velocities.reserve(parseResult.rows.size());
//	speedPercents.reserve(parseResult.rows.size());
//	approaches.reserve(parseResult.rows.size());
//
//	for (const CsvRow& row : parseResult.rows)
//	{
//		for (double value : row.robotJointsDeg)
//			robotJoints.push_back(degreesToRadians(value));
//		for (double value : row.guideJoints)
//			guideJoints.push_back(value);
//		velocities.push_back(row.pathVelocity);
//		speedPercents.push_back(request.speedPercent);
//		approaches.push_back(request.approach);
//	}
//
//	ULONG generatedPathId = 0;
//	const ULONG insertionPathId = 0;
//	result.hr = m_ptrKit->PQAPIAddAbsJointPath(
//		request.machiningRobotId,
//		robotJoints.data(),
//		6,
//		guideJoints.data(),
//		3,
//		nullptr,
//		0,
//		velocities.data(),
//		speedPercents.data(),
//		approaches.data(),
//		static_cast<INT>(parseResult.rows.size()),
//		insertionPathId,
//		&generatedPathId);
//
//	result.generatedPathId = generatedPathId;
//	if (FAILED(result.hr) || generatedPathId == 0)
//	{
//		result.success = false;
//		result.message = QStringLiteral("PQAPIAddAbsJointPath failed. HRESULT=%1, pathId=%2")
//			.arg(result.hr)
//			.arg(generatedPathId);
//		return result;
//	}
//
//	result.success = true;
//	result.message = QStringLiteral("Trajectory generated.");
//	return result;
//}

JointCsvTrajectoryGenerator::GenerateResult JointCsvTrajectoryGenerator::callAddAbsJointPath(
	const GenerateRequest& request,
	const ParseResult& parseResult)
{
	GenerateResult result;
	result.totalRows = parseResult.totalDataRows;
	result.validRows = static_cast<int>(parseResult.rows.size());
	result.skippedRows = parseResult.skippedRows;

	std::vector<DOUBLE> robotJoints;
	std::vector<DOUBLE> guideJoints;
	std::vector<DOUBLE> velocities;
	std::vector<DOUBLE> speedPercents;
	std::vector<INT> approaches;

	robotJoints.reserve(parseResult.rows.size() * 6);
	guideJoints.reserve(parseResult.rows.size() * 3);
	velocities.reserve(parseResult.rows.size());
	speedPercents.reserve(parseResult.rows.size());
	approaches.reserve(parseResult.rows.size());

	for (const CsvRow& row : parseResult.rows)
	{
		for (double value : row.robotJointsDeg)
			robotJoints.push_back(degreesToRadians(value));
		for (double value : row.guideJoints)
			guideJoints.push_back(value);
		velocities.push_back(row.pathVelocity);
		speedPercents.push_back(request.speedPercent);
		approaches.push_back(request.approach);
	}

	const INT pointCount = static_cast<INT>(parseResult.rows.size());
	std::vector<PQPointInstruction> instructs(static_cast<size_t>(pointCount),PQ_LINE);
	std::vector<DOUBLE> positionerJoints(1, 0.0);

	const INT robotJointsSize = static_cast<INT>(robotJoints.size());
	const INT guideJointsSize = static_cast<INT>(guideJoints.size());
	const INT positionerJointsSize = 0;

	CComBSTR pathName(request.trajectoryName.toStdWString().c_str());
	CComBSTR groupName(L"testGrp");
	const ULONG coordinateId = 0;
	const long isUpdate = TRUE;

	ULONG generatedPathId = 0;
	result.hr = m_ptrKit->Path_insert_from_joint(
		request.machiningRobotId,
		robotJoints.data(),
		robotJointsSize,
		guideJoints.data(),
		guideJointsSize,
		positionerJoints.data(),
		positionerJointsSize,
		pointCount,
		instructs.data(),
		velocities.data(),
		speedPercents.data(),
		approaches.data(),
		pathName,
		groupName,
		coordinateId,
		&generatedPathId,
		isUpdate);

	result.generatedPathId = generatedPathId;
	if (FAILED(result.hr) || generatedPathId == 0)
	{
		result.success = false;
		result.message = QStringLiteral("Path_insert_from_joint failed. HRESULT=%1, pathId=%2")
			.arg(result.hr)
			.arg(generatedPathId);
		return result;
	}

	result.success = true;
	result.message = QStringLiteral("Trajectory generated.");
	return result;
}

std::vector<JointCsvTrajectoryGenerator::PoseRow>
JointCsvTrajectoryGenerator::readGeneratedPoses(ULONG pathId, PoseFormat format) const
{
	std::vector<PoseRow> poses;
	if (!m_ptrKit || pathId == 0)
		return poses;

	INT pointCount = 0;
	ULONG* pointIds = nullptr;
	HRESULT hr = m_ptrKit->Path_get_point_id(pathId, &pointCount, &pointIds);
	if (FAILED(hr) || pointCount <= 0 || !pointIds)
		return poses;

	poses.reserve(static_cast<size_t>(pointCount));
	const PQPostureType postureType = (format == PoseFormat::Quaternion) ? QUATERNION : EULERANGLEXYZ;

	for (INT i = 0; i < pointCount; ++i)
	{
		PoseRow row;
		row.pointId = pointIds[i];

		INT postureCount = 0;
		DOUBLE* posture = nullptr;
		DOUBLE velocity = 0.0;
		DOUBLE speedPercent = 0.0;
		PQPointInstruction instruct = PQ_LINE;
		INT approach = 0;

		HRESULT infoHr = m_ptrKit->PQAPIGetPointInfo(
			row.pointId,
			postureType,
			&postureCount,
			&posture,
			&velocity,
			&speedPercent,
			&instruct,
			&approach);

		if (SUCCEEDED(infoHr) && posture && postureCount > 0)
		{
			row.ok = true;
			row.values.assign(posture, posture + postureCount);
		}
		else
		{
			row.ok = false;
			row.error = QStringLiteral("PQAPIGetPointInfo failed. HRESULT=%1").arg(infoHr);
		}

		if (posture)
			m_ptrKit->PQAPIFree(reinterpret_cast<LONG_PTR*>(posture));

		poses.push_back(row);
	}

	m_ptrKit->PQAPIFreeArray(reinterpret_cast<LONG_PTR*>(pointIds));
	return poses;
}

bool JointCsvTrajectoryGenerator::exportVerificationCsv(
	const GenerateRequest& request,
	const ParseResult& parseResult,
	GenerateResult& result) const
{
	result.verificationCsvPath = uniqueVerificationPath(request.trajectoryName);
	QFile file(result.verificationCsvPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		result.message = QStringLiteral("Generated trajectory, but failed to write verification CSV: %1")
			.arg(result.verificationCsvPath);
		return false;
	}

	const std::vector<PoseRow> poses = readGeneratedPoses(result.generatedPathId, request.poseFormat);
	result.poseReadbackFailures = 0;

	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << "sourceLine,index,time,pathVel,J1_deg,J2_deg,J3_deg,J4_deg,J5_deg,J6_deg,"
		<< "R1_pos_mm,R2_pos_mm,R3_pos_mm,generatedPathId,pointId,x,y,z";
	if (request.poseFormat == PoseFormat::Quaternion)
		out << ",qw,qx,qy,qz";
	else
		out << ",rx,ry,rz";
	out << ",status\n";

	for (size_t i = 0; i < parseResult.rows.size(); ++i)
	{
		const CsvRow& row = parseResult.rows[i];
		out << row.sourceLine << ','
			<< row.index << ','
			<< row.time << ','
			<< row.pathVelocity;
		for (double value : row.robotJointsDeg)
			out << ',' << value;
		for (double value : row.guideJoints)
			out << ',' << value;
		out << ',' << result.generatedPathId;

		const bool hasPose = i < poses.size() && poses[i].ok;
		if (hasPose)
		{
			out << ',' << poses[i].pointId;
			const int expectedCount = request.poseFormat == PoseFormat::Quaternion ? 7 : 6;
			for (int j = 0; j < expectedCount; ++j)
			{
				const double value = j < static_cast<int>(poses[i].values.size()) ? poses[i].values[static_cast<size_t>(j)] : 0.0;
				out << ',' << value;
			}
			out << ",ok\n";
		}
		else
		{
			++result.poseReadbackFailures;
			const int emptyCount = request.poseFormat == PoseFormat::Quaternion ? 8 : 7;
			for (int j = 0; j < emptyCount; ++j)
				out << ',';
			out << "pose_readback_failed\n";
		}
	}

	return true;
}

JointCsvTrajectoryGenerator::GenerateResult JointCsvTrajectoryGenerator::generate(const GenerateRequest& request)
{
	const ParseResult parseResult = parseCsv(request.csvPath, request.ratedVelocity);
	const QString validationError = validateRequest(request, parseResult);
	if (!validationError.isEmpty())
	{
		GenerateResult result;
		result.success = false;
		result.totalRows = parseResult.totalDataRows;
		result.validRows = static_cast<int>(parseResult.rows.size());
		result.skippedRows = parseResult.skippedRows;
		result.message = validationError;
		return result;
	}

	GenerateResult result = callAddAbsJointPath(request, parseResult);
	if (!result.success)
		return result;

	exportVerificationCsv(request, parseResult, result);
	return result;
}

