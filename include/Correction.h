#pragma once
#include <qstring.h>
#include<array>
#include<vector>
#include <nlohmann/json.hpp>
class Correction
{
public:
	enum class Type {
		Bending,    // 弯曲变形
		Docking,    // 停靠误差
		Other       // 其他类型
	};
	enum class interpolationType {
		Liner,   ///线性分段
		Poly,    ///多项式
		Other
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
	void set_m_v2dTrajPointsToCorrect(const std::vector<std::vector<double>> trajPoint2Correct) { m_v2dTrajPointsToCorrect = trajPoint2Correct; }
	void setIsApply(bool isChecked) { m_isApply = isChecked; }
	//------setter-----------
	//virtual void apply() = 0;na
public:	
	QString m_name = "null";/**修正名称*/
	interpolationType m_interType = interpolationType::Liner;/**插值类型*/
	bool m_isPosCorrect = false; /**是否修正姿态*/
	std::array<double, 6> m_range = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }; /**作用域*/
	std::vector<std::vector<double>> m_v2dTrajPointsToCorrect;
	std::vector<double> m_flagPoints; /**标志点*/
	std::vector<double> m_measurePoints;/**测量点*/
	bool m_isApply = false;/**是否应用*/
	
	// ✅ 非模板 JSON 接口


};
void to_json(nlohmann::json& j, const Correction& c);
void from_json(const nlohmann::json& j, Correction& c);
class Bending :Correction
{
public:

private:


};