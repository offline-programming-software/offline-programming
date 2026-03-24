#define GLOG_USE_GLOG_EXPORT
#define GLOG_NO_ABBREVIATED_SEVERITIES
#define _USE_MATH_DEFINES
#pragma warning(disable: 4996)

#include<iostream>
#include<filesystem>
#include<iomanip>
#include "core\Correction.h"



Correction::Correction() = default;
Correction::~Correction() = default;

/**
 * 以timoshenko梁模型建立问题，并考虑梁的扭转
 *
 * 问题建模：
 * 飞机上的梁、机翼等部件长径比大，易发生弯曲变形，导致实机与数模产生不一致。
 *
 * 梁坐标系定义：
 * - x轴：沿梁长轴，指向自由端
 * - y轴：与重力方向相反
 * - z轴：右手坐标系
 *
 * 考虑的变形：
 * - 沿y轴负方向的挠曲变形：v(x)
 * - 绕x轴的扭转变形：phi_x(x)
 * - 绕z轴的旋转：phi_z(x)
 *
 * 位移关系：
 * u_x(x,y,z) = -y*phi_z(x)
 * u_y(x,y,z) = v(x) - z*phi_x(x)
 * u_z(x,y,z) = y*phi_x(x)
 *
 * 多项式拟合：
 * v(x)     = a1 + a2*x + a3*x^2 + a4*x^3
 * phi_x(x) = b1 + b2*x + b3*x^2
 * phi_z(x) = c1 + c2*x + c3*x^2
 */
struct timoshenkoBeamResidual
{
	// 理想未变形点坐标 (x, y, z)
	// 实际测量点坐标 (xp, yp, zp)
	timoshenkoBeamResidual(double x, double y, double z,
		double xp, double yp, double zp)
		: xi(x), yi(y), zi(z), xip(xp), yip(yp), zip(zp) {
	}

	template <typename T>
	bool operator()(const T* const coeffs, T* residual) const
	{
		// coeffs: a1, a2, a3, a4,           (v(x): 4个参数)
		//         b1, b2, b3,               (phi_x(x): 3个参数)
		//         c1, c2, c3                (phi_z(x): 3个参数)

		// v(x) = a1 + a2*x + a3*x^2 + a4*x^3
		T v_x = coeffs[0] + coeffs[1] * T(xi) + coeffs[2] * T(xi) * T(xi)
			+ coeffs[3] * T(xi) * T(xi) * T(xi);

		// phi_x(x) = b1 + b2*x + b3*x^2
		T phi_x = coeffs[4] + coeffs[5] * T(xi) + coeffs[6] * T(xi) * T(xi);

		// phi_z(x) = c1 + c2*x + c3*x^2
		T phi_z = coeffs[7] + coeffs[8] * T(xi) + coeffs[9] * T(xi) * T(xi);

		// 位移关系式
		// u_x(x,y,z) = -y*phi_z(x)
		T u_x = -T(yi) * phi_z;

		// u_y(x,y,z) = v(x) - z*phi_x(x)
		T u_y = v_x - T(zi) * phi_x;

		// u_z(x,y,z) = y*phi_x(x)
		T u_z = T(yi) * phi_x;

		// 变形后的坐标
		T xiCal = T(xi) - u_x;
		T yiCal = T(yi) - u_y;
		T ziCal = T(zi) - u_z;

		// 残差项: 测量值 - 计算值
		T r1 = T(xip) - xiCal;
		T r2 = T(yip) - yiCal;
		T r3 = T(zip) - ziCal;

		residual[0] = r1;
		residual[1] = r2;
		residual[2] = r3;

		return true;
	}

private:
	double xi, yi, zi;    // 理想未变形点坐标
	double xip, yip, zip; // 实际测量点坐标
};


coeffs Correction::calCoeffs()
{
	
	//构造齐次变换矩阵，将坐标点变换到尾梁坐标系：
	//R = [beamDirection, [0,0,1]^T, zDirection]
	
	// 从向量中读取数据，转换为 Vector3d
	Eigen::Vector3d beamDir(vBeamDirection[0], vBeamDirection[1], vBeamDirection[2]);
	Eigen::Vector3d T(vBeamOrigin[0], vBeamOrigin[1], vBeamOrigin[2]);
	Eigen::Vector3d yDir = Eigen::Vector3d(0, 0, 1);
	Eigen::Vector3d zDir = beamDir.cross(yDir); //计算z方向
	
	// 构造旋转矩阵 R (3x3)
	Eigen::Matrix3d R;
	R.col(0) = beamDir;
	R.col(1) = Eigen::Vector3d(0, 0, 1);
	R.col(2) = zDir;
	
	// 构造齐次变换矩阵 (4x4)
	Eigen::Matrix4d TOB = Eigen::Matrix4d::Identity();
	TOB.block<3, 3>(0, 0) = R;
	TOB.block<3, 1>(0, 3) = T;
	
	// 计算逆变换矩阵
	Eigen::Matrix4d TBO = TOB.inverse();
	//printTBO
	std::cout << "TBO: \n" << TBO << std::endl;
	m_TBO = TBO;
	
	if (m_bendingDeg != 0)
	{
		//优先采用简单弯曲量输入模式
		m_coeffs.a0 = 0.0;
		m_coeffs.a1 = -tan(m_bendingDeg * M_PI / 180.0); //弯曲角度转弯曲斜率
		return m_coeffs;
	}

	//从	 m_flagPoints; 和 m_measurePoints读取数据点转化为matrixx3d
	//然后再应用变换TBO 将其从大地坐标系下转换为尾梁坐标系下
	size_t flagPointsCount = m_flagPoints.size() / 3;
	size_t measurePointsCount = m_measurePoints.size() / 3;

	Eigen::MatrixX3d flagPointsMat(flagPointsCount, 3);
	for (size_t i = 0; i < flagPointsCount; ++i)
	{
		flagPointsMat(i, 0) = m_flagPoints[3 * i];
		flagPointsMat(i, 1) = m_flagPoints[3 * i + 1];
		flagPointsMat(i, 2) = m_flagPoints[3 * i + 2];
	}

	Eigen::MatrixX3d measurePointsMat(measurePointsCount, 3);
	for (size_t i = 0; i < measurePointsCount; ++i)
	{
		measurePointsMat(i, 0) = m_measurePoints[3 * i];
		measurePointsMat(i, 1) = m_measurePoints[3 * i + 1];
		measurePointsMat(i, 2) = m_measurePoints[3 * i + 2];
	}
	std::cout << "flagPointMat = " << flagPointsMat<<std::endl;
	std::cout << "measurePointsMat = " << measurePointsMat << std::endl;

	flagPointsMat = HT(flagPointsMat, TBO);
	measurePointsMat = HT(measurePointsMat, TBO);
	std::cout << "flagPointMat after HT = " << flagPointsMat << std::endl;
	std::cout << "measurePointsMat after HT= " << measurePointsMat << std::endl;
	
	/**
	 * 数据初始化完成，目前有
	 * flagPointsMat
	 * measurePointsMat
	 * \return 
	 */

	

	if (m_interType == interpolationType::Liner)
	{
		//线性拟合求解系数
		double beamLength;
		Eigen::MatrixX3d a = flagPointsMat - measurePointsMat;
		beamLength = flagPointsMat.col(0).maxCoeff() - flagPointsMat.col(0).minCoeff();
		auto V = a.col(1);
		std::cout << "V = " << V.transpose() << std::endl;
		double deltaZ = V.maxCoeff();
		double k =- (deltaZ / beamLength);
		m_coeffs.a0 = 0.0;
		m_coeffs.a1 = k;
		std::cout << "log,线性拟合计算完成，计算斜率k = " << k << std::endl;
	}
	else if (m_interType == interpolationType::Euler)
	{
		//euler拟合方法
	}
	if (m_interType == interpolationType::Timoshenko)
	{
		//timoshenko拟合方法
	}
	return m_coeffs;
}





void Correction::calOffset()
{
	coeffs myCoeffs = calCoeffs();

}


Eigen::MatrixX3d Correction::HT(Eigen::MatrixX3d pointSet, Eigen::Matrix4d TransformMatrix)
{
	// Validate input
	if (pointSet.cols() != 3) {
		throw std::invalid_argument("HT: pointSet must have exactly 3 columns (X, Y, Z)");
	}

	// Add homogeneous coordinate (column of 1s) to convert N×3 to N×4
	int numPoints = pointSet.rows();
	Eigen::MatrixXd homoPoints(numPoints, 4);
	homoPoints.block(0, 0, numPoints, 3) = pointSet.cast<double>();
	homoPoints.col(3) = Eigen::VectorXd::Ones(numPoints);

	// Apply homogeneous transformation: (N×4) × (4×4)^T = (N×4)
	Eigen::MatrixXd transformed = (TransformMatrix * homoPoints.transpose()).transpose();

	// Extract only the first 3 columns (drop the homogeneous coordinate)
	Eigen::MatrixX3d result = transformed.block(0, 0, numPoints, 3).cast<double>();

	return result;
}

coeffs Correction::timoShenkoBeamSolve(const Eigen::MatrixX3d& flagPointsMat, const Eigen::MatrixX3d& measurePointsMat)
{
	coeffs resultCoeffs;
	//int numPoints = flagPointsMat.rows();
	//// ============ TIMOSHENKO BEAM OPTIMIZATION ============
	//std::cout << "\n\n========== Ceres timoshenkoBeam Residual Optimization ==========" << std::endl;

	//// ============ Step 1: Initialize timoshenkoBeam Parameters ============
	//std::cout << "\n[Step 1] Initialize timoshenkoBeam parameters..." << std::endl;

	//// Parameters for Timoshenko beam:
	//// coeffs[0-3]: v(x) = a1 + a2*x + a3*x^2 + a4*x^3
	//// coeffs[4-6]: phi_x(x) = b1 + b2*x + b3*x^2
	//// coeffs[7-9]: phi_z(x) = c1 + c2*x + c3*x^2
	//double timoshenkoCoeffs[10] = {
	//	0.0, 1e-3, 1e-5, 1e-7,      // v(x)
	//	0.0, 1e-4, 1e-6,             // phi_x(x)
	//	0.0, 1e-4, 1e-6              // phi_z(x)
	//};

	//std::cout << "Initial Timoshenko parameters:" << std::endl;
	//std::cout << "  v(x) coeffs: [" << timoshenkoCoeffs[0];
	//for (int i = 1; i < 4; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;
	//std::cout << "  phi_x(x) coeffs: [" << timoshenkoCoeffs[4];
	//for (int i = 5; i < 7; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;
	//std::cout << "  phi_z(x) coeffs: [" << timoshenkoCoeffs[7];
	//for (int i = 8; i < 10; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;

	//// Create new optimization problem for Timoshenko
	//Problem timoshenkoProblem;

	//// ============ Step 2: Add Residual Blocks ============
	//std::cout << "\n[Step 2] Adding " << numPoints << " residual blocks for Timoshenko..." << std::endl;
	//for (int i = 0; i < numPoints; ++i) {
	//	CostFunction* cost_function = new AutoDiffCostFunction<timoshenkoBeamResidual, 3, 10>(
	//		new timoshenkoBeamResidual(
	//			flagPointsMat(i, 0), flagPointsMat(i, 1), flagPointsMat(i, 2),
	//			measurePointsMat(i, 0), measurePointsMat(i, 1), measurePointsMat(i, 2)
	//		)
	//	);
	//	timoshenkoProblem.AddResidualBlock(cost_function, nullptr, timoshenkoCoeffs);
	//}
	//std::cout << "? Residual blocks added" << std::endl;

	//// ============ Step 3: Configure Solver ============
	//std::cout << "\n[Step 3] Configuring Timoshenko solver..." << std::endl;
	//Solver::Options timoshenkoOptions;
	//timoshenkoOptions.linear_solver_type = ceres::DENSE_QR;
	//timoshenkoOptions.minimizer_progress_to_stdout = true;
	//timoshenkoOptions.max_num_iterations = 1000;
	//timoshenkoOptions.function_tolerance = 1e-6;
	//timoshenkoOptions.gradient_tolerance = 1e-8;
	//timoshenkoOptions.parameter_tolerance = 1e-8;

	//std::cout << "Solver options:" << std::endl;
	//std::cout << "  Linear solver: DENSE_QR" << std::endl;
	//std::cout << "  Max iterations: " << timoshenkoOptions.max_num_iterations << std::endl;

	//// ============ Step 4: Run Optimization ============
	//std::cout << "\n[Step 4] Running Timoshenko optimization..." << std::endl;
	//std::cout << "=====================================================" << std::endl;

	//Solver::Summary timoshenkoSummary;
	//ceres::Solve(timoshenkoOptions, &timoshenkoProblem, &timoshenkoSummary);

	//std::cout << "=====================================================" << std::endl;

	//// ============ Step 5: Display Results ============
	//std::cout << "\n[Step 5] Timoshenko Optimization Results" << std::endl;
	//std::cout << "=====================================================" << std::endl;

	//std::cout << timoshenkoSummary.BriefReport() << std::endl;

	//std::cout << "\nOptimized Timoshenko Parameters:" << std::endl;
	//std::cout << "  v(x) coeffs: [" << std::fixed << std::setprecision(6) << timoshenkoCoeffs[0];
	//for (int i = 1; i < 4; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;
	//std::cout << "  phi_x(x) coeffs: [" << timoshenkoCoeffs[4];
	//for (int i = 5; i < 7; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;
	//std::cout << "  phi_z(x) coeffs: [" << timoshenkoCoeffs[7];
	//for (int i = 8; i < 10; ++i) std::cout << ", " << timoshenkoCoeffs[i];
	//std::cout << "]" << std::endl;


	//resultCoeffs.a0 = timoshenkoCoeffs[0];
	//resultCoeffs.a1 = timoshenkoCoeffs[1];
	//resultCoeffs.a2 = timoshenkoCoeffs[2];
	//resultCoeffs.a3 = timoshenkoCoeffs[3];
	//resultCoeffs.b0 = timoshenkoCoeffs[4];
	//resultCoeffs.b1 = timoshenkoCoeffs[5];
	//resultCoeffs.b2 = timoshenkoCoeffs[6];
	//resultCoeffs.c0 = timoshenkoCoeffs[7];
	//resultCoeffs.c1 = timoshenkoCoeffs[8];
	//resultCoeffs.c2 = timoshenkoCoeffs[9];

	//

	//std::cout << "\nOptimization Statistics:" << std::endl;
	//std::cout << "  Initial cost: " << timoshenkoSummary.initial_cost << std::endl;
	//std::cout << "  Final cost: " << timoshenkoSummary.final_cost << std::endl;
	//std::cout << "  Cost reduction: " << (timoshenkoSummary.initial_cost - timoshenkoSummary.final_cost) << std::endl;
	//std::cout << "  Iterations: " << timoshenkoSummary.iterations.size() << std::endl;

	//if (timoshenkoSummary.IsSolutionUsable()) {
	//	std::cout << "\n? Timoshenko Optimization SUCCESSFUL" << std::endl;
	//}
	//else {
	//	std::cout << "\n? Timoshenko Optimization FAILED: " << timoshenkoSummary.message << std::endl;
	//}
	return resultCoeffs;
}

