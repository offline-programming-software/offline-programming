#include "TrajectoryFileGenerator.h"
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <comdef.h>
#include <algorithm>
#include <cwctype>

namespace {
	std::wstring trimWString(const std::wstring& input) {
		size_t begin = 0;
		while (begin < input.size() && iswspace(input[begin])) {
			++begin;
		}

		size_t end = input.size();
		while (end > begin && iswspace(input[end - 1])) {
			--end;
		}

		return input.substr(begin, end - begin);
	}

	std::wstring toLowerWString(std::wstring value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
		return value;
	}

	bool shouldPlaceEventBeforeMOVD(const event_information& eventInfo) {
		if (!eventInfo.hasEvent) {
			return false;
		}

		const wchar_t* rawPos = static_cast<const wchar_t*>(eventInfo.position);
		if (rawPos == nullptr) {
			return false;
		}

		std::wstring position = toLowerWString(trimWString(std::wstring(rawPos)));

		// 兼容中英文配置
		return position == L"前"
			|| position == L"点前"
			|| position == L"before"
			|| position == L"pre"
			|| position == L"front";
	}
}

// 构造函数
TrajectoryFileGenerator::TrajectoryFileGenerator(const std::string& filename,
	const std::string& jobName)
	: filename_(filename), jobName_(jobName),
	nposParams_("0,0,0,0,0,0"), postype_(0), version_("[1.0.0]"),
	robotJointsCount_(6), externalAxesCount_(0) {

	// 设置当前日期时间
	time_t now = time(0);
	tm localTime;
	if (localtime_s(&localTime, &now) == 0) {
		char buffer[80];
		strftime(buffer, 80, "DATE=%Y/%d /%d %H:%M:%S", &localTime);
		date_ = buffer;
	}
	else {
		date_ = "DATE=2023/01/01 00:00:00";
	}
}

// 生成RobotType字符串
std::string TrajectoryFileGenerator::generateRobotTypeString() const {
	std::ostringstream oss;
	//oss << "R_" << std::setw(2) << std::setfill('0') << robotJointsCount_ << "_" << externalAxesCount_ << "_SPRAY";
	oss << "R_07_00_SPRAY";
	return oss.str();
}

// 添加轨迹点信息
void TrajectoryFileGenerator::addTrajectoryPoint(const TrajectoryPointInfo& pointInfo) {
	trajectoryPoints_.push_back(pointInfo);

	// 创建位置点
	PositionPoint posPoint = createPositionPointFromTrajectoryInfo(pointInfo);
	positions_.push_back(posPoint);

	// 创建运动与事件指令
	const std::string motionInstruction = createMotionInstructionFromTrajectoryInfo(pointInfo);
	const std::vector<std::string> eventInstructions = createEventInstructions(pointInfo.event);

	auto appendInstruction = [this](const std::string& instr) {
		if (!instr.empty()) {
			validateInstructionLength(instr);
			instructions_.push_back(instr);
		}
	};

	auto appendEventInstructions = [this](const std::vector<std::string>& instrs) {
		for (const auto& instr : instrs) {
			validateInstructionLength(instr);
			instructions_.push_back(instr);
		}
	};

	// 需求：MOVD 的喷涂事件按“点前/点后”放置
	const bool isMOVD = (pointInfo.motionType == "MOVD");
	const bool eventBeforeMOVD = shouldPlaceEventBeforeMOVD(pointInfo.event);

	if (isMOVD && eventBeforeMOVD) {
		appendEventInstructions(eventInstructions);
		appendInstruction(motionInstruction);
	}
	else {
		appendInstruction(motionInstruction);
		appendEventInstructions(eventInstructions);
	}
}

// 从轨迹点信息创建位置点
TrajectoryFileGenerator::PositionPoint TrajectoryFileGenerator::createPositionPointFromTrajectoryInfo(
	const TrajectoryPointInfo& pointInfo) const {

	PositionPoint posPoint;
	posPoint.index = pointInfo.pointIndex;

	// 合并机器人关节和外部轴值
	posPoint.values = pointInfo.robotJoints;
	posPoint.values.insert(posPoint.values.end(),
		pointInfo.externalAxes.begin(),
		pointInfo.externalAxes.end());

	// 确保轨迹点有十位，未用位设为0
	while (posPoint.values.size() < 10) {
		posPoint.values.push_back(0.0);
	}

	// 如果超过十位，截断到十位
	if (posPoint.values.size() > 10) {
		posPoint.values.resize(10);
	}

	return posPoint;
}

// 从轨迹点信息创建运动指令
std::string TrajectoryFileGenerator::createMotionInstructionFromTrajectoryInfo(
	const TrajectoryPointInfo& pointInfo) const {

	std::ostringstream instruction;

	std::string pointName = "P" + std::to_string(pointInfo.pointIndex);

	// 确保点名称格式为 P+六位数字
	if (pointName.length() < 7) {
		pointName = "P" + std::string(6 - std::to_string(pointInfo.pointIndex).length(), '0') + std::to_string(pointInfo.pointIndex);
	}

	if (pointInfo.motionType == "MOVJ") {
		instruction << "MOVJ " << pointName << " DYN " << pointInfo.motionPercentage;
	}
	else if (pointInfo.motionType == "MOVD") {
		instruction << "MOVD " << pointName;
	}
	else if (pointInfo.motionType == "PTP") {
		if (!pointInfo.externalAxes.empty()) {
			instruction << "PTP AXIS 11 " << pointInfo.externalAxes[0] << " " << pointInfo.velocity;
		}
	}
	// 可以继续添加其他运动类型...

	return instruction.str();
}

// 从事件信息创建指令
std::vector<std::string> TrajectoryFileGenerator::createEventInstructions(
	const event_information& event) const {

	std::vector<std::string> instructions;

	if (!event.hasEvent) {
		return instructions;
	}

	const std::string eventName = std::string(CW2A(event.name));

	// 规则优先：Brush -> 开枪；关枪 -> 关枪
	if (eventName.find("Brush") != std::string::npos ||
		eventName.find("BRUSH") != std::string::npos ||
		eventName.find("brush") != std::string::npos) {
		instructions.push_back("OUT GUN1 TRUE");
		return instructions;
	}

	if (eventName.find("关枪") != std::string::npos) {
		instructions.push_back("OUT GUN1 FALSE");
		return instructions;
	}

	if (eventName.find("OUT") != std::string::npos) {
		std::string content = std::string(CW2A(event.content));
		bool state = (content == "TRUE" || content == "true");
		std::ostringstream instr;
		instr << "OUT " << eventName.substr(3) << " " << (state ? "TRUE" : "FALSE");
		instructions.push_back(instr.str());
	}
	else if (eventName.find("ACT") != std::string::npos) {
		std::string content = std::string(CW2A(event.content));
		std::ostringstream instr;
		instr << "ACT BT " << content;
		instructions.push_back(instr.str());
	}
	else if (eventName.find("DELAY") != std::string::npos) {
		std::string content = std::string(CW2A(event.content));
		std::ostringstream instr;
		instr << "DELAY " << content;
		instructions.push_back(instr.str());
	}
	else {
		std::string eventLabel = std::string(CW2A(event.name));
		std::string content = std::string(CW2A(event.content));
		std::ostringstream instr;
		instr << "; SPRAY_EVENT NAME=" << eventLabel << " " << content;
		instructions.push_back(instr.str());
	}

	return instructions;
}

// 验证指令长度
void TrajectoryFileGenerator::validateInstructionLength(const std::string& instruction) {
	if (instruction.length() > 140) {
		throw std::runtime_error("Instruction exceeds 140 characters: " + instruction);
	}
}

std::string TrajectoryFileGenerator::padLineToWidth(const std::string& line, size_t targetWidth) const
{
	if (line.length() >= targetWidth) {
		return line;
	}
	return line + std::string(targetWidth - line.length(), ' ');
}

// 添加位置点
void TrajectoryFileGenerator::addPositionPoint(const PositionPoint& point) {
	// 确保轨迹点有十位，未用位设为0
	PositionPoint normalizedPoint = point;
	while (normalizedPoint.values.size() < 10) {
		normalizedPoint.values.push_back(0.0);
	}

	// 如果超过十位，截断十位
	if (normalizedPoint.values.size() > 10) {
		normalizedPoint.values.resize(10);
	}

	positions_.push_back(normalizedPoint);
}

// 添加通用指令
void TrajectoryFileGenerator::addInstruction(const std::string& instruction) {
	validateInstructionLength(instruction);
	instructions_.push_back(instruction);
}

// OUT指令
void TrajectoryFileGenerator::addOUTInstruction(const std::string& signalName, bool state, int delayMs) {
	std::ostringstream instruction;
	instruction << "OUT " << signalName << " " << (state ? "TRUE" : "FALSE");

	if (delayMs > 0) {
		instruction << " DELAY " << delayMs;
	}

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

// ACT指令
void TrajectoryFileGenerator::addACTInstruction(const std::string& brushTableName) {
	std::ostringstream instruction;
	instruction << "ACT BT " << brushTableName;

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

// PTP指令
void TrajectoryFileGenerator::addPTPInstruction(int axisNumber, double position, double velocity) {
	std::ostringstream instruction;
	instruction << "PTP AXIS " << axisNumber << " " << position << " " << velocity;

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

void TrajectoryFileGenerator::addMOVJInstruction(int pointIndex, int speedPercent) {
	std::ostringstream instruction;
	std::string pointName = "P" + std::string(6 - std::to_string(pointIndex).length(), '0') + std::to_string(pointIndex);
	instruction << "MOVJ " << pointName << " DYN " << speedPercent;

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

// 修改 addMOVDInstruction 函数
void TrajectoryFileGenerator::addMOVDInstruction(int pointIndex) {
	std::ostringstream instruction;
	std::string pointName = "P" + std::string(6 - std::to_string(pointIndex).length(), '0') + std::to_string(pointIndex);
	instruction << "MOVD " << pointName;

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}
// DELAY指令
void TrajectoryFileGenerator::addDELAYInstruction(int delayMs) {
	std::ostringstream instruction;
	instruction << "DELAY " << delayMs;

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

// WAIT指令
void TrajectoryFileGenerator::addWAITInstruction(const std::string& variableName, bool expectedState) {
	std::ostringstream instruction;
	instruction << "WAIT " << variableName << " " << (expectedState ? "TRUE" : "FALSE");

	std::string fullInstruction = instruction.str();
	validateInstructionLength(fullInstruction);
	instructions_.push_back(fullInstruction);
}

// 设置NPOS参数
void TrajectoryFileGenerator::setNposParams(const std::string& nposParams) {
	nposParams_ = nposParams;
}

// 设置版本
void TrajectoryFileGenerator::setVersion(const std::string& version) {
	version_ = version;
}

// 设置日期
void TrajectoryFileGenerator::setDate(const std::string& date) {
	date_ = date;
}

// 设置位置类型
void TrajectoryFileGenerator::setPostype(int postype) {
	postype_ = postype;
}

// 设置机器人轴数量
void TrajectoryFileGenerator::setRobotJointsCount(int count) {
	robotJointsCount_ = count;
}

// 设置导轨轴数量
void TrajectoryFileGenerator::setExternalAxesCount(int count) {
	externalAxesCount_ = count;
}

// 自动设置NPOS参数
void TrajectoryFileGenerator::calculateAndSetNposParams() {
	// 统计各种指令类型的数量
	int totalInstructions = instructions_.size();
	int ptpMovjCount = 0;
	int ptpMovjMovdCount = 0;

	// 遍历所有指令，统计不同类型
	for (const auto& instruction : instructions_) {
		// 检查指令类型
		if (instruction.find("PTP") != std::string::npos ||
			instruction.find("MOVJ") != std::string::npos) {
			ptpMovjCount++;
		}

		if (instruction.find("PTP") != std::string::npos ||
			instruction.find("MOVJ") != std::string::npos ||
			instruction.find("MOVD") != std::string::npos) {
			ptpMovjMovdCount++;
		}
	}

	std::ostringstream nposStream;
	nposStream << totalInstructions << ","
		<< ptpMovjCount << ","
		<< ptpMovjMovdCount << ",0,0,0";

	nposParams_ = nposStream.str();
}

// 从命令行参数设置NPOS
void TrajectoryFileGenerator::setNposFromCommandLine(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i) {
		args.push_back(argv[i]);
	}
	setNposFromCommandLine(args);
}

void TrajectoryFileGenerator::setNposFromCommandLine(const std::vector<std::string>& args) {
	for (size_t i = 0; i < args.size(); ++i) {
		const std::string& arg = args[i];

		if (arg == "--npos" && i + 1 < args.size()) {
			nposParams_ = args[++i];
		}
		else if (arg == "--auto-npos") {
			calculateAndSetNposParams();
		}
		else if (arg == "--robot-joints" && i + 1 < args.size()) {
			robotJointsCount_ = std::stoi(args[++i]);
		}
		else if (arg == "--external-axes" && i + 1 < args.size()) {
			externalAxesCount_ = std::stoi(args[++i]);
		}
	}
}

// 生成文件
void TrajectoryFileGenerator::generate() {
	std::ofstream outFile(filename_);
	if (!outFile.is_open()) {
		throw std::runtime_error("Failed to open output file: " + filename_);
	}

	writeContent(outFile);
	outFile.close();
}

// 生成字符串
std::string TrajectoryFileGenerator::generateToString() {
	std::ostringstream stream;
	writeContent(stream);
	return stream.str();
}

// 格式化位置值
std::string TrajectoryFileGenerator::formatPositionValue(double value) const {
	std::ostringstream oss;

	// 符号处理
	if (value < 0) {
		oss << "-";
		value = -value;
	}
	else {
		oss << "0";
	}

	// 整数部分
	int intPart = static_cast<int>(value);
	oss << std::setw(4) << std::setfill('0') << intPart;

	// 小数部分
	double fractionalPart = value - intPart;
	oss << "." << std::setw(6) << std::setfill('0') << static_cast<int>(round(fractionalPart * 1000000));

	return oss.str();
}

std::string TrajectoryFileGenerator::generatePointName(int pointIndex) const {
	std::ostringstream oss;
	oss << "P" << std::setw(6) << std::setfill('0') << pointIndex;
	return oss.str();
}

// 写入文件头
void TrajectoryFileGenerator::writeFileHeader(std::ostream& outStream) const {
	outStream << padLineToWidth("//DIR;") << std::endl;
	outStream << padLineToWidth("///JOB;") << std::endl;
	outStream << padLineToWidth("NAME " + jobName_ + ";") << std::endl;
	outStream << padLineToWidth("///ROBOT;") << std::endl;
	outStream << padLineToWidth("ROBOTTYPE " + generateRobotTypeString() + ";") << std::endl;
	outStream << padLineToWidth("///POS;") << std::endl;
	outStream << padLineToWidth("NPOS " + nposParams_ + ";") << std::endl;
	outStream << padLineToWidth("POSTYPE " + std::to_string(postype_) + ";") << std::endl;
}

// 写入位置数据
void TrajectoryFileGenerator::writePositionData(std::ostream& outStream) const {

	outStream << padLineToWidth("//COORD;") << std::endl;
	for (const auto& point : positions_) {
		outStream << "P" << std::setw(6) << std::setfill('0') << point.index << "=";
		outStream << std::setw(2) << std::setfill('0') << robotJointsCount_ + externalAxesCount_;

		// 确保写入10个值，即使有些是0
		for (int i = 0; i < 10; ++i) {
			if (i < point.values.size()) {
				outStream << "," << formatPositionValue(point.values[i]);
			}
			else {
				outStream << ",00000.000000"; // 补充0值
			}
		}

		outStream << std::endl; // 注意：位置行末尾没有';'
	}
}

// 写入指令数据
void TrajectoryFileGenerator::writeInstructionData(std::ostream& outStream) const {
	outStream << padLineToWidth("///INSTRUCTION;") << std::endl;
	outStream << padLineToWidth(date_ + ";") << std::endl;
	outStream << padLineToWidth("VERSION " + version_ + ";") << std::endl;
	outStream << padLineToWidth("NOP;") << std::endl;
	for (const auto& instruction : instructions_) {
		outStream << padLineToWidth(instruction + ";") << std::endl;
	}
	outStream << padLineToWidth("END;") << std::endl;
}


// 写入完整内容
void TrajectoryFileGenerator::writeContent(std::ostream& stream) const {
	writeFileHeader(stream);
	writePositionData(stream);
	writeInstructionData(stream);
}