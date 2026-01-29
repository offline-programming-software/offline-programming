#ifndef SPACE_CALCULATE_H
#define SPACE_CALCULATE_H
#define NOMINMAX

#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <array>
#include <limits>
#include <Eigen/Dense>

#include "PQKitCallback.h"
#import "RPC.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types

// 空间点结构
struct spacePoint {
	double x, y, z;
	spacePoint(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

	spacePoint operator+(const spacePoint& other) const {
		return spacePoint(x + other.x, y + other.y, z + other.z);
	}

	spacePoint operator-(const spacePoint& other) const {
		return spacePoint(x - other.x, y - other.y, z - other.z);
	}

	spacePoint operator*(double scalar) const {
		return spacePoint(x * scalar, y * scalar, z * scalar);
	}

	spacePoint operator/(double scalar) const {
		return spacePoint(x / scalar, y / scalar, z / scalar);
	}

	bool operator==(const spacePoint& other) const {
		return std::abs(x - other.x) < 1e-9 &&
			std::abs(y - other.y) < 1e-9 &&
			std::abs(z - other.z) < 1e-9;
	}
};

struct RobotWorkspaceBoundary {

	ULONG robotID;
	double thickness;
	double theta;
	bool isLink;
	std::vector<QString> railName;
	std::vector<double> points;
};

struct workSpaceInformation {
	QString robotName;
	int number;
	QString coodinate;
	QString mainDir;
	bool isLink;
	QString railName;

	workSpaceInformation(const QString& rName = "",
		int num = 0,
		const QString& coord = "",
		const QString& mDir = "",
		bool m_isLink = 0,
		const QString& railName = "") :
		robotName(rName),
		number(num),
		coodinate(coord),
		mainDir(mDir),
		isLink(m_isLink),
		railName(railName)
	{}
};

// 局部坐标系类
class LocalCoordinateSystem {
private:
	Eigen::Matrix3d m_rotationMatrix; // 从局部坐标系到世界坐标系的旋转矩阵
	spacePoint m_origin; // 坐标系原点（世界坐标系下）

public:
	LocalCoordinateSystem(const spacePoint& origin, const std::vector<double>& direction) {
		m_origin = origin;

		if (direction.size() >= 3) {
			// 方向矢量作为局部坐标系的Z轴
			Eigen::Vector3d zAxis(direction[0], direction[1], direction[2]);
			zAxis.normalize();

			// 构造X轴（与Z轴垂直的任意方向）
			Eigen::Vector3d xAxis;
			if (std::abs(zAxis.dot(Eigen::Vector3d::UnitX())) < 0.9) {
				xAxis = zAxis.cross(Eigen::Vector3d::UnitX());
			}
			else {
				xAxis = zAxis.cross(Eigen::Vector3d::UnitY());
			}
			xAxis.normalize();

			// 构造Y轴（Z轴 × X轴）
			Eigen::Vector3d yAxis = zAxis.cross(xAxis);
			yAxis.normalize();

			// 构建旋转矩阵
			m_rotationMatrix.col(0) = xAxis;
			m_rotationMatrix.col(1) = yAxis;
			m_rotationMatrix.col(2) = zAxis;
		}
		else {
			// 默认使用世界坐标系
			m_rotationMatrix = Eigen::Matrix3d::Identity();
		}
	}

	// 世界坐标转局部坐标
	spacePoint worldToLocal(const spacePoint& worldPoint) const {
		Eigen::Vector3d worldVec(worldPoint.x - m_origin.x,
			worldPoint.y - m_origin.y,
			worldPoint.z - m_origin.z);
		Eigen::Vector3d localVec = m_rotationMatrix.transpose() * worldVec;
		return spacePoint(localVec.x(), localVec.y(), localVec.z());
	}

	// 局部坐标转世界坐标
	spacePoint localToWorld(const spacePoint& localPoint) const {
		Eigen::Vector3d localVec(localPoint.x, localPoint.y, localPoint.z);
		Eigen::Vector3d worldVec = m_rotationMatrix * localVec;
		return spacePoint(worldVec.x() + m_origin.x,
			worldVec.y() + m_origin.y,
			worldVec.z() + m_origin.z);
	}

	// 获取局部坐标系下的方向矢量
	spacePoint getLocalDirection(const std::string& direction) const {
		if (direction == "x+") return spacePoint(1, 0, 0);
		else if (direction == "x-") return spacePoint(-1, 0, 0);
		else if (direction == "y+") return spacePoint(0, 1, 0);
		else if (direction == "y-") return spacePoint(0, -1, 0);
		else if (direction == "z+") return spacePoint(0, 0, 1);
		else if (direction == "z-") return spacePoint(0, 0, -1);
		else return spacePoint(0, 0, 0);
	}

	// 获取世界坐标系下的方向矢量
	spacePoint getWorldDirection(const std::string& direction) const {
		spacePoint localDir = getLocalDirection(direction);
		Eigen::Vector3d localVec(localDir.x, localDir.y, localDir.z);
		Eigen::Vector3d worldVec = m_rotationMatrix * localVec;
		return spacePoint(worldVec.x(), worldVec.y(), worldVec.z());
	}
};

class Workspace {
private:
	spacePoint m_center;
	spacePoint m_size;
	CComPtr<IPQPlatformComponent> m_ptrKit;
	CPQKitCallback* m_ptrKitCallback;

	// 局部坐标系（基于方向矢量）
	std::shared_ptr<LocalCoordinateSystem> m_localCoordSystem;

	//将方向向量转化为四元数
	std::vector<double> directionVectorToQuaternion(const std::vector<double>& direction);

	// 可达性检测方法
	bool isPointReachable(ULONG robotID, const spacePoint& point,
		const std::vector<double>& direction, bool enforceOrientation);

	// 辅助函数：罗德里格斯旋转公式
	std::vector<double> rodriguesRotate(const std::vector<double>& v,
		const std::vector<double>& k, double theta);

	//创建八条母线方向向量
	std::vector<std::vector<double>> generateUniformGeneratrix(
		const std::vector<double>& coneAxis, double coneAngle);

	bool isPointReachableWithConeConstraint(ULONG robotID, const spacePoint& point,
		const std::vector<double>& coneAxis, double coneAngle,
		int numSamples = 8);

	// 检查面上所有采样点是否可达
	bool checkFacePointsReachability(ULONG robotID,
		const spacePoint& center,
		const spacePoint& size,
		double theta,
		const std::string& direction,
		int samplePoints,
		const std::vector<double>& coneDirection,
		const LocalCoordinateSystem& coordSystem);

	// 在面上生成均匀采样点
	std::vector<spacePoint> generateSamplePointsOnFace(const spacePoint& faceCenter,
		const spacePoint& size,
		const std::string& direction,
		int samplePoints,
		const LocalCoordinateSystem& coordSystem);

	// 更新工作空间尺寸和中心点
	void updateWorkspaceSizeAndCenter(spacePoint& center,
		spacePoint& size,
		const std::map<std::string, double>& expandDistances,
		const LocalCoordinateSystem& coordSystem);

	// 计算长方体的八个角点
	std::vector<spacePoint> calculateCornerPoints(const spacePoint& center,
		const spacePoint& size,
		const LocalCoordinateSystem& coordSystem);

	// 分析功能
	static std::vector<double> analyzeRobotSpaceCharacteristics(const std::vector<std::array<double, 3>>& robotSpace);

public:
	Workspace(const spacePoint& center, const spacePoint& size,
		CComPtr<IPQPlatformComponent> ptrKit, CPQKitCallback* ptrKitCallback,
		const std::vector<double>& direction = std::vector<double>())
		: m_center(center), m_size(size), m_ptrKit(ptrKit), m_ptrKitCallback(ptrKitCallback) {
		// 创建局部坐标系
		m_localCoordSystem = std::make_shared<LocalCoordinateSystem>(center, direction);
	}

	// 获取信息
	spacePoint getCenter() const { return m_center; }
	spacePoint getSize() const { return m_size; }
	double getVolume() const { return m_size.x * m_size.y * m_size.z; }

	// 基于局部坐标系的拓展方法
	Workspace expand(const std::string& direction, double step) const;
	Workspace expandInLocalCoord(const std::string& direction, double step,
		const std::vector<double>& localDirection) const;

	std::vector<spacePoint> getFacePoints(const std::string& direction, int n = 5) const;
	bool contains(const spacePoint& point) const;

	// 主要功能方法

	//1、利用蒙特卡洛法计算出可达空间中心点 (计算时将X_min设置为0)
	spacePoint calculateRobotWorkspaceCenter(ULONG robotID);

	//2、输入步长、中心点、主法矢方向、厚度，输出工作空间(八个角点)
	std::vector<spacePoint> calculateExpandableWorkspace(ULONG robotID,
		const spacePoint& center,
		double initialStepSize,
		double thickness,
		double theta,
		const std::vector<double>& direction,
		int samplePointsPerFace = 5,
		double minStepSize = 1.0);

	//3、输入起始角度、终止角度、起始厚度、终止厚度，中心点、步长、主法矢方向，输出map
	std::map<std::pair<double, double>, std::vector<spacePoint>> calculateRobotSpaceRange(ULONG robotID,
		const spacePoint& center,
		double initialStepSize,
		double startthick,
		double endthick,
		double starttheta,
		double endtheta,
		const std::vector<double>& direction,
		int samplePointsPerFace = 5,
		double minStepSize = 1.0);

	//4、输入机器人id、主法矢量、厚度，输出八个角点
	std::vector<spacePoint> RobotWorkspace(
		const std::map<std::pair<double, double>, std::vector<spacePoint>>& aabbMap,
		double targetThick, double targetTheta);

	Workspace calculateRailRobotWorkspace(
		ULONG robotID,
		const std::vector<double>& railDirection,
		double spraySpeed,
		double railSpeed,
		const spacePoint& initialCenter,
		double initialStepSize,
		double thickness,
		double theta,
		const std::vector<double>& direction,
		int samplePointsPerFace,
		double minStepSize);

	Workspace calculateMultiRailRobotWorkspace(
		ULONG robotID,
		const std::vector<std::vector<double>>& railDirections,
		const std::vector<double>& spraySpeeds,
		const std::vector<double>& railSpeeds,
		const spacePoint& initialCenter,
		double initialStepSize,
		double thickness,
		double theta,
		const std::vector<double>& direction,
		int samplePointsPerFace,
		double minStepSize);

	//5、中心点优化方法
	spacePoint optimizeCenterPoint(ULONG robotID,
		const spacePoint& initialCenter,
		const std::vector<double>& direction,
		double searchRadius = 100.0,
		int searchSteps = 10,
		int samplePoints = 10);
};

#endif // SPACE_CALCULATE_H