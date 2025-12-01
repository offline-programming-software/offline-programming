#ifndef BOUNDBOX_H
#define BOUNDBOX_H

#include <vector>

// 表面索引的枚举，便于使用
enum SurfaceType {
	FRONT = 0,   // 前表面 (z最小)
	BACK = 1,    // 后表面 (z最大)  
	LEFT = 2,    // 左表面 (x最小)
	RIGHT = 3,   // 右表面 (x最大)
	TOP = 4,     // 上表面 (y最大)
	BOTTOM = 5   // 下表面 (y最小)
};

// 点结构体
struct Point3D {
	double x, y, z;
	Point3D(double x = 0, double y = 0, double z = 0);

	// 向量运算
	Point3D operator+(const Point3D& other) const;
	Point3D operator-(const Point3D& other) const;
	Point3D operator*(double scalar) const; // 添加标量乘法
	double dot(const Point3D& other) const;
	Point3D cross(const Point3D& other) const;
	double length() const;
	Point3D normalize() const;
};

// 非成员函数：标量乘以点
Point3D operator*(double scalar, const Point3D& point);

// AABB（轴向对齐包围盒）
struct AABB {
	Point3D minPoint;
	Point3D maxPoint;

	AABB() : minPoint(0, 0, 0), maxPoint(0, 0, 0) {}
	std::vector<Point3D> getCorners() const;
	double volume() const;
};

// OBB（有向包围盒）
struct OBB {
	Point3D center;
	Point3D axes[3];
	Point3D halfExtents;

	std::vector<Point3D> getCorners() const;
	double volume() const;
};

// AABB计算方法
AABB calculateAABB(const std::vector<Point3D>& points);

// OBB计算方法（基于PCA主成分分析）
OBB calculateOBB(const std::vector<Point3D>& points);

std::vector<Point3D> createSurfaceGrid(const std::vector<Point3D>& corners,
	double spacing, int surfaceIndex);
std::vector<Point3D> createSurfaceGrid(const AABB& aabb, double spacing, int surfaceIndex);
std::vector<Point3D> createSurfaceGrid(const OBB& obb, double spacing, int surfaceIndex);
std::vector<Point3D> createSurfaceGridPrecise(const std::vector<Point3D>& corners,
	double spacing, int surfaceIndex);


struct EnhancedGridResult {
	int surfaceIndex;
	std::vector<std::vector<Point3D>> rectangles;
	double angle;
};

// 函数声明
double angleBetweenVectors(const Point3D& v1, const Point3D& v2);
Point3D calculateSurfaceNormal(const std::vector<Point3D>& corners, int surfaceIndex);
int findClosestSurface(const std::vector<Point3D>& corners, const Point3D& direction);
std::vector<Point3D> createGridOnClosestSurface(const std::vector<Point3D>& corners,
	double rectLength, double rectWidth,
	const Point3D& direction);
std::vector<Point3D> createGridOnClosestSurface(const AABB& aabb,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment = true);
std::vector<Point3D> createGridOnClosestSurface(const OBB& obb,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment = true);
EnhancedGridResult createEnhancedGridOnClosestSurface(const std::vector<Point3D>& corners,
	double rectLength, double rectWidth,
	const Point3D& direction,
	bool centerAlignment = true);
const char* getSurfaceName(int surfaceIndex);

#endif // BOUNDBOX_H