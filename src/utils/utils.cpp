#pragma once
#include "utils.h"
#include "PQKitCallback.h"

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



