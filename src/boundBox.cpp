#include "boundBox.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

// Point3D 实现
Point3D::Point3D(double x, double y, double z) : x(x), y(y), z(z) {}

Point3D Point3D::operator+(const Point3D& other) const {
	return Point3D(x + other.x, y + other.y, z + other.z);
}

Point3D Point3D::operator-(const Point3D& other) const {
	return Point3D(x - other.x, y - other.y, z - other.z);
}

// 添加标量乘法运算符
Point3D Point3D::operator*(double scalar) const {
	return Point3D(x * scalar, y * scalar, z * scalar);
}

double Point3D::dot(const Point3D& other) const {
	return x * other.x + y * other.y + z * other.z;
}

Point3D Point3D::cross(const Point3D& other) const {
	return Point3D(
		y * other.z - z * other.y,
		z * other.x - x * other.z,
		x * other.y - y * other.x
	);
}

double Point3D::length() const {
	return std::sqrt(x * x + y * y + z * z);
}

Point3D Point3D::normalize() const {
	double len = length();
	if (len > 0) {
		return Point3D(x / len, y / len, z / len);
	}
	return *this;
}

// 非成员函数：标量乘以点（支持交换律）
Point3D operator*(double scalar, const Point3D& point) {
	return Point3D(scalar * point.x, scalar * point.y, scalar * point.z);
}

// AABB 方法实现
std::vector<Point3D> AABB::getCorners() const {
	std::vector<Point3D> corners(8);

	corners[0] = Point3D(minPoint.x, minPoint.y, minPoint.z); // 前左下
	corners[1] = Point3D(maxPoint.x, minPoint.y, minPoint.z); // 前右下
	corners[2] = Point3D(maxPoint.x, maxPoint.y, minPoint.z); // 前右上
	corners[3] = Point3D(minPoint.x, maxPoint.y, minPoint.z); // 前左上

	corners[4] = Point3D(minPoint.x, minPoint.y, maxPoint.z); // 后左下
	corners[5] = Point3D(maxPoint.x, minPoint.y, maxPoint.z); // 后右下
	corners[6] = Point3D(maxPoint.x, maxPoint.y, maxPoint.z); // 后右上
	corners[7] = Point3D(minPoint.x, maxPoint.y, maxPoint.z); // 后左上

	return corners;
}

// 合并另一个AABB到当前AABB
void AABB::merge(const AABB& other) {
	minPoint.x = std::min(minPoint.x, other.minPoint.x);
	minPoint.y = std::min(minPoint.y, other.minPoint.y);
	minPoint.z = std::min(minPoint.z, other.minPoint.z);

	maxPoint.x = std::max(maxPoint.x, other.maxPoint.x);
	maxPoint.y = std::max(maxPoint.y, other.maxPoint.y);
	maxPoint.z = std::max(maxPoint.z, other.maxPoint.z);
}

// 从8个角点合并包围盒
void AABB::mergeFromVertices(const std::vector<double>& vertices) {
	if (vertices.size() < 24) return; // 至少需要8个点

	for (int i = 0; i < 24; i += 3) {
		minPoint.x = std::min(minPoint.x, vertices[i]);
		minPoint.y = std::min(minPoint.y, vertices[i + 1]);
		minPoint.z = std::min(minPoint.z, vertices[i + 2]);

		maxPoint.x = std::max(maxPoint.x, vertices[i]);
		maxPoint.y = std::max(maxPoint.y, vertices[i + 1]);
		maxPoint.z = std::max(maxPoint.z, vertices[i + 2]);
	}
}

double AABB::volume() const {
	double dx = maxPoint.x - minPoint.x;
	double dy = maxPoint.y - minPoint.y;
	double dz = maxPoint.z - minPoint.z;
	return dx * dy * dz;
}

// OBB 方法实现（修复后的版本）
std::vector<Point3D> OBB::getCorners() const {
	std::vector<Point3D> corners(8);

	// 计算八个角点，使用标量乘法
	for (int i = 0; i < 8; ++i) {
		double signX = (i & 1) ? 1.0 : -1.0;
		double signY = (i & 2) ? 1.0 : -1.0;
		double signZ = (i & 4) ? 1.0 : -1.0;

		Point3D corner = center;
		corner = corner + (axes[0] * (signX * halfExtents.x));
		corner = corner + (axes[1] * (signY * halfExtents.y));
		corner = corner + (axes[2] * (signZ * halfExtents.z));
		corners[i] = corner;
	}

	return corners;
}

double OBB::volume() const {
	return 8.0 * halfExtents.x * halfExtents.y * halfExtents.z;
}

// AABB 计算函数实现
AABB calculateAABB(const std::vector<Point3D>& points) {
	if (points.empty()) {
		return AABB();
	}

	double minX = std::numeric_limits<double>::max();
	double minY = std::numeric_limits<double>::max();
	double minZ = std::numeric_limits<double>::max();
	double maxX = std::numeric_limits<double>::lowest();
	double maxY = std::numeric_limits<double>::lowest();
	double maxZ = std::numeric_limits<double>::lowest();

	for (const auto& point : points) {
		minX = std::min(minX, point.x);
		minY = std::min(minY, point.y);
		minZ = std::min(minZ, point.z);

		maxX = std::max(maxX, point.x);
		maxY = std::max(maxY, point.y);
		maxZ = std::max(maxZ, point.z);
	}

	AABB bbox;
	bbox.minPoint = Point3D(minX, minY, minZ);
	bbox.maxPoint = Point3D(maxX, maxY, maxZ);

	return bbox;
}

// OBB 计算函数实现
OBB calculateOBB(const std::vector<Point3D>& points) {
	if (points.empty()) {
		return OBB();
	}

	// 1. 计算点云的中心点
	Point3D center(0, 0, 0);
	for (const auto& point : points) {
		center.x += point.x;
		center.y += point.y;
		center.z += point.z;
	}
	center.x /= points.size();
	center.y /= points.size();
	center.z /= points.size();

	// 2. 计算协方差矩阵
	double cov[3][3] = { 0 };
	for (const auto& point : points) {
		Point3D diff = point - center;
		cov[0][0] += diff.x * diff.x;
		cov[0][1] += diff.x * diff.y;
		cov[0][2] += diff.x * diff.z;
		cov[1][0] += diff.y * diff.x;
		cov[1][1] += diff.y * diff.y;
		cov[1][2] += diff.y * diff.z;
		cov[2][0] += diff.z * diff.x;
		cov[2][1] += diff.z * diff.y;
		cov[2][2] += diff.z * diff.z;
	}

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			cov[i][j] /= points.size();
		}
	}

	// 3. 计算特征向量（使用幂迭代法）
	Point3D axes[3];

	// 第一个轴（最大方差方向）
	Point3D v1(1, 0, 0);
	for (int iter = 0; iter < 100; ++iter) {
		Point3D newV1(0, 0, 0);
		newV1.x = cov[0][0] * v1.x + cov[0][1] * v1.y + cov[0][2] * v1.z;
		newV1.y = cov[1][0] * v1.x + cov[1][1] * v1.y + cov[1][2] * v1.z;
		newV1.z = cov[2][0] * v1.x + cov[2][1] * v1.y + cov[2][2] * v1.z;

		double length = newV1.length();
		if (length > 0) {
			v1 = Point3D(newV1.x / length, newV1.y / length, newV1.z / length);
		}
	}
	axes[0] = v1;

	// 第二个轴（与第一个轴正交的方向）
	Point3D v2(0, 1, 0);
	v2 = v2 - axes[0] * axes[0].dot(v2);
	if (v2.length() > 0) {
		v2 = v2.normalize();
	}

	for (int iter = 0; iter < 100; ++iter) {
		Point3D newV2(0, 0, 0);
		newV2.x = cov[0][0] * v2.x + cov[0][1] * v2.y + cov[0][2] * v2.z;
		newV2.y = cov[1][0] * v2.x + cov[1][1] * v2.y + cov[1][2] * v2.z;
		newV2.z = cov[2][0] * v2.x + cov[2][1] * v2.y + cov[2][2] * v2.z;

		// 保持与第一个轴正交
		newV2 = newV2 - axes[0] * axes[0].dot(newV2);

		double length = newV2.length();
		if (length > 0) {
			v2 = Point3D(newV2.x / length, newV2.y / length, newV2.z / length);
		}
	}
	axes[1] = v2;

	// 第三个轴（前两个轴的叉积）
	axes[2] = axes[0].cross(axes[1]).normalize();

	// 4. 计算每个轴向上的半长
	Point3D halfExtents(0, 0, 0);
	for (const auto& point : points) {
		Point3D diff = point - center;
		halfExtents.x = std::max(halfExtents.x, std::abs(diff.dot(axes[0])));
		halfExtents.y = std::max(halfExtents.y, std::abs(diff.dot(axes[1])));
		halfExtents.z = std::max(halfExtents.z, std::abs(diff.dot(axes[2])));
	}

	OBB obb;
	obb.center = center;
	obb.axes[0] = axes[0];
	obb.axes[1] = axes[1];
	obb.axes[2] = axes[2];
	obb.halfExtents = halfExtents;

	return obb;
}

std::vector<Point3D> createSurfaceGrid(const std::vector<Point3D>& corners,
	double spacing,
	int surfaceIndex) {
	std::vector<Point3D> gridPoints;

	if (corners.size() != 8) {
		return gridPoints; // 返回空向量
	}

	// 定义六个表面（每个表面由4个顶点组成）
	// 表面索引: 0=前, 1=后, 2=左, 3=右, 4=上, 5=下
	int surfaceVertices[6][4] = {
		{0, 1, 2, 3}, // 前表面 (z最小)
		{4, 5, 6, 7}, // 后表面 (z最大)
		{0, 3, 7, 4}, // 左表面 (x最小)
		{1, 2, 6, 5}, // 右表面 (x最大)
		{2, 3, 7, 6}, // 上表面 (y最大)
		{0, 1, 5, 4}  // 下表面 (y最小)
	};

	if (surfaceIndex < 0 || surfaceIndex > 5) {
		return gridPoints; // 无效的表面索引
	}

	// 获取表面四个顶点
	Point3D v0 = corners[surfaceVertices[surfaceIndex][0]];
	Point3D v1 = corners[surfaceVertices[surfaceIndex][1]];
	Point3D v2 = corners[surfaceVertices[surfaceIndex][2]];
	Point3D v3 = corners[surfaceVertices[surfaceIndex][3]];

	// 计算表面的两个方向向量
	Point3D uDir = v1 - v0; // 第一个方向
	Point3D vDir = v3 - v0; // 第二个方向

	// 计算表面的法向量（用于验证点是否在表面上）
	Point3D normal = uDir.cross(vDir).normalize();

	// 计算两个方向的长度
	double uLength = uDir.length();
	double vLength = vDir.length();

	// 计算两个方向上的单位向量
	Point3D uUnit = uDir.normalize();
	Point3D vUnit = vDir.normalize();

	// 计算在两个方向上需要生成的点数
	int uPoints = std::max(1, static_cast<int>(std::ceil(uLength / spacing)));
	int vPoints = std::max(1, static_cast<int>(std::ceil(vLength / spacing)));

	// 调整实际间距，使其均匀分布
	double actualUSpacing = uLength / uPoints;
	double actualVSpacing = vLength / vPoints;

	// 生成点阵
	for (int i = 0; i <= uPoints; ++i) {
		for (int j = 0; j <= vPoints; ++j) {
			// 计算点在表面局部坐标系中的位置
			Point3D point = v0 + (uUnit * (i * actualUSpacing)) + (vUnit * (j * actualVSpacing));

			// 验证点是否在表面上（可选，提高精度）
			// 这里可以添加更复杂的验证逻辑，确保点在四边形表面内

			gridPoints.push_back(point);
		}
	}

	return gridPoints;
}

// 重载函数：直接在AABB表面创建点阵
std::vector<Point3D> createSurfaceGrid(const AABB& aabb, double spacing, int surfaceIndex) {
	std::vector<Point3D> corners = aabb.getCorners();
	return createSurfaceGrid(corners, spacing, surfaceIndex);
}

// 重载函数：直接在OBB表面创建点阵
std::vector<Point3D> createSurfaceGrid(const OBB& obb, double spacing, int surfaceIndex) {
	std::vector<Point3D> corners = obb.getCorners();
	return createSurfaceGrid(corners, spacing, surfaceIndex);
}

// 更精确的版本：确保点在四边形表面内
std::vector<Point3D> createSurfaceGridPrecise(const std::vector<Point3D>& corners,
	double spacing,
	int surfaceIndex) {
	std::vector<Point3D> gridPoints;

	if (corners.size() != 8) {
		return gridPoints;
	}

	int surfaceVertices[6][4] = {
		{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 3, 7, 4},
		{1, 2, 6, 5}, {2, 3, 7, 6}, {0, 1, 5, 4}
	};

	if (surfaceIndex < 0 || surfaceIndex > 5) {
		return gridPoints;
	}

	Point3D v0 = corners[surfaceVertices[surfaceIndex][0]];
	Point3D v1 = corners[surfaceVertices[surfaceIndex][1]];
	Point3D v2 = corners[surfaceVertices[surfaceIndex][2]];
	Point3D v3 = corners[surfaceVertices[surfaceIndex][3]];

	// 使用重心坐标法验证点是否在四边形内
	auto isPointInQuad = [](const Point3D& P, const Point3D& A, const Point3D& B,
		const Point3D& C, const Point3D& D) -> bool {
		// 将四边形分成两个三角形：ABC 和 ACD
		auto isPointInTriangle = [](const Point3D& P, const Point3D& A,
			const Point3D& B, const Point3D& C) -> bool {
			Point3D v0 = C - A;
			Point3D v1 = B - A;
			Point3D v2 = P - A;

			double dot00 = v0.dot(v0);
			double dot01 = v0.dot(v1);
			double dot02 = v0.dot(v2);
			double dot11 = v1.dot(v1);
			double dot12 = v1.dot(v2);

			double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
			double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			return (u >= 0) && (v >= 0) && (u + v <= 1);
		};

		return isPointInTriangle(P, A, B, C) || isPointInTriangle(P, A, C, D);
	};

	Point3D uDir = v1 - v0;
	Point3D vDir = v3 - v0;

	double uLength = uDir.length();
	double vLength = vDir.length();

	Point3D uUnit = uDir.normalize();
	Point3D vUnit = vDir.normalize();

	int uPoints = std::max(1, static_cast<int>(std::ceil(uLength / spacing)));
	int vPoints = std::max(1, static_cast<int>(std::ceil(vLength / spacing)));

	double actualUSpacing = uLength / uPoints;
	double actualVSpacing = vLength / vPoints;

	// 扩展边界，确保覆盖整个表面
	for (int i = -1; i <= uPoints + 1; ++i) {
		for (int j = -1; j <= vPoints + 1; ++j) {
			Point3D point = v0 + (uUnit * (i * actualUSpacing)) + (vUnit * (j * actualVSpacing));

			// 精确验证点是否在四边形内
			if (isPointInQuad(point, v0, v1, v2, v3)) {
				gridPoints.push_back(point);
			}
		}
	}

	return gridPoints;
}

// 计算两个向量之间的夹角（弧度）
double angleBetweenVectors(const Point3D& v1, const Point3D& v2) {
	double dotProduct = v1.dot(v2);
	double lengthProduct = v1.length() * v2.length();

	if (lengthProduct < 1e-10) {
		return 0.0; // 避免除以零
	}

	double cosAngle = dotProduct / lengthProduct;
	// 确保cosAngle在有效范围内
	cosAngle = std::max(-1.0, std::min(1.0, cosAngle));

	return std::acos(cosAngle);
}

// 计算表面法向量
Point3D calculateSurfaceNormal(const std::vector<Point3D>& corners, int surfaceIndex) {
	if (corners.size() != 8) {
		return Point3D(0, 0, 0);
	}

	// 表面索引定义
	int surfaceVertices[6][4] = {
		{0, 1, 2, 3}, // 前表面
		{4, 5, 6, 7}, // 后表面
		{0, 3, 7, 4}, // 左表面
		{1, 2, 6, 5}, // 右表面
		{2, 3, 7, 6}, // 上表面
		{0, 1, 5, 4}  // 下表面
	};

	if (surfaceIndex < 0 || surfaceIndex > 5) {
		return Point3D(0, 0, 0);
	}

	// 获取表面四个顶点
	Point3D v0 = corners[surfaceVertices[surfaceIndex][0]];
	Point3D v1 = corners[surfaceVertices[surfaceIndex][1]];
	Point3D v2 = corners[surfaceVertices[surfaceIndex][2]];
	Point3D v3 = corners[surfaceVertices[surfaceIndex][3]];

	// 计算两个边向量
	Point3D edge1 = v1 - v0;
	Point3D edge2 = v3 - v0;

	// 计算法向量（叉积）
	Point3D normal = edge1.cross(edge2);

	// 归一化
	return normal.normalize();
}

// 找到与给定方向夹角最小的表面
int findClosestSurface(const std::vector<Point3D>& corners, const Point3D& direction) {
	if (corners.size() != 8) {
		return -1;
	}

	Point3D dirNormalized = direction.normalize();
	double minAngle = std::numeric_limits<double>::max();
	int closestSurface = -1;

	// 检查所有6个表面
	for (int i = 0; i < 6; ++i) {
		Point3D surfaceNormal = calculateSurfaceNormal(corners, i);

		// 计算表面法向量与给定方向之间的夹角
		double angle = angleBetweenVectors(surfaceNormal, dirNormalized);

		// 我们想要最小夹角（最接近平行）
		if (angle < minAngle) {
			minAngle = angle;
			closestSurface = i;
		}
	}

	return closestSurface;
}

// 对与给定方向成最小夹角的表面进行划分，输出长方体的四个角点
std::vector<Point3D> createGridOnClosestSurface(const std::vector<Point3D>& corners,
	double rectLength, double rectWidth,
	const Point3D& direction) {
	std::vector<Point3D> gridPoints;

	if (corners.size() != 8 || rectLength <= 0 || rectWidth <= 0) {
		return gridPoints;
	}

	// 找到与给定方向夹角最小的表面
	int surfaceIndex = findClosestSurface(corners, direction);

	if (surfaceIndex == -1) {
		return gridPoints;
	}

	// 表面索引定义
	int surfaceVertices[6][4] = {
		{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 3, 7, 4},
		{1, 2, 6, 5}, {2, 3, 7, 6}, {0, 1, 5, 4}
	};

	Point3D v0 = corners[surfaceVertices[surfaceIndex][0]];
	Point3D v1 = corners[surfaceVertices[surfaceIndex][1]];
	Point3D v2 = corners[surfaceVertices[surfaceIndex][2]];
	Point3D v3 = corners[surfaceVertices[surfaceIndex][3]];

	// 计算表面的两个方向向量
	Point3D uDir = v1 - v0; // 第一个方向
	Point3D vDir = v3 - v0; // 第二个方向

	// 计算两个方向的长度
	double surfaceLength = uDir.length();
	double surfaceWidth = vDir.length();

	// 计算两个方向上的单位向量
	Point3D uUnit = uDir.normalize();
	Point3D vUnit = vDir.normalize();

	// 向上圆整长方体数量
	int lengthCount = std::max(1, static_cast<int>(std::ceil(surfaceLength / rectLength)));
	int widthCount = std::max(1, static_cast<int>(std::ceil(surfaceWidth / rectWidth)));

	// 重新计算长方体边长以实现全覆盖
	double adjustedRectLength = surfaceLength / lengthCount;
	double adjustedRectWidth = surfaceWidth / widthCount;

	// 生成每个长方体的四个角点
	for (int i = 0; i < lengthCount; ++i) {
		for (int j = 0; j < widthCount; ++j) {
			// 计算长方体的起始位置
			double startU = i * adjustedRectLength;
			double startV = j * adjustedRectWidth;

			// 生成四个角点
			Point3D corner1 = v0 + (uUnit * startU) + (vUnit * startV);
			Point3D corner2 = v0 + (uUnit * (startU + adjustedRectLength)) + (vUnit * startV);
			Point3D corner3 = v0 + (uUnit * (startU + adjustedRectLength)) + (vUnit * (startV + adjustedRectWidth));
			Point3D corner4 = v0 + (uUnit * startU) + (vUnit * (startV + adjustedRectWidth));

			// 将四个角点按顺序添加到结果中
			gridPoints.push_back(corner1);
			gridPoints.push_back(corner2);
			gridPoints.push_back(corner3);
			gridPoints.push_back(corner4);
		}
	}

	return gridPoints;
}

// 重载函数：直接使用AABB
std::vector<Point3D> createGridOnClosestSurface(const AABB& aabb,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment) {
	std::vector<Point3D> corners = aabb.getCorners();
	return createGridOnClosestSurface(corners, rectLength, rectWidth, direction);
}

// 重载函数：直接使用OBB
std::vector<Point3D> createGridOnClosestSurface(const OBB& obb,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment) {
	std::vector<Point3D> corners = obb.getCorners();
	return createGridOnClosestSurface(corners, rectLength, rectWidth, direction);
}

EnhancedGridResult createEnhancedGridOnClosestSurface(const std::vector<Point3D>& corners,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment) {
	EnhancedGridResult result;

	if (corners.size() != 8 || rectLength <= 0 || rectWidth <= 0) {
		return result;
	}

	// 找到与给定方向夹角最小的表面
	int surfaceIndex = findClosestSurface(corners, direction);
	result.surfaceIndex = surfaceIndex;

	if (surfaceIndex == -1) {
		return result;
	}

	// 计算夹角
	Point3D surfaceNormal = calculateSurfaceNormal(corners, surfaceIndex);
	Point3D dirNormalized = direction.normalize();
	result.angle = angleBetweenVectors(surfaceNormal, dirNormalized);

	int surfaceVertices[6][4] = {
		{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 3, 7, 4},
		{1, 2, 6, 5}, {2, 3, 7, 6}, {0, 1, 5, 4}
	};

	Point3D v0 = corners[surfaceVertices[surfaceIndex][0]];
	Point3D v1 = corners[surfaceVertices[surfaceIndex][1]];
	Point3D v2 = corners[surfaceVertices[surfaceIndex][2]];
	Point3D v3 = corners[surfaceVertices[surfaceIndex][3]];

	Point3D uDir = v1 - v0;
	Point3D vDir = v3 - v0;

	double surfaceLength = uDir.length();
	double surfaceWidth = vDir.length();

	Point3D uUnit = uDir.normalize();
	Point3D vUnit = vDir.normalize();

	int lengthCount = std::max(1, static_cast<int>(std::floor(surfaceLength / rectLength)));
	int widthCount = std::max(1, static_cast<int>(std::floor(surfaceWidth / rectWidth)));

	double usedLength = lengthCount * rectLength;
	double usedWidth = widthCount * rectWidth;

	double startOffsetU = 0.0;
	double startOffsetV = 0.0;

	if (centerAlignment) {
		startOffsetU = (surfaceLength - usedLength) / 2.0;
		startOffsetV = (surfaceWidth - usedWidth) / 2.0;
	}

	// 生成每个长方体的四个角点
	for (int i = 0; i < lengthCount; ++i) {
		for (int j = 0; j < widthCount; ++j) {
			double startU = startOffsetU + i * rectLength;
			double startV = startOffsetV + j * rectWidth;

			Point3D corner1 = v0 + (uUnit * startU) + (vUnit * startV);
			Point3D corner2 = v0 + (uUnit * (startU + rectLength)) + (vUnit * startV);
			Point3D corner3 = v0 + (uUnit * (startU + rectLength)) + (vUnit * (startV + rectWidth));
			Point3D corner4 = v0 + (uUnit * startU) + (vUnit * (startV + rectWidth));

			// 将四个角点作为一个矩形存储
			std::vector<Point3D> rectangle = { corner1, corner2, corner3, corner4 };
			result.rectangles.push_back(rectangle);
		}
	}

	return result;
}

// 表面类型名称（用于输出）
const char* getSurfaceName(int surfaceIndex) {
	const char* names[] = { "FRONT", "BACK", "LEFT", "RIGHT", "TOP", "BOTTOM" };
	if (surfaceIndex >= 0 && surfaceIndex < 6) {
		return names[surfaceIndex];
	}
	return "UNKNOWN";
}