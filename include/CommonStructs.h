#pragma once
#ifndef COMMONSTRUCTS_H// 确保宏名称唯一
#define COMMONSTRUCTS_H
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include <atlstr.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr double DEG2RAD = M_PI / 180.0;
constexpr double RAD2DEG = 180.0 / M_PI;
constexpr double MM2M = 0.001;
constexpr double M2MM = 1000.0;

// 导轨加速度类型枚举
enum RailAccelType {
	ACCEL_TO_MAX = 1,     // 导轨以最大加速度加速到额定速率
	CONSTANT_SPEED = 0,   // 导轨速度保持不变
	DECEL_TO_ZERO = -1    // 导轨以最大加速度减速至零
};

// 机器人关节参数结构体
struct JointParameters {
	double minAngle;        // 关节最小角度（弧度）
	double maxAngle;        // 关节最大角度（弧度）
	double maxVelocity;     // 关节最大速度（rad/s）
	double maxAcceleration; // 关节最大加速度（rad/s²）
};

// DH参数结构体
struct DHParameters {
	double a;      // 连杆长度（毫米）
	double alpha;  // 连杆扭角（弧度）
	double d;      // 连杆偏移（毫米）
	double theta;  // 关节角度（弧度）
};

// 导轨参数结构体
struct RailParameters {
	std::array<double, 3> direction; // 导轨方向向量（单位向量）
	double minPosition;              // 最小位置（毫米）
	double maxPosition;              // 最大位置（毫米）
	double maxVelocity;              // 最大速度（mm/s）
	double maxAcceleration;          // 最大加速度（mm/s²）
};

using Pose = std::array<double, 7>;       // 固定大小的位姿 [x,y,z,qw,qx,qy,qz]
using RobotJoint = std::vector<double>;   // 动态关节数组
using RailJoint = std::vector<double>;    // 动态轨道数组
using RailVel = std::vector<double>;	  // 动态导轨速度
using RailAcc = std::vector<double>;	  // 动态导轨加速度
using RailTypes = std::vector<int>;		  // 动态导轨加速度类型
using Vector3 = std::array<double, 3>;    // 固定大小的3D向量

struct RobotPoint {
	double time;            // 时间戳
	int move_type;          // 运动类型 (1: move L, 2: move C)
	double velocity;        // 机器人末端速度大小 (mm/s)
	Pose posture;           // 位置信息[x,y,z,qw,qx,qy,qz]
	RobotJoint robotJoints; // 关节角度 
	RailJoint  railJoints;  // 导轨位置
	RailVel    railVel;     // 导轨速度
	RailAccelType railAccelType = CONSTANT_SPEED;
};

struct jointInformation {
	RobotJoint	robotJoint;
	double		pathVel;
	RailJoint	railPos;
	RailVel		railVel;
	RailAcc		railAcc;
	double		time;
};

// 机器人运动信息结构体
// struct Joint_information {
// 	std::array<double, 6> angles;
// 	std::array<double, 6> velocities;
// 	std::array<double, 6> accelerations;
// 	std::vector<double> rail_p;
// 	std::vector<double> rail_v;
// 	std::vector<double> rail_a;
// 	double time;
// };

struct end_information {
	std::array<double, 7> pos;
	std::array<double, 3> vel;
	std::array<double, 3> acc;
	double rail_a;
};

#endif