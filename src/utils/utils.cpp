#pragma once
#include "utils.h"
#include "PQKitCallback.h"
#include <fstream>
#include <iomanip>


namespace utils {
	QStringList BSTR2QStringList(BSTR bstr)
	{

		QStringList list;

		// 1?? 判断 bstr 是否为空
		if (!bstr || SysStringLen(bstr) == 0)
			return list;

		// 2?? BSTR → QString
		QString str = QString::fromWCharArray(bstr, SysStringLen(bstr));
		list = str.split("#", QString::SkipEmptyParts);


		return list;
	}


}

PQUtils::PQUtils(CComPtr<IPQPlatformComponent> pqkit)
	: m_ptrKit(pqkit)
{
}

PQUtils::~PQUtils()
{
}

void PQUtils::setTrajPointPosture(ULONG pointID, const std::array<double, 7>& posture)
{
	DOUBLE dPosition[7];
	for (int i = 0; i < 7; ++i)
		dPosition[i] = posture[i];

	INT nPositionCount = 7;
	m_ptrKit->PQAPIModifyPointPosture(pointID, dPosition, nPositionCount, QUATERNION);
	//测试通过
	//注意事项： 
}

std::vector<trajPoint> PQUtils::getAllTrajPointPosture()
{
	INT count;
	ULONG* pointID;
	std::vector<ULONG> m_vAllPathIDs;
	std::vector<trajPoint> m_vAllPointInfo;

	getID(m_vAllPathIDs, PQ_PATH);

	for (int pathIdx = 0; pathIdx < static_cast<int>(m_vAllPathIDs.size()); pathIdx++)
	{
		m_ptrKit->Path_get_point_count(m_vAllPathIDs[pathIdx], &count);
		if (count <= 0)
			continue;

		// 获取轨迹点 ID
		m_ptrKit->Path_get_point_id(m_vAllPathIDs[pathIdx], &count, &pointID);
		std::vector<ULONG> curPointIDs(pointID, pointID + count);
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointID);

		// 获取轨迹点位姿
		int nPostureSize = 0;
		double* dPosture = nullptr;
		m_ptrKit->Point_get_posture_batch(m_vAllPathIDs[pathIdx], 0, count, QUATERNION, 0, &nPostureSize, &dPosture);

		int ptIndex = 0;
		for (int k = 0; k + 6 < nPostureSize; k += 7, ptIndex++)
		{
			// 跳过无效点（PQ平台哨兵值）
			if (dPosture[k] < -9e+60)
				continue;

			trajPoint point;
			point.id = (ptIndex < static_cast<int>(curPointIDs.size())) ? curPointIDs[ptIndex] : 0;
			point.x = dPosture[k];
			point.y = dPosture[k + 1];
			point.z = dPosture[k + 2];
			point.q1 = dPosture[k + 3];
			point.q2 = dPosture[k + 4];
			point.q3 = dPosture[k + 5];
			point.q4 = dPosture[k + 6];
			m_vAllPointInfo.push_back(point);
		}
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);
	}

	return m_vAllPointInfo;


	//测试通过
}


void PQUtils::getID(std::vector<ULONG>& listID, __MIDL___MIDL_itf_RPC_0000_0000_0005 PQDAtatype)
{
	CComBSTR IDs = nullptr;
	CComBSTR names = nullptr;
	m_ptrKit->pq_GetAllDataObjectsByType(PQDAtatype, &names, &IDs);
	std::wstring wStrID(IDs, IDs.Length());


	
	listID.clear();
	std::wstringstream wss(wStrID);
	std::wstring token;
	while (std::getline(wss, token, L'#'))
	{
		if (!token.empty())
		{
			ULONG id = std::stoul(token);
			listID.push_back(id);
		}
	}
}

void PQUtils::drawBox()
{
	//绘制函数
	//pq接口说明
	//HRESULT Doc_draw_cylinder(DOUBLE* i_dLineStart, INT i_nStartCount,DOUBL E* i_dLineEnd, INT i_nEndCount, DOUBLE i_radius, DOUBLE* i_dColorRGB, I NT i_nColorCount, ULONG i_uCoordinateID,ULONG* o_uCylinderID, BOOL i_bIs Update)
	//Parameters:  i_dLineStart 输入线的起点坐标 xyz
	//i_nStartCount 起点位置数组长度,一般为 3 
	//i_dLineEnd 输入线的终点坐标 xyz  i_nEndCount 终点位置数组长度,一般为 3 
	//i_radius 线的粗细,最大为 64  
	//i_dColorRGB 线的颜色 RGB(0-255)三色数组
	//i_nColorCount 传入颜色数组的长度,一般为 3  
	//i_uCoordinateID 传入位置参考坐标系,若为世界坐标系传入 0  
	//o_uCylinderID 传出画线对象 id  
	//i_bIsUpdate 是否刷新绘图区,若循环插入多条线,设置成 false,最后一条线设置  为 True 刷新,或者所有线生成后调用 Doc_update_view 刷新一次即 可,减少计算时间
	//示例void MainWindow:: DrawLine () {  double start[3] = {0.0}; double dEnd[3] = {100,100,100}; double dRGB[3] = { 255, 0, 0 }; ULONG i_uCoordinateID = 0; ULONG o_uCylinderID = 0; m_ptrKit ->Doc_draw_cylinder(start,3, dEnd,3,16, dRGB,3,i_uCoordinateID,&o_uCylinderID,false); }
	double start[3] = { 0.0 }; 
	double dEnd[3] = { 100,100,100 }; 
	double dRGB[3] = { 255, 0, 0 }; 
	ULONG i_uCoordinateID = 0;
	ULONG o_uCylinderID = 0; 
	m_ptrKit->Doc_draw_cylinder(start, 3, dEnd, 3, 16, dRGB, 3, i_uCoordinateID, &o_uCylinderID, false);
}

RobMathUtils::RobMathUtils()
{
}

RobMathUtils::~RobMathUtils()
{
}

trajPoint RobMathUtils::homoTransform(const trajPoint& point, const Eigen::Matrix4d& T)
{
	trajPoint result = point;

	// 1) 位置齐次变换
	const Eigen::Vector4d p(point.x, point.y, point.z, 1.0);
	const Eigen::Vector4d pTrans = T * p;
	result.x = pTrans.x();
	result.y = pTrans.y();
	result.z = pTrans.z();

	// 2) 姿态变换（假定 trajPoint 四元数顺序为 w, x, y, z）
	Eigen::Quaterniond q(point.q1, point.q2, point.q3, point.q4);
	if (q.norm() < 1e-12)
	{
		q = Eigen::Quaterniond::Identity();
	}
	else
	{
		q.normalize();
	}

	const Eigen::Matrix3d R = T.block<3, 3>(0, 0);
	Eigen::Quaterniond qT(R);
	qT.normalize();

	Eigen::Quaterniond qNew = qT * q;
	qNew.normalize();

	result.q1 = qNew.w();
	result.q2 = qNew.x();
	result.q3 = qNew.y();
	result.q4 = qNew.z();

	return result;
}

void RobMathUtils::exportToCSV(const std::vector<trajPoint>& points, const std::string& filename, bool isOutPutID)
{
	std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
		return;

	ofs << std::fixed << std::setprecision(10);

	if (isOutPutID)
		ofs << "id,x,y,z,q1,q2,q3,q4\n";

	for (const auto& p : points)
	{
		if (isOutPutID)
		{
			ofs << p.id << ","
				<< p.x << "," << p.y << "," << p.z << ","
				<< p.q1 << "," << p.q2 << "," << p.q3 << "," << p.q4 << "\n";
		}
		else
		{
			ofs << p.x << "," << p.y << "," << p.z  << "\n";
		}
	}
}

void RobMathUtils::exportToCSV(const Eigen::MatrixX3d points, const std::string& filename)
{
	std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
		return;

	ofs << std::fixed << std::setprecision(10);

	for (int i = 0; i < points.rows(); ++i)
	{
		ofs << points(i, 0) << ","
			<< points(i, 1) << ","
			<< points(i, 2) << "\n";
	}
}

Eigen::Matrix4d RobMathUtils::inv(Eigen::MatrixX4d inputMat)
{
	return inputMat.inverse();
}
