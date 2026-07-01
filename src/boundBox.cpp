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

// 静态工厂方法实现
AABB AABB::calculateAABB(const std::vector<Point3D>& points) {
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

// 表面网格生成方法实现
std::vector<Point3D> AABB::createSurfaceGrid(double spacing, int surfaceIndex) const {
	std::vector<Point3D> corners = this->getCorners();
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

			gridPoints.push_back(point);
		}
	}

	return gridPoints;
}

std::vector<Point3D> AABB::createGridOnClosestSurface(double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment) const {
	std::vector<Point3D> corners = this->getCorners();
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

// OBB 方法实现
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

// OBB 静态工厂方法实现
OBB OBB::calculateOBB(const std::vector<Point3D>& points) {
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

// OBB 表面网格生成方法实现
std::vector<Point3D> OBB::createSurfaceGrid(double spacing, int surfaceIndex) const {
	std::vector<Point3D> corners = this->getCorners();
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

			gridPoints.push_back(point);
		}
	}

	return gridPoints;
}

std::vector<Point3D> OBB::createGridOnClosestSurface(double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment) const {
	std::vector<Point3D> corners = this->getCorners();
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

std::vector<double> OBB::calculateCoordinateSystemFromCorners(const std::vector<double>& corners) const
{

	if (corners.size() != 24) { // 8个点 * 3个坐标 = 24
		throw std::invalid_argument("角点数量必须为24（8个点，每个点3个坐标）");
	}

	// 将输入的角点数据转换为Point3D数组
	std::vector<Point3D> points(8);
	for (int i = 0; i < 8; i++) {
		points[i] = Point3D(corners[i * 3], corners[i * 3 + 1], corners[i * 3 + 2]);
	}

	// 步骤1: 计算包围盒的中心点
	Point3D center(0, 0, 0);
	for (const auto& pt : points) {
		center = center + pt;
	}
	center.x /= 8.0;
	center.y /= 8.0;
	center.z /= 8.0;

	// 步骤2: 识别OBB的三个轴向量
	// OBB的8个角点中，相对的两个顶点连线会穿过中心点
	// 我们可以通过计算所有边的中点，找到穿过中心的边对来识别轴
	std::vector<Point3D> edges; // 存储所有12条边的向量

	// 定义OBB的标准连接方式（假设角点按特定顺序排列）
	int edgeConnections[][2] = {
		{0, 1}, {1, 3}, {3, 2}, {2, 0}, // 底面边
		{4, 5}, {5, 7}, {7, 6}, {6, 4}, // 顶面边
		{0, 4}, {1, 5}, {2, 6}, {3, 7}  // 垂直边
	};

	for (int i = 0; i < 12; i++) {
		Point3D p1 = points[edgeConnections[i][0]];
		Point3D p2 = points[edgeConnections[i][1]];
		Point3D edgeVec = p2 - p1;
		edges.push_back(edgeVec);
	}

	// 步骤3: 找出三个主要轴向量（具有相同长度的边对应同一轴）
	std::vector<double> edgeLengths;
	std::vector<Point3D> normalizedEdges;

	for (const auto& edge : edges) {
		double length = edge.magnitude();
		edgeLengths.push_back(length);
		normalizedEdges.push_back(edge.normalize());
	}

	// 找出三种不同的边长（对应OBB的三个维度）
	std::vector<double> uniqueLengths;
	for (double len : edgeLengths) {
		bool found = false;
		for (double existingLen : uniqueLengths) {
			if (std::abs(len - existingLen) < 1e-6) {
				found = true;
				break;
			}
		}
		if (!found) {
			uniqueLengths.push_back(len);
		}
	}

	// 排序边长（从小到大）
	std::sort(uniqueLengths.begin(), uniqueLengths.end());

	// 步骤4: 识别三个轴向量
	std::vector<Point3D> axes(3);
	std::vector<double> axisLengths(3);

	for (int i = 0; i < 3; i++) {
		Point3D avgAxis(0, 0, 0);
		int count = 0;

		for (int j = 0; j < 12; j++) {
			if (std::abs(edgeLengths[j] - uniqueLengths[i]) < 1e-6) {
				avgAxis = avgAxis + normalizedEdges[j];
				count++;
			}
		}

		if (count > 0) {
			avgAxis = avgAxis * (1.0 / count);
			axes[i] = avgAxis.normalize();
			axisLengths[i] = uniqueLengths[i];
		}
	}

	// 确保轴向量形成右手坐标系
	Point3D crossProduct = axes[0].cross(axes[1]);
	if (crossProduct.dot(axes[2]) < 0) {
		axes[2] = Point3D(-axes[2].x, -axes[2].y, -axes[2].z);
	}

	// 步骤5: 构建旋转矩阵
	// 由于不能使用Eigen，我们手动构建旋转矩阵
	double rotationMatrix[3][3];
	// 注意：根据右手坐标系，我们把axes[0], axes[1], axes[2]分别对应X, Y, Z轴
	rotationMatrix[0][0] = axes[0].x; // Rxx
	rotationMatrix[0][1] = axes[1].x; // Rxy  
	rotationMatrix[0][2] = axes[2].x; // Rxz
	rotationMatrix[1][0] = axes[0].y; // Ryx
	rotationMatrix[1][1] = axes[1].y; // Ryy
	rotationMatrix[1][2] = axes[2].y; // Ryz
	rotationMatrix[2][0] = axes[0].z; // Rzx
	rotationMatrix[2][1] = axes[1].z; // Rzy
	rotationMatrix[2][2] = axes[2].z; // Rzz

	// 步骤6: 从旋转矩阵计算XYZ欧拉角
	double alpha, beta, gamma; // 分别对应绕X、Y、Z轴的旋转角度

	// 从旋转矩阵提取XYZ欧拉角 (Tait-Bryan angles)
	beta = asin(-rotationMatrix[2][0]); // 绕Y轴的旋转

	double cosBeta = cos(beta);

	if (std::abs(cosBeta) > 1e-6) { // 避免除零
		alpha = atan2(rotationMatrix[2][1], rotationMatrix[2][2]); // 绕X轴的旋转
		gamma = atan2(rotationMatrix[1][0], rotationMatrix[0][0]); // 绕Z轴的旋转
	}
	else {
		// 万向锁情况
		alpha = 0.0;
		gamma = atan2(-rotationMatrix[0][1], rotationMatrix[1][1]);
	}

	// 步骤7: 构建EULERANGLEXYZ格式的输出
	std::vector<double> result(6);
	result[0] = center.x;  // X坐标
	result[1] = center.y;  // Y坐标
	result[2] = center.z;  // Z坐标
	result[3] = alpha;     // alpha (绕X轴旋转)
	result[4] = beta;      // beta (绕Y轴旋转)
	result[5] = gamma;     // gamma (绕Z轴旋转)

	return result;
}

// 辅助函数实现
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

std::vector<Point3D> createSurfaceGridPrecise(const std::vector<Point3D>& corners,
	double spacing, int surfaceIndex) {
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


