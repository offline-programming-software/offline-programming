#pragma once
#include <qstring.h>
#include<array>
#include<vector>
#include <nlohmann/json.hpp>
#include<Eigen/dense>
#include <Windows.h>

using namespace Eigen;
struct coeffs
{
	//设置三组系数，每组5个系数，全部初始化为0
	double a0, a1, a2, a3, a4, a5;
	double b0, b1, b2, b3, b4, b5;
	double c0, c1, c2, c3, c4, c5;

	coeffs() : a0(0), a1(0), a2(0), a3(0), a4(0), a5(0),
		b0(0), b1(0), b2(0), b3(0), b4(0), b5(0),
		c0(0), c1(0), c2(0), c3(0), c4(0), c5(0) {
	}
};

struct trajPoint
{
	double x;
	double y;
	double z;
	double q1;
	double q2;
	double q3;
	double q4;
	ULONG id;
};
inline void to_json(nlohmann::json& j, const coeffs& c)
{
	j = {
  {"a", {c.a0, c.a1, c.a2, c.a3, c.a4, c.a5}},
		{"b", {c.b0, c.b1, c.b2, c.b3, c.b4, c.b5}},
		{"c", {c.c0, c.c1, c.c2, c.c3, c.c4, c.c5}}
	};
}

inline void from_json(const nlohmann::json& j, coeffs& c)
{
	auto a = j.at("a").get<std::array<double, 6>>();
	auto b = j.at("b").get<std::array<double, 6>>();
	auto cArr = j.at("c").get<std::array<double, 6>>();

	c.a0 = a[0]; c.a1 = a[1]; c.a2 = a[2];
	c.a3 = a[3]; c.a4 = a[4]; c.a5 = a[5];

	c.b0 = b[0]; c.b1 = b[1]; c.b2 = b[2];
	c.b3 = b[3]; c.b4 = b[4]; c.b5 = b[5];

	c.c0 = cArr[0]; c.c1 = cArr[1]; c.c2 = cArr[2];
	c.c3 = cArr[3]; c.c4 = cArr[4]; c.c5 = cArr[5];
}


/**
 * @brief 将整机各个部分的弯曲变形看作不同函数
 * 
 * @details 
 * 
 * 创建一个Correction对象，需要用户输入如下属性
 *	- 1. 修正名称					m_name
 * - 2. 插值类型
 * - 3. 梁模型	
 * - 4. 作用域
 * - 5. 梁的顶点位置
 * - 6. 梁的方向
 * - 7. 是否修正姿态
 * - 8. 标志点 & 测量点  
 * - 9. 弯曲角度
 * 用户可能一次无法定义全部属性，因此有一个手动方法 
 * correction.solve()来计算结果，此方法返回参数矩阵m_coeffs
 * 
 * correction对象有一个开关：isApply控制是否修正
 * 通过方法计算出的值
 * - 1. coeffs:		vector<vector<double>>
 * - 2. offset:		vector<array<double,7>>
 * @code
 * ```
 * Correction mycor;
 * //为变形修正赋予属性
 * mycor.name = ...;
 * mycor.range = ...;
 * ...
 * //计算参数
 * mycor.m_coeffs = mycor.solve();  
 * //应用变形
 * 
 * //应用&撤销变形
 * mycor.apply();
 * mycor.withdraw();
 * ```.
 */
class Correction
{
public:

	enum class interpolationType {
		Liner,   ///线性分段
		Euler,
		Timoshenko
	};


	
	
	


	Correction() ;
	~Correction();
	

	
	//------getter-----------
	QString name() const { return m_name; }   //getter函数
	interpolationType type() const { return m_interType; }
	std::array<double, 6> range() const {
		return m_range;
	}
	double rang(int i) const { return m_range[i]; }
	bool isPosCorrect() const { return m_isPosCorrect; }
	std::vector<double> flagPoints() const { return m_flagPoints; }
	std::vector<double> measurePoints() const { return m_measurePoints; }
	bool isApplied() const { return m_isApply; }
	// 在 getter-----------  部分添加
	//------getter-----------

	//------setter-----------
	void setName(const QString& name) { m_name = name; }
	void setType(interpolationType type) { m_interType = type; }
	void setRange(const double range[6]) {
		for (int i = 0; i < 6; ++i)
			m_range[i] = range[i];
	}
	void setIsPosCorrect(bool flag) { m_isPosCorrect = flag; }
	void setFlagPoints(const std::vector<double> flagPoints) { m_flagPoints = flagPoints; }
	void setMeasurePoints(const std::vector<double> measurePoints) { m_measurePoints = measurePoints; }
	void setIsApply(bool isChecked) { m_isApply = isChecked; }
	void setBeamDir(const std::vector<double> &dir) { vBeamDirection = dir; }
	void setBeamOrigin(const std::vector<double>& orig) { vBeamOrigin = orig; }
	void setBendingDeg(double deg) { m_bendingDeg = deg; }
	//------setter-----------
public:	
	QString m_name = "null";/**修正名称 - json*/
	//选项
	interpolationType m_interType = interpolationType::Liner;/**默认插值类型 -json*/
	bool m_isApply = false;/**是否应用json*/
	bool m_isPosCorrect = false; /**是否修正姿态-json*/

	//待修正轨迹点
	std::vector<trajPoint> m_trajPointsToCorrect; /**待修正轨迹点-cal*/
	std::vector<trajPoint> m_originTrajPoints; /**原始轨迹点-cal*/
	//作用域
	std::array<double, 6> m_range = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }; /**作用域-json*/

	//约束点&弯曲量输入
	double m_bendingDeg = 0.0;/**为方便定义，输入的弯曲量角度-json*/
	std::vector<double> m_flagPoints; /**标志点-json*/
	std::vector<double> m_measurePoints;/**测量点-json*/

	//beamFream
	Eigen::Vector3d beamOrigin; /**梁的原点位置-json*/
	Eigen::Vector3d beamDirection; /**梁的方向-json*/
	std::vector<double> vBeamOrigin = {0.0,0.0,0.0}; /**用于json存储的梁原点位置-json*/
	std::vector<double> vBeamDirection = { 1.0,0.0,0.0 };/**用于json存储的梁方向-json*/ 
	Eigen::Matrix4d m_TBO; /**梁坐标系到机器人基坐标系的变换矩阵*/

	//计算结果	
	coeffs m_coeffs; /**修正函数系数-json*/
	std::vector<std::array<double,7>> m_v2dOffSets; /**修正量*/
	Correction* m_parentCorrection = nullptr;
	std::vector<Correction*> m_childCorrections;
private:


private:
		Eigen::MatrixX3d HT(Eigen::MatrixX3d pointSet, Eigen::Matrix4d TransformMatrix);
		coeffs timoShenkoBeamSolve(const MatrixX3d& flagPointsMat, const MatrixX3d& measurePointsMat);
		/**
		 * @brief LinearSolve 线性方程组求解
		 * 
		 * @param flagPointsMat
		 * @param measurePointsMat
		 * \return 
		 */
		coeffs LinearSolve(const MatrixX3d& flagPointsMat, const MatrixX3d& measurePointsMat);
		

public:
	//算法部分
	//属性编辑界面
	/**
	 * @brief 计算参数，程序总入口
	 */
	coeffs calCoeffs();

	//在管理界面
	/**
	 * 计算待修正轨迹点.
	 */
	void prepareTrajPoints2Correct(const std::vector<std::array<double, 7>>& ptList);
	/**
	 * fn 
	 * @brief 计算单个点的修正量
	 */
	void calPointOffset();
	/**
	 * @brief 计算修正量，程序总入口
	 */
	std::vector<std::array<double, 7>> calOffset(const std::vector<std::array<double,7>> &trajPoint, const coeffs &c);
	/**
	 * 应用修正
	 * 1. 计算当前range待修正轨迹点ID列表
	 * 2. 计算修正量offset
	 * 3. 修正量应用到待修正轨迹点
	 */
	void applyCorrection(const std::vector<std::array<double, 7>> &offsets);
	/**
	 * 撤销修正.
	 * 1. 
	 */
	void withDrawCorrection();

	//功能函数: 
	
};
