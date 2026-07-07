#pragma once
#ifndef TRAJECTORY_GENERATOR_H
#define TRAJECTORY_GENERATOR_H


#include <Eigen/Dense>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include "PQKitCallback.h"
#include "CommonStructs.h"

class QuinticPolynomial {
public:
	void ComputeCoefficients(double q0, double q1,
		double v0, double v1,
		double a0, double a1,
		double T);

	double Position(double t) const;
	double Velocity(double t) const;
	double Acceleration(double t) const;

private:
	double a0_ = 0, a1_ = 0, a2_ = 0, a3_ = 0, a4_ = 0, a5_ = 0;
};

class TrajectoryGenerator {
public:
	// 默认构造函数
	TrajectoryGenerator() = default;

	// 初始化状态
	void initstate(const std::vector<jointInformation>& jointInfos);

	// 计算关节速度和加速度
	void computeJointVelocitiesAndAccelerations();

	// 生成轨迹
	void Generate(double dt = 0.01);

	// 获取位置数组
	std::vector<std::vector<double>> get_positions() const {
		return positions_;
	}

	// 获取轨道位置数组
	std::vector<std::vector<double>> get_railpos() const {
		return rail_positions_;
	}

	// 获取轨道速度数组
	std::vector<std::vector<double>> get_railvel() const {
		return rail_velocities_;
	}

	// 获取轨道加速度数组
	std::vector<std::vector<double>> get_railacc() const {
		return rail_accelerations_;
	}

	// 获取时间步长
	std::vector<double> get_timesteps() const {
		return time_steps_;
	}

	// 获取速度数组
	std::vector<std::vector<double>> get_velocities() const {
		return velocities_;
	}

	// 获取加速度数组
	std::vector<std::vector<double>> get_accelerations() const {
		return accelerations_;
	}

	// 获取关节数量
	size_t getNumJoints() const {
		return num_joints_;
	}

	// 获取轨道数量
	size_t getNumRails() const {
		return num_rails_;
	}

	// 导出CSV文件
	void ExportCSV(const std::string& filename) const;

private:
	std::vector<QuinticPolynomial> joints_;
	std::vector<QuinticPolynomial> rail_;
	std::vector<double> time_steps_;
	std::vector<std::vector<double>> positions_;
	std::vector<std::vector<double>> velocities_;
	std::vector<std::vector<double>> accelerations_;

	std::vector<std::vector<double>> rail_positions_;
	std::vector<std::vector<double>> rail_velocities_;
	std::vector<std::vector<double>> rail_accelerations_;

	// 存储关节信息
	std::vector<RobotJoint> robotJoints_;
	std::vector<double> pathVels_;
	std::vector<RailJoint> railJoints_;
	std::vector<RailVel> railVels_;
	std::vector<RailAcc> railAccs_;
	std::vector<PQPointInstruction> iInstruct_;
	std::vector<double> times_;

	// 存储关节速度和加速度
	std::vector<std::vector<double>> joint_velocities_;
	std::vector<std::vector<double>> joint_accelerations_;

	// 关节总数和轨道总数
	size_t num_joints_ = 0;
	size_t num_rails_ = 0;
};

#endif // TRAJECTORY_GENERATOR_H