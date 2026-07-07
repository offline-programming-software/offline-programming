#include "trajectory_processor.h"
#include <fstream>
#include <stdexcept>
#include <cassert>

void QuinticPolynomial::ComputeCoefficients(double q0, double q1,
	double v0, double v1,
	double a0, double a1,
	double T) {
	if (T <= 0) throw std::invalid_argument("Time duration must be positive");

	Eigen::MatrixXd A(6, 6);
	Eigen::VectorXd B(6);

	A << 1, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0,
		1, T, T*T, T*T*T, T*T*T*T, T*T*T*T*T,
		0, 1, 2 * T, 3 * T*T, 4 * T*T*T, 5 * T*T*T*T,
		0, 0, 2, 6 * T, 12 * T*T, 20 * T*T*T;

	B << q0, v0, a0, q1, v1, a1;

	Eigen::VectorXd coeffs = A.colPivHouseholderQr().solve(B);

	a0_ = coeffs[0];
	a1_ = coeffs[1];
	a2_ = coeffs[2];
	a3_ = coeffs[3];
	a4_ = coeffs[4];
	a5_ = coeffs[5];
}

double QuinticPolynomial::Position(double t) const {
	return a0_ + a1_ * t + a2_ * t*t + a3_ * t*t*t + a4_ * t*t*t*t + a5_ * t*t*t*t*t;
}

double QuinticPolynomial::Velocity(double t) const {
	return a1_ + 2 * a2_*t + 3 * a3_*t*t + 4 * a4_*t*t*t + 5 * a5_*t*t*t*t;
}

double QuinticPolynomial::Acceleration(double t) const {
	return 2 * a2_ + 6 * a3_*t + 12 * a4_*t*t + 20 * a5_*t*t*t;
}

void TrajectoryGenerator::initstate(const std::vector<jointInformation>& jointInfos) {
	// 清空之前的数据
	robotJoints_.clear();
	pathVels_.clear();
	railJoints_.clear();
	railVels_.clear();
	railAccs_.clear();
	iInstruct_.clear();
	times_.clear();

	// 检查输入是否为空
	if (jointInfos.empty()) {
		throw std::invalid_argument("Joint information list cannot be empty");
	}

	// 从第一个点获取关节数量和导轨数量
	num_joints_ = jointInfos[0].robotJoint.size();
	num_rails_ = jointInfos[0].railPos.size();

	if (num_joints_ == 0) {
		throw std::invalid_argument("Robot joints cannot be empty");
	}

	// 存储数据并验证一致性
	for (const auto& info : jointInfos) {
		// 验证关节数量一致性
		if (info.robotJoint.size() != num_joints_) {
			throw std::invalid_argument("All robot joints must have the same number of joints");
		}

		// 验证导轨数量一致性
		if (info.railPos.size() != num_rails_) {
			throw std::invalid_argument("All rail positions must have the same number of rails");
		}

		if (info.railVel.size() != num_rails_) {
			throw std::invalid_argument("All rail velocities must have the same number of rails");
		}

		if (info.railAcc.size() != num_rails_) {
			throw std::invalid_argument("All rail accelerations must have the same number of rails");
		}

		robotJoints_.push_back(info.robotJoint);
		pathVels_.push_back(info.pathVel);
		railJoints_.push_back(info.railPos);
		railVels_.push_back(info.railVel);
		railAccs_.push_back(info.railAcc);
		iInstruct_.push_back(info.iInstruct);
		times_.push_back(info.time);
	}

	// 初始化角速度和角加速度向量
	joint_velocities_.resize(robotJoints_.size(), std::vector<double>(num_joints_, 0.0));
	joint_accelerations_.resize(robotJoints_.size(), std::vector<double>(num_joints_, 0.0));

	// 初始化多项式数组大小
	joints_.resize(num_joints_);
	rail_.resize(num_rails_);
}

void TrajectoryGenerator::computeJointVelocitiesAndAccelerations() {
	if (robotJoints_.size() < 2) return;

	// 计算角速度（使用中心差分法）
	for (size_t i = 0; i < robotJoints_.size(); i++) {
		if (i == 0) {
			// 第一个点使用前向差分
			double dt = times_[1] - times_[0];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_velocities_[i][j] = (robotJoints_[1][j] - robotJoints_[0][j]) / dt;
				}
			}
		}
		else if (i == robotJoints_.size() - 1) {
			// 最后一个点使用后向差分
			double dt = times_[i] - times_[i - 1];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_velocities_[i][j] = (robotJoints_[i][j] - robotJoints_[i - 1][j]) / dt;
				}
			}
		}
		else {
			// 中间点使用中心差分
			double dt = times_[i + 1] - times_[i - 1];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_velocities_[i][j] = (robotJoints_[i + 1][j] - robotJoints_[i - 1][j]) /  dt;
				}
			}
		}
	}

	// 计算角加速度（使用中心差分法）
	for (size_t i = 0; i < robotJoints_.size(); i++) {
		if (i == 0) {
			// 第一个点使用前向差分
			double dt = times_[1] - times_[0];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_accelerations_[i][j] = (joint_velocities_[1][j] - joint_velocities_[0][j]) / dt;
				}
			}
		}
		else if (i == robotJoints_.size() - 1) {
			// 最后一个点使用后向差分
			double dt = times_[i] - times_[i - 1];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_accelerations_[i][j] = (joint_velocities_[i][j] - joint_velocities_[i - 1][j]) / dt;
				}
			}
		}
		else {
			// 中间点使用中心差分
			double dt = times_[i + 1] - times_[i - 1];
			if (dt > 1e-6) {
				for (size_t j = 0; j < num_joints_; j++) {
					joint_accelerations_[i][j] = (joint_velocities_[i + 1][j] - joint_velocities_[i - 1][j]) / dt;
				}
			}
		}
	}
}

void TrajectoryGenerator::Generate(double dt) {
	// 计算角速度和角加速度
	computeJointVelocitiesAndAccelerations();

	// 清除之前的结果
	time_steps_.clear();
	positions_.clear();
	velocities_.clear();
	accelerations_.clear();
	rail_positions_.clear();
	rail_velocities_.clear();
	rail_accelerations_.clear();

	// 确保有足够的点生成轨迹
	if (times_.size() < 2) {
		throw std::invalid_argument("At least two points are required to generate trajectory");
	}

	// 检查时间序列是否单调递增
	for (size_t i = 1; i < times_.size(); i++) {
		if (times_[i] <= times_[i - 1]) {
			std::ostringstream oss;
			oss << "Non-increasing time at point " << i
				<< ": " << times_[i] << " <= " << times_[i - 1];
			throw std::invalid_argument(oss.str());
		}
	}

	for (size_t k = 1; k < times_.size(); k++) {
		double duration = times_[k] - times_[k - 1];

		// 计算所有关节的系数（使用五次多项式）
		for (size_t i = 0; i < num_joints_; ++i) 
		{
			joints_[i].ComputeCoefficients(
				robotJoints_[k - 1][i], robotJoints_[k][i],
				joint_velocities_[k - 1][i], joint_velocities_[k][i],
				joint_accelerations_[k - 1][i], joint_accelerations_[k][i],
				duration
			);
		}

		// 计算所有导轨的系数
		for (size_t rail_idx = 0; rail_idx < num_rails_; rail_idx++) 
		{
			rail_[rail_idx].ComputeCoefficients(
				railJoints_[k - 1][rail_idx], railJoints_[k][rail_idx],
				railVels_[k - 1][rail_idx], railVels_[k][rail_idx],
				railAccs_[k - 1][rail_idx], railAccs_[k][rail_idx],
				duration
			);
		}

		// 生成轨迹点
		// 判断机器人运动类型
		if (iInstruct_[k] == PQ_LINE)
		{
			for (double t_physical = times_[k - 1];
				t_physical <= times_[k] + 1e-6;  // 添加容差
				t_physical += dt)
			{
				double t_offset = t_physical - times_[k - 1];
				if (t_offset > duration) t_offset = duration;

				// 计算所有关节的位置、速度、加速度
				std::vector<double> pos(num_joints_), vel(num_joints_), acc(num_joints_);
				for (size_t j = 0; j < num_joints_; ++j) {
					pos[j] = joints_[j].Position(t_offset);
					vel[j] = joints_[j].Velocity(t_offset);
					acc[j] = joints_[j].Acceleration(t_offset);
				}

				// 计算所有导轨的位置、速度、加速度
				std::vector<double> rail_pos(num_rails_), rail_vel(num_rails_), rail_acc(num_rails_);
				for (size_t rail_idx = 0; rail_idx < num_rails_; rail_idx++) {
					rail_pos[rail_idx] = rail_[rail_idx].Position(t_offset);
					rail_vel[rail_idx] = rail_[rail_idx].Velocity(t_offset);
					rail_acc[rail_idx] = rail_[rail_idx].Acceleration(t_offset);
				}

				// 存储结果
				time_steps_.push_back(t_physical);
				positions_.push_back(pos);
				velocities_.push_back(vel);
				accelerations_.push_back(acc);
				rail_positions_.push_back(rail_pos);
				rail_velocities_.push_back(rail_vel);
				rail_accelerations_.push_back(rail_acc);
			}
		}
		else
		{
			double t_physical = times_[k] - times_[k - 1];
			//输入机器人的位置、速度、加速度
			std::vector<double> pos(num_joints_), vel(num_joints_), acc(num_joints_);
			pos = robotJoints_[k - 1];
			joint_velocities_[k - 1];
			joint_accelerations_[k - 1];
			
			//输入导轨的位置、速度、加速度
			std::vector<double> rail_pos(num_rails_), rail_vel(num_rails_), rail_acc(num_rails_);
			rail_pos = railJoints_[k - 1];
			rail_vel = railVels_[k - 1];
			rail_acc = railAccs_[k - 1];

			time_steps_.push_back(t_physical);
			positions_.push_back(pos);
			velocities_.push_back(vel);
			accelerations_.push_back(acc);
			rail_positions_.push_back(rail_pos);
			rail_velocities_.push_back(rail_vel);
			rail_accelerations_.push_back(rail_acc);
		}
	}

	// 不在 Generate 中自动导出，避免产生默认输出文件。
}

void TrajectoryGenerator::ExportCSV(const std::string& filename) const {
	std::ofstream file(filename);
	if (!file.is_open()) throw std::runtime_error("Cannot open file: " + filename);

	// 写入CSV头部（包含关节和导轨）
	file << "Time(s),";

	// 添加关节列头
	for (size_t j = 0; j < num_joints_; ++j) {
		file << "Joint" << j + 1 << "_Pos(rad),"
			<< "Joint" << j + 1 << "_Vel(rad/s),"
			<< "Joint" << j + 1 << "_Acc(rad/s²),";
	}

	// 添加导轨列头
	for (size_t rail_idx = 0; rail_idx < num_rails_; rail_idx++) {
		file << "Rail" << rail_idx + 1 << "_Pos(mm),"
			<< "Rail" << rail_idx + 1 << "_Vel(mm/s),"
			<< "Rail" << rail_idx + 1 << "_Acc(mm/s^2),";
	}

	// 移除最后一个逗号并换行
	file.seekp(-1, std::ios_base::cur);
	file << "\n";

	// 写入数据
	for (size_t i = 0; i < time_steps_.size(); ++i) {
		file << std::fixed << std::setprecision(6) << time_steps_[i] << ",";

		// 写入所有关节数据
		for (size_t j = 0; j < num_joints_; ++j) {
			file << positions_[i][j] << ","
				<< velocities_[i][j] << ","
				<< accelerations_[i][j] << ",";
		}

		// 写入所有导轨数据
		for (size_t rail_idx = 0; rail_idx < num_rails_; rail_idx++) {
			file << rail_positions_[i][rail_idx] << ","
				<< rail_velocities_[i][rail_idx] << ","
				<< rail_accelerations_[i][rail_idx] << ",";
		}

		// 移除最后一个逗号并换行
		file.seekp(-1, std::ios_base::cur);
		file << "\n";
	}

	file.close();
}