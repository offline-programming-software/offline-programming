#include "spaceCalculate.h"
#include <random>
#include <functional>
#include <memory>
#include <cmath>
#include <qmath.h>

// 随机数生成
double getRandomDouble(double min, double max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<double> dis(min, max);
	return dis(gen);
}

// 基于局部坐标系的拓展方法
Workspace Workspace::expand(const std::string& direction, double step) const {
	spacePoint newCenter = m_center;
	spacePoint newSize = m_size;

	// 获取世界坐标系下的拓展方向
	spacePoint worldDir = m_localCoordSystem->getWorldDirection(direction);

	// 计算新的中心点和尺寸
	if (direction == "x+" || direction == "x-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.x += step;
	}
	else if (direction == "y+" || direction == "y-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.y += step;
	}
	else if (direction == "z+" || direction == "z-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.z += step;
	}

	return Workspace(newCenter, newSize, m_ptrKit, m_ptrKitCallback);
}

// 基于指定局部坐标系的拓展方法
Workspace Workspace::expandInLocalCoord(const std::string& direction, double step,
	const std::vector<double>& localDirection) const {
	// 创建临时局部坐标系
	LocalCoordinateSystem tempCoordSystem(m_center, localDirection);

	spacePoint newCenter = m_center;
	spacePoint newSize = m_size;

	// 获取世界坐标系下的拓展方向
	spacePoint worldDir = tempCoordSystem.getWorldDirection(direction);

	// 计算新的中心点和尺寸
	if (direction == "x+" || direction == "x-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.x += step;
	}
	else if (direction == "y+" || direction == "y-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.y += step;
	}
	else if (direction == "z+" || direction == "z-") {
		newCenter.x += worldDir.x * step / 2;
		newCenter.y += worldDir.y * step / 2;
		newCenter.z += worldDir.z * step / 2;
		newSize.z += step;
	}

	return Workspace(newCenter, newSize, m_ptrKit, m_ptrKitCallback);
}

// 修改获取面点的方法，基于局部坐标系
std::vector<spacePoint> Workspace::getFacePoints(const std::string& direction, int n) const {
	std::vector<spacePoint> points;
	if (n < 2) n = 2;

	double dx = m_size.x / 2;
	double dy = m_size.y / 2;
	double dz = m_size.z / 2;

	// 获取世界坐标系下的方向
	spacePoint worldDir = m_localCoordSystem->getWorldDirection(direction);

	// 计算面的法向量和生成点
	if (direction == "x+" || direction == "x-") {
		// 使用局部坐标系的Y和Z轴来生成面
		spacePoint yAxis = m_localCoordSystem->getWorldDirection("y+");
		spacePoint zAxis = m_localCoordSystem->getWorldDirection("z+");

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				double yParam = -1.0 + 2.0 * i / (n - 1);
				double zParam = -1.0 + 2.0 * j / (n - 1);

				spacePoint facePoint = m_center;
				facePoint = facePoint + worldDir * dx; // X方向的面
				facePoint = facePoint + yAxis * (yParam * dy);
				facePoint = facePoint + zAxis * (zParam * dz);

				points.push_back(facePoint);
			}
		}
	}
	else if (direction == "y+" || direction == "y-") {
		// 使用局部坐标系的X和Z轴来生成面
		spacePoint xAxis = m_localCoordSystem->getWorldDirection("x+");
		spacePoint zAxis = m_localCoordSystem->getWorldDirection("z+");

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				double xParam = -1.0 + 2.0 * i / (n - 1);
				double zParam = -1.0 + 2.0 * j / (n - 1);

				spacePoint facePoint = m_center;
				facePoint = facePoint + worldDir * dy; // Y方向的面
				facePoint = facePoint + xAxis * (xParam * dx);
				facePoint = facePoint + zAxis * (zParam * dz);

				points.push_back(facePoint);
			}
		}
	}
	else if (direction == "z+" || direction == "z-") {
		// 使用局部坐标系的X和Y轴来生成面
		spacePoint xAxis = m_localCoordSystem->getWorldDirection("x+");
		spacePoint yAxis = m_localCoordSystem->getWorldDirection("y+");

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				double xParam = -1.0 + 2.0 * i / (n - 1);
				double yParam = -1.0 + 2.0 * j / (n - 1);

				spacePoint facePoint = m_center;
				facePoint = facePoint + worldDir * dz; // Z方向的面
				facePoint = facePoint + xAxis * (xParam * dx);
				facePoint = facePoint + yAxis * (yParam * dy);

				points.push_back(facePoint);
			}
		}
	}

	return points;
}

// 其他方法保持不变（但可以基于新的坐标系进行优化）
bool Workspace::contains(const spacePoint& point) const {
	// 将点转换到局部坐标系
	spacePoint localPoint = m_localCoordSystem->worldToLocal(point);
	spacePoint localCenter = m_localCoordSystem->worldToLocal(m_center);

	double dx = m_size.x / 2;
	double dy = m_size.y / 2;
	double dz = m_size.z / 2;

	return (localPoint.x >= localCenter.x - dx) && (localPoint.x <= localCenter.x + dx) &&
		(localPoint.y >= localCenter.y - dy) && (localPoint.y <= localCenter.y + dy) &&
		(localPoint.z >= localCenter.z - dz) && (localPoint.z <= localCenter.z + dz);
}

std::vector<spacePoint> Workspace::calculateExpandableWorkspace(ULONG robotID, const spacePoint & center,
	double initialStepSize, double thickness, double theta,
	const std::vector<double>& direction, int samplePointsPerFace, double minStepSize)
{
	std::vector<spacePoint> cornerPoints;

	// 初始化当前工作空间尺寸（小正方体）
	spacePoint currentSize(initialStepSize, initialStepSize, thickness);
	spacePoint currentCenter = center;

	// 仅四个扩展方向的扩展步长（x+, x-, y+, y-）
	std::map<std::string, double> stepSizes = {
		{"x+", initialStepSize}, {"x-", initialStepSize},
		{"y+", initialStepSize}, {"y-", initialStepSize}
	};

	// 仅四个扩展方向的扩展距离（x+, x-, y+, y-）
	std::map<std::string, double> expandDistances = {
		{"x+", 0}, {"x-", 0},
		{"y+", 0}, {"y-", 0}
	};

	// 创建临时局部坐标系
	LocalCoordinateSystem tempCoordSystem(center, direction);

	bool anyDirectionExpanding = true;
	int maxIterations = 100; // 防止无限循环
	int iteration = 0;

	while (anyDirectionExpanding && iteration < maxIterations) {
		iteration++;
		anyDirectionExpanding = false;

		// 四个扩展方向
		std::vector<std::string> directions = { "x+", "x-", "y+", "y-" };

		for (const auto& dir : directions) {
			double currentStep = stepSizes[dir];

			// 如果步长已经小于最小值，跳过该方向
			if (currentStep < minStepSize) {
				continue;
			}

			// 检查当前方向的面上的所有采样点是否可达
			bool allPointsReachable = checkFacePointsReachability(robotID, currentCenter,
				currentSize, theta, dir,
				samplePointsPerFace,
				direction, tempCoordSystem);

			if (allPointsReachable) {
				// 所有点可达，进行扩展
				expandDistances[dir] += currentStep;
				anyDirectionExpanding = true;
				// 下次继续使用当前步长
				stepSizes[dir] = currentStep;
			}
			else {
				// 有点不可达，步长减半
				stepSizes[dir] = currentStep / 2.0;
				// 如果步长仍然大于最小值，继续尝试
				if (stepSizes[dir] >= minStepSize) {
					anyDirectionExpanding = true;
				}
			}
		}

		// 更新当前工作空间尺寸和中心点（只更新xy方向，保持z方向为固定的thickness）
		updateWorkspaceSizeAndCenter(currentCenter, currentSize, expandDistances, tempCoordSystem);

		// 确保z轴尺寸保持为thickness（关键修复）
		currentSize.z = thickness;
	}

	// 计算八个角点
	cornerPoints = calculateCornerPoints(currentCenter, currentSize, tempCoordSystem);

	return cornerPoints;
}

spacePoint Workspace::calculateRobotWorkspaceCenter(ULONG robotID) {
	std::vector<spacePoint> reachablePoints;
	const int NUM_SAMPLES = 10000;

	// 获取关节限制
	int jointCount = 0;
	double* jointLimitsArray = nullptr;
	HRESULT hr = m_ptrKit->Doc_get_obj_links(robotID, &jointCount, &jointLimitsArray);

	if (FAILED(hr) || jointCount == 0 || jointLimitsArray == nullptr) {
		return spacePoint(0, 0, 0);
	}

	int actualJointCount = jointCount / 2;
	std::vector<std::pair<double, double>> jointLimits(actualJointCount);

	for (int i = 0; i < actualJointCount; ++i) {
		jointLimits[i] = { jointLimitsArray[2 * i], jointLimitsArray[2 * i + 1] };
	}
	m_ptrKit->PQAPIFreeArray((LONG_PTR*)jointLimitsArray);

	// 获取基坐标系
	ULONG baseCoordinateID = 0;
	m_ptrKit->Robot_get_base_coordinate(robotID, &baseCoordinateID);

	// 蒙特卡洛采样
	for (int i = 0; i < NUM_SAMPLES; ++i) {
		std::vector<double> jointAngles(actualJointCount);

		// 生成随机关节角度
		for (int j = 0; j < actualJointCount; ++j) {
			double range = jointLimits[j].second - jointLimits[j].first;
			jointAngles[j] = jointLimits[j].first + getRandomDouble(0, 1) * range;
			jointAngles[j] *= M_PI / 180.0; // 度转弧度
		}

		// 正向运动学
		int postureSize = 0;
		double* posture = nullptr;

		hr = m_ptrKit->Robot_get_forward_kinematics(robotID, jointAngles.data(), actualJointCount,
			QUATERNION, baseCoordinateID, 1, &postureSize, &posture);

		if (SUCCEEDED(hr) && posture != nullptr && postureSize >= 3) {
			reachablePoints.push_back(spacePoint(posture[0], posture[1], posture[2]));
		}

		if (posture != nullptr) {
			m_ptrKit->PQAPIFreeArray((LONG_PTR*)posture);
		}
	}

	if (reachablePoints.empty()) {
		return spacePoint(0, 0, 0);
	}

	// 计算边界框
	spacePoint minPoint = reachablePoints[0];
	spacePoint maxPoint = reachablePoints[0];

	for (const auto& point : reachablePoints) {
		minPoint.x = std::min(minPoint.x, point.x);
		minPoint.y = std::min(minPoint.y, point.y);
		minPoint.z = std::min(minPoint.z, point.z);
		maxPoint.x = std::max(maxPoint.x, point.x);
		maxPoint.y = std::max(maxPoint.y, point.y);
		maxPoint.z = std::max(maxPoint.z, point.z);
	}

	// 计算中心点
	return spacePoint((maxPoint.x) / 2.0,
		(minPoint.y + maxPoint.y) / 2.0,
		(minPoint.z + maxPoint.z) / 2.0);
}

bool Workspace::isPointReachable(ULONG robotID, const spacePoint& point,
	const std::vector<double>& direction, bool enforceOrientation) {
	try {
		// 7元素数组：[x, y, z, dw, dx, dy, dz] - dw是实部，dx,dy,dz是虚部
		double endPosture[7] = { point.x, point.y, point.z, 1.0, 0.0, 0.0, 0.0 }; // 默认单位四元数

		if (enforceOrientation && direction.size() >= 3) {
			// 将方向向量转换为四元数 [dw, dx, dy, dz] 格式
			std::vector<double> quaternion = directionVectorToQuaternion(direction);

			if (quaternion.size() == 4) {
				// 按照 [dw, dx, dy, dz] 顺序赋值
				endPosture[3] = quaternion[0]; // dw (实部)
				endPosture[4] = quaternion[1]; // dx (虚部x)
				endPosture[5] = quaternion[2]; // dy (虚部y) 
				endPosture[6] = quaternion[3]; // dz (虚部z)
			}
			else {
				// 转换失败，使用默认四元数 [1, 0, 0, 0] (无旋转)
				endPosture[3] = 1.0;
				endPosture[4] = 0.0;
				endPosture[5] = 0.0;
				endPosture[6] = 0.0;
			}
		}

		//机器人全部逆解
		double* i_pJointValues = NULL;
		INT o_nJointValuesCountl = 0;
		struct_PQ6RConfig * i_eAxisCfg = NULL;
		INT o_nAxisCfgCount = 0;
		HRESULT hr = m_ptrKit->Robot_Get_All_Inverse_Kinematics(robotID, endPosture, 7, QUATERNION,
			0, &i_pJointValues, &o_nJointValuesCountl, &i_eAxisCfg, &o_nAxisCfgCount);

		m_ptrKit->PQAPIFreeArray((LONG_PTR*)i_pJointValues);
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)i_eAxisCfg);

		return (SUCCEEDED(hr));
	}
	catch (...) {
		return false;
	}
}

// 方向向量转四元数 [dw, dx, dy, dz] 格式
std::vector<double> Workspace::directionVectorToQuaternion(const std::vector<double>& direction) {
	if (direction.size() < 3) {
		return { 1.0, 0.0, 0.0, 0.0 }; // 返回单位四元数 [1, 0, 0, 0]
	}

	// 归一化方向向量（作为Z轴）
	Eigen::Vector3d z_axis(direction[0], direction[1], direction[2]);
	z_axis.normalize();

	// 默认上向量（世界坐标系Z轴）
	Eigen::Vector3d up_vector(0, 0, 1);

	// 如果方向向量接近上向量，选择不同的参考
	if (z_axis.dot(up_vector) > 0.99) {
		up_vector = Eigen::Vector3d(0, 1, 0);
	}

	// 构建坐标系
	Eigen::Vector3d x_axis = up_vector.cross(z_axis);
	x_axis.normalize();
	Eigen::Vector3d y_axis = z_axis.cross(x_axis);
	y_axis.normalize();

	// 构建旋转矩阵
	Eigen::Matrix3d rotation_matrix;
	rotation_matrix.col(0) = x_axis;
	rotation_matrix.col(1) = y_axis;
	rotation_matrix.col(2) = z_axis;

	// 旋转矩阵转四元数 [w, x, y, z] 格式
	Eigen::Quaterniond quat(rotation_matrix);
	quat.normalize();

	// 返回 [dw, dx, dy, dz] 格式
	return { quat.w(), quat.x(), quat.y(), quat.z() };
}



std::vector<double> Workspace::rodriguesRotate(
	const std::vector<double>& v,      // 待旋转向量
	const std::vector<double>& k,      // 旋转轴（单位向量）
	double theta)                      // 旋转角度（弧度）
{
	double cosTheta = std::cos(theta);
	double sinTheta = std::sin(theta);

	// 旋转轴归一化
	double kNorm = std::sqrt(k[0] * k[0] + k[1] * k[1] + k[2] * k[2]);
	std::vector<double> axis = { k[0] / kNorm, k[1] / kNorm, k[2] / kNorm };

	// 计算叉积和点积
	std::vector<double> cross = {
		axis[1] * v[2] - axis[2] * v[1],
		axis[2] * v[0] - axis[0] * v[2],
		axis[0] * v[1] - axis[1] * v[0]
	};
	double dot = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];

	// 应用罗德里格斯公式
	return {
		v[0] * cosTheta + cross[0] * sinTheta + axis[0] * dot*(1 - cosTheta),
		v[1] * cosTheta + cross[1] * sinTheta + axis[1] * dot*(1 - cosTheta),
		v[2] * cosTheta + cross[2] * sinTheta + axis[2] * dot*(1 - cosTheta)
	};
}

std::vector<std::vector<double>> Workspace::generateUniformGeneratrix(
	const std::vector<double>& coneAxis,
	double coneAngle)
{
	// 归一化圆锥轴
	double axisNorm = std::sqrt(coneAxis[0] * coneAxis[0] + coneAxis[1] * coneAxis[1] + coneAxis[2] * coneAxis[2]);
	if (axisNorm < 1e-9) return {}; // 无效轴向量

	std::vector<double> axis = { coneAxis[0] / axisNorm, coneAxis[1] / axisNorm, coneAxis[2] / axisNorm };

	// 生成8个均匀分布的方位角（0°, 45°, ..., 315°）
	std::vector<double> azimuths;
	for (int i = 0; i < 8; ++i) {
		azimuths.push_back(2 * M_PI * i / 8);
	}

	std::vector<std::vector<double>> generatrices;
	for (double azimuth : azimuths) {
		// 生成局部坐标系下的母线方向（极角θ=coneAngle, φ=azimuth）
		double theta = coneAngle;  // 圆锥半角
		double phi = azimuth;      // 方位角

		// 球坐标转局部笛卡尔坐标（默认Z轴为圆锥轴）
		double x = std::sin(theta) * std::cos(phi);
		double y = std::sin(theta) * std::sin(phi);
		double z = std::cos(theta);
		std::vector<double> localDir = { x, y, z };

		// 将局部方向旋转到全局圆锥轴方向
		std::vector<double> globalDir = rodriguesRotate(localDir, axis, M_PI / 2); // 旋转90°使局部Z轴对齐全局coneAxis

		generatrices.push_back(globalDir);
	}

	return generatrices;
}

bool Workspace::isPointReachableWithConeConstraint(
	ULONG robotID,
	const spacePoint& point,
	const std::vector<double>& coneAxis,
	double coneAngle,
	int numSamples)
{
	// 退化情况处理
	if (coneAxis.size() < 3 || coneAngle <= 0) {
		return isPointReachable(robotID, point, std::vector<double>(), false);
	}

	// 生成八条均匀母线方向
	std::vector<std::vector<double>> generatrices = generateUniformGeneratrix(coneAxis, coneAngle);

	// 验证每条母线方向
	for (const auto& dir : generatrices) {
		if (isPointReachable(robotID, point, dir, true)) {
			return true;
		}
	}

	return false;
}

bool Workspace::checkFacePointsReachability(ULONG robotID, const spacePoint & center,
	const spacePoint & size, double theta, const std::string & direction, int samplePoints,
	const std::vector<double>& coneDirection,
	const LocalCoordinateSystem & coordSystem)
{
	// 获取方向在世界坐标系中的向量
	spacePoint worldDir = coordSystem.getWorldDirection(direction);

	// 计算面的中心点
	spacePoint faceCenter = center;
	double offset = 0;

	if (direction == "x+" || direction == "x-") {
		offset = size.x / 2;
		faceCenter = faceCenter + worldDir * offset;
	}
	else if (direction == "y+" || direction == "y-") {
		offset = size.y / 2;
		faceCenter = faceCenter + worldDir * offset;
	}
	else if (direction == "z+" || direction == "z-") {
		offset = size.z / 2;
		faceCenter = faceCenter + worldDir * offset;
	}

	// 在面上均匀采样
	std::vector<spacePoint> samplePointsOnFace = generateSamplePointsOnFace(faceCenter, size,
		direction, samplePoints, coordSystem);

	// 检查每个采样点是否可达
	for (const auto& point : samplePointsOnFace) {
		bool reachable = false;

		// 修复：现在当coneDirection大小为3时，也使用圆锥约束检查
		// 我们将coneDirection视为圆锥轴向量，theta作为圆锥半角
		if (coneDirection.size() >= 3 && theta > 0) {
			// 使用圆锥约束进行可达性检查
			reachable = isPointReachableWithConeConstraint(robotID, point, coneDirection, theta);
		}
		else {
			// 使用固定方向进行可达性检查
			reachable = isPointReachable(robotID, point, coneDirection, true);
		}

		if (!reachable) {
			return false;
		}
	}

	return true;
}

std::vector<spacePoint> Workspace::generateSamplePointsOnFace(const spacePoint & faceCenter,
	const spacePoint & size, const std::string & direction, int samplePoints,
	const LocalCoordinateSystem & coordSystem)
{
	std::vector<spacePoint> points;

	if (samplePoints < 1) samplePoints = 1;

	// 获取局部坐标系的三个轴向量
	spacePoint xAxis = coordSystem.getWorldDirection("x+");
	spacePoint yAxis = coordSystem.getWorldDirection("y+");
	spacePoint zAxis = coordSystem.getWorldDirection("z+");

	// 根据面的方向确定采样平面的两个轴
	spacePoint uAxis, vAxis;
	double uSize, vSize;

	if (direction == "x+" || direction == "x-") {
		// X方向的面，采样在YZ平面上
		uAxis = yAxis;
		vAxis = zAxis;
		uSize = size.y / 2.0;
		vSize = size.z / 2.0;
	}
	else if (direction == "y+" || direction == "y-") {
		// Y方向的面，采样在XZ平面上
		uAxis = xAxis;
		vAxis = zAxis;
		uSize = size.x / 2.0;
		vSize = size.z / 2.0;
	}
	else if (direction == "z+" || direction == "z-") {
		// Z方向的面，采样在XY平面上
		uAxis = xAxis;
		vAxis = yAxis;
		uSize = size.x / 2.0;
		vSize = size.y / 2.0;
	}

	// 生成采样点网格
	for (int i = 0; i < samplePoints; ++i) {
		for (int j = 0; j < samplePoints; ++j) {
			// 计算参数坐标，范围从-1到1
			double uParam = -1.0 + 2.0 * i / std::max(1, samplePoints - 1);
			double vParam = -1.0 + 2.0 * j / std::max(1, samplePoints - 1);

			// 计算实际的偏移量
			spacePoint offset = uAxis * (uParam * uSize) + vAxis * (vParam * vSize);

			// 计算采样点位置
			spacePoint samplePoint = faceCenter + offset;
			points.push_back(samplePoint);
		}
	}

	return points;
}

void Workspace::updateWorkspaceSizeAndCenter(spacePoint & center, spacePoint & size,
	const std::map<std::string, double>& expandDistances, const LocalCoordinateSystem & coordSystem)
{
	// 仅更新x和y方向的尺寸，保持z方向尺寸不变
	double newSizeX = expandDistances.count("x+") ? expandDistances.at("x+") + expandDistances.at("x-") : size.x;
	double newSizeY = expandDistances.count("y+") ? expandDistances.at("y+") + expandDistances.at("y-") : size.y;
	// z轴尺寸保持不变，由调用者在函数外设置

	// 计算新的中心点偏移（仅考虑x和y方向）
	spacePoint xpDir = coordSystem.getWorldDirection("x+");
	spacePoint ypDir = coordSystem.getWorldDirection("y+");

	spacePoint offset;
	if (expandDistances.count("x+")) {
		offset = offset + xpDir * (expandDistances.at("x+") - expandDistances.at("x-")) / 2;
	}
	if (expandDistances.count("y+")) {
		offset = offset + ypDir * (expandDistances.at("y+") - expandDistances.at("y-")) / 2;
	}

	// 更新中心点和尺寸
	center = center + offset;
	size.x = newSizeX;
	size.y = newSizeY;
	// 注意：size.z 保持不变，由调用者控制
}

std::vector<spacePoint> Workspace::calculateCornerPoints(const spacePoint & center,
	const spacePoint & size, const LocalCoordinateSystem & coordSystem)
{
	std::vector<spacePoint> corners;

	// 获取局部坐标系的轴方向
	spacePoint xAxis = coordSystem.getWorldDirection("x+");
	spacePoint yAxis = coordSystem.getWorldDirection("y+");
	spacePoint zAxis = coordSystem.getWorldDirection("z+");

	double dx = size.x / 2;
	double dy = size.y / 2;
	double dz = size.z / 2;

	// 计算八个角点
	std::vector<double> xSigns = { -1, 1 };
	std::vector<double> ySigns = { -1, 1 };
	std::vector<double> zSigns = { -1, 1 };

	for (double xSign : xSigns) {
		for (double ySign : ySigns) {
			for (double zSign : zSigns) {
				spacePoint corner = center;
				corner = corner + xAxis * (xSign * dx);
				corner = corner + yAxis * (ySign * dy);
				corner = corner + zAxis * (zSign * dz);
				corners.push_back(corner);
			}
		}
	}

	return corners;
}

std::map<std::pair<double, double>, std::vector<spacePoint>> Workspace::calculateRobotSpaceRange(
	ULONG robotID,
	const spacePoint& center,
	double initialStepSize,
	double startThick,
	double endThick,
	double startTheta,
	double endTheta,
	const std::vector<double>& baseDirection,
	int samplePointsPerFace,
	double minStepSize)
{
	// 参数校验
	if (startThick > endThick || startTheta > endTheta || baseDirection.size() != 3) {
		throw std::invalid_argument("Invalid input parameters");
	}

	// 归一化基础方向向量
	Eigen::Vector3d direction = Eigen::Map<const Eigen::Vector3d>(baseDirection.data()).normalized();

	std::map<std::pair<double, double>, std::vector<spacePoint>> spaceMap;

	// 遍历厚度参数（步长100）
	for (double thick = startThick; thick <= endThick; thick += 100) {
		// 计算角度步长（保持精度）
		double thetaStep = (endTheta - startTheta) / samplePointsPerFace;

		// 遍历角度参数
		for (int i = 0; i < samplePointsPerFace; ++i) {
			double currentTheta = startTheta + i * thetaStep;

			// 计算当前参数下的工作空间
			std::vector<spacePoint> aabb = calculateExpandableWorkspace(
				robotID,
				center,
				initialStepSize,
				thick,
				currentTheta,
				baseDirection,
				samplePointsPerFace,
				minStepSize
			);

			// 存储结果（使用规范化参数键）
			spaceMap[std::make_pair(thick, currentTheta)] = aabb;
		}
	}

	return spaceMap;
}

std::vector<spacePoint> Workspace::RobotWorkspace(const std::map<std::pair<double, double>, std::vector<spacePoint>>& aabbMap, double targetThick, double targetTheta)
{
	// 参数有效性检查（C++14兼容）
	if (aabbMap.empty()) {
		return {};
	}

	// 初始化最佳匹配变量
	std::pair<double, double> bestMatch;
	double minDistance = std::numeric_limits<double>::max();

	// 使用传统迭代器遍历（C++11/14兼容）
	for (auto it = aabbMap.begin(); it != aabbMap.end(); ++it) {
		const auto& key = it->first;
		const auto& aabb = it->second;

		// 计算曼哈顿距离
		double thickDiff = std::abs(key.first - targetThick);
		double thetaDiff = std::abs(key.second - targetTheta);
		double currentDistance = thickDiff + thetaDiff;

		// 更新最佳匹配
		if (currentDistance < minDistance) {
			minDistance = currentDistance;
			bestMatch = key;
		}
	}

	// 查找最佳匹配项
	auto foundIt = aabbMap.find(bestMatch);
	if (foundIt == aabbMap.end()) {
		return {};  // 未找到匹配项
	}

	// 坐标向上取整处理
	const auto& aabb = foundIt->second;
	std::vector<spacePoint> roundedAABB;
	roundedAABB.reserve(aabb.size());  // 预分配内存（C++11特性）

	for (const auto& pt : aabb) {
		roundedAABB.push_back(spacePoint{
			std::ceil(pt.x),
			std::ceil(pt.y),
			std::ceil(pt.z)
			});
	}

	return roundedAABB;
}

std::vector<double> Workspace::analyzeRobotSpaceCharacteristics(const std::vector<std::array<double, 3>>& robotSpace) {
	if (robotSpace.empty()) {
		return std::vector<double>();
	}

	std::vector<double> characteristics;

	// 计算统计特征
	double minX = robotSpace[0][0], maxX = robotSpace[0][0];
	double minY = robotSpace[0][1], maxY = robotSpace[0][1];
	double minZ = robotSpace[0][2], maxZ = robotSpace[0][2];

	for (const auto& point : robotSpace) {
		minX = std::min(minX, point[0]);
		maxX = std::max(maxX, point[0]);
		minY = std::min(minY, point[1]);
		maxY = std::max(maxY, point[1]);
		minZ = std::min(minZ, point[2]);
		maxZ = std::max(maxZ, point[2]);
	}

	characteristics.push_back(maxX - minX); // x方向范围
	characteristics.push_back(maxY - minY); // y方向范围
	characteristics.push_back(maxZ - minZ); // z方向范围
	characteristics.push_back((maxX - minX) * (maxY - minY) * (maxZ - minZ)); // 体积估计

	return characteristics;
}

// 中心点优化方法
spacePoint Workspace::optimizeCenterPoint(ULONG robotID,
	const spacePoint& initialCenter,
	const std::vector<double>& direction,
	double searchRadius,
	int searchSteps,
	int samplePoints)
{
	// 检查初始中心点是否可达
	if (isPointReachable(robotID, initialCenter, direction, true)) {
		// 如果初始中心点可达，返回原中心点
		return initialCenter;
	}

	// 定义搜索步长
	double stepSize = searchRadius / searchSteps;

	// 在初始中心点周围搜索可达的中心点
	for (int xStep = -searchSteps; xStep <= searchSteps; xStep++) {
		for (int yStep = -searchSteps; yStep <= searchSteps; yStep++) {
			for (int zStep = -searchSteps; zStep <= searchSteps; zStep++) {
				// 计算新的中心点
				spacePoint newCenter(
					initialCenter.x + xStep * stepSize,
					initialCenter.y + yStep * stepSize,
					initialCenter.z + zStep * stepSize
				);

				// 检查新中心点是否可达
				if (isPointReachable(robotID, newCenter, direction, true)) {
					// 进一步验证该中心点的可达性
					// 检查工作空间的角点是否可达
					Workspace tempWorkspace(newCenter, m_size, m_ptrKit, m_ptrKitCallback, direction);
					std::vector<spacePoint> cornerPoints = tempWorkspace.calculateCornerPoints(
						newCenter, m_size, LocalCoordinateSystem(newCenter, direction));

					bool allCornersReachable = true;
					for (const auto& corner : cornerPoints) {
						if (!isPointReachable(robotID, corner, direction, true)) {
							allCornersReachable = false;
							break;
						}
					}

					if (allCornersReachable) {
						// 找到一个合适的中心点，返回
						return newCenter;
					}
				}
			}
		}
	}

	// 如果没有找到完全可达的中心点，返回最接近可达的点
	// 尝试找到一个接近初始点的中心点
	double minDistance = std::numeric_limits<double>::max();
	spacePoint bestCenter = initialCenter;

	for (int xStep = -searchSteps; xStep <= searchSteps; xStep++) {
		for (int yStep = -searchSteps; yStep <= searchSteps; yStep++) {
			for (int zStep = -searchSteps; zStep <= searchSteps; zStep++) {
				// 计算新的中心点
				spacePoint newCenter(
					initialCenter.x + xStep * stepSize,
					initialCenter.y + yStep * stepSize,
					initialCenter.z + zStep * stepSize
				);

				// 检查新中心点是否可达
				if (isPointReachable(robotID, newCenter, direction, true)) {
					// 计算与初始中心点的距离
					double distance = sqrt(
						pow(newCenter.x - initialCenter.x, 2) +
						pow(newCenter.y - initialCenter.y, 2) +
						pow(newCenter.z - initialCenter.z, 2)
					);

					if (distance < minDistance) {
						minDistance = distance;
						bestCenter = newCenter;
					}
				}
			}
		}
	}

	return bestCenter;
}
