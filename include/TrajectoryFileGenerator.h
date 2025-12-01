#ifndef TRAJECTORYFILEGENERATOR_H
#define TRAJECTORYFILEGENERATOR_H

#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <comdef.h>
#include <atlbase.h>
#include <atlstr.h>

// 事件信息结构体
struct event_information {
	bool hasEvent;
	CComBSTR name;
	CComBSTR content;
	CComBSTR position;

	event_information(
		bool hasEvent_ = false,
		CComBSTR name_ = L"",
		CComBSTR content_ = L"",
		CComBSTR position_ = nullptr)
		: hasEvent(hasEvent_)
		, name(name_)
		, content(content_)
		, position(position_)
	{}
};

// 轨迹点信息结构体（移除了pointName字段）
struct TrajectoryPointInfo {
	int pointIndex;
	std::vector<double> robotJoints;
	std::vector<double> externalAxes;
	double velocity;
	int motionPercentage;
	std::string motionType;
	event_information event;

	TrajectoryPointInfo()
		: pointIndex(0), velocity(0.0), motionPercentage(100)
	{}

	TrajectoryPointInfo(int index)
		: pointIndex(index), velocity(0.0), motionPercentage(100)
	{}
};

class TrajectoryFileGenerator {
public:
	// 位置点结构体
	struct PositionPoint {
		int index;
		std::vector<double> values;
	};

	TrajectoryFileGenerator(const std::string& filename,
		const std::string& jobName);

	// 添加原始位置点
	void addPositionPoint(const PositionPoint& point);

	// 添加轨迹点信息
	void addTrajectoryPoint(const TrajectoryPointInfo& pointInfo);

	// 通用指令添加
	void addInstruction(const std::string& instruction);

	// 专用指令添加方法
	void addOUTInstruction(const std::string& signalName, bool state, int delayMs = 0);
	void addACTInstruction(const std::string& brushTableName);
	void addPTPInstruction(int axisNumber, double position, double velocity);
	void addMOVJInstruction(int pointIndex, int speedPercent);  // 修改：使用pointIndex代替pointName
	void addMOVDInstruction(int pointIndex);                    // 修改：使用pointIndex代替pointName
	void addDELAYInstruction(int delayMs);
	void addWAITInstruction(const std::string& variableName, bool expectedState);

	// 设置参数
	void setNposParams(const std::string& nposParams);
	void setVersion(const std::string& version);
	void setDate(const std::string& date);
	void setPostype(int postype);

	// 设置关节和外部轴数量
	void setRobotJointsCount(int count);
	void setExternalAxesCount(int count);

	// 自动计算NPOS参数
	void calculateAndSetNposParams();

	// 从命令行设置NPOS
	void setNposFromCommandLine(int argc, char* argv[]);
	void setNposFromCommandLine(const std::vector<std::string>& args);

	// 生成文件
	void generate();
	std::string generateToString();

	// 获取内部状态和配置
	size_t getPositionCount() const { return positions_.size(); }
	size_t getInstructionCount() const { return instructions_.size(); }
	std::string getCurrentNposParams() const { return nposParams_; }
	int getRobotJointsCount() const { return robotJointsCount_; }
	int getExternalAxesCount() const { return externalAxesCount_; }
	std::string getRobotType() const { return generateRobotTypeString(); }

	// 生成点名称（新增）
	std::string generatePointName(int pointIndex) const;

private:
	// 生成RobotType字符串
	std::string generateRobotTypeString() const;

	// 格式化位置值
	std::string formatPositionValue(double value) const;

	// 文件写入方法
	void writeFileHeader(std::ostream& outStream) const;
	void writePositionData(std::ostream& outStream) const;
	void writeInstructionData(std::ostream& outStream) const;
	void writeContent(std::ostream& stream) const;

	// 验证指令长度
	void validateInstructionLength(const std::string& instruction);

	// 从轨迹点信息创建位置点
	PositionPoint createPositionPointFromTrajectoryInfo(const TrajectoryPointInfo& pointInfo) const;

	// 从轨迹点信息创建运动指令
	std::string createMotionInstructionFromTrajectoryInfo(const TrajectoryPointInfo& pointInfo) const;

	// 创建事件指令
	std::vector<std::string> createEventInstructions(const event_information& event) const;

private:
	std::string filename_;
	std::string jobName_;
	std::string nposParams_;
	int postype_;
	std::string date_;
	std::string version_;

	std::vector<PositionPoint> positions_;
	std::vector<std::string> instructions_;
	std::vector<TrajectoryPointInfo> trajectoryPoints_;

	// 关节和外部轴数量配置
	int robotJointsCount_;
	int externalAxesCount_;
};

#endif // TRAJECTORYFILEGENERATOR_H