#include "BendingManager.h"
#include <atlstr.h>
#include<QDebug>

BendingManager::BendingManager(CComPtr<IPQPlatformComponent> ptrKit,
	CorrectionModel* model)
	: m_ptrKit(ptrKit),
	m_correctionModel(model)
{

}

BendingManager::~BendingManager()
{

}

void BendingManager::getAllPointInfo()
{
	INT count;
	ULONG* pointID;
	//获取所有路径信息
	getID(m_vAllPathIDs, PQ_PATH);
	//获取轨迹点信息
	for (int i = 0 ; i < m_vAllPathIDs.size(); i++)
	{
		//当前路径轨迹点个数	
		m_ptrKit->Path_get_point_count(m_vAllPathIDs[i], &count);
		//获取轨迹点id
		m_ptrKit->Path_get_point_id(m_vAllPathIDs[i], &count, &pointID);
		for (int j = 0; j < count; j++)
			m_vAllTrajPointIDs.push_back(pointID[j]);
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)pointID);
		//获取轨迹点位姿
		int nPostureSize = 0;
		double* dPosture = nullptr;
		m_ptrKit->Point_get_posture_batch(m_vAllPathIDs[i],0,count,QUATERNION,0,&nPostureSize,&dPosture);
		std::vector<double> pos;
		int nCount = 0;
		for (int i = 0; i < nPostureSize; i++)
		{
			pos.push_back(dPosture[i]);
			if ((i+1) % 7 == 0)
			{
				trajPoint point;
				m_vAllPointInfo.push_back(point);
				m_vAllPointInfo[nCount].x = pos[0];
				m_vAllPointInfo[nCount].y= pos[1];
				m_vAllPointInfo[nCount].z = pos[2];
				m_vAllPointInfo[nCount].q1 = pos[3];
				m_vAllPointInfo[nCount].q2 = pos[4];
				m_vAllPointInfo[nCount].q3 = pos[5];
				m_vAllPointInfo[nCount].q4 = pos[6];
				pos.clear();
				nCount++;
			}

		}
		m_ptrKit->PQAPIFreeArray((LONG_PTR*)dPosture);

	}
	for (int i = 0; i < m_vAllTrajPointIDs.size(); i++)
	{
		m_vAllPointInfo[i].id = m_vAllTrajPointIDs[i];
	}

}


void BendingManager::allocatePointsToCorrectionModels()
{
	for(auto i: m_correctionModel->getItems())
	{
		std::vector<ULONG> trajPointIDsToCorrect;
		for (int j = 0; j < m_vAllPointInfo.size(); j++)
		{
			if (m_vAllPointInfo[j].x >= i.rang(0) && m_vAllPointInfo[j].x <= i.rang(1) &&
				m_vAllPointInfo[j].y >= i.rang(2) && m_vAllPointInfo[j].y <= i.rang(3) &&
				m_vAllPointInfo[j].z >= i.rang(4) && m_vAllPointInfo[j].z <= i.rang(5))
			{
				trajPointIDsToCorrect.push_back(m_vAllPointInfo[j].id);
			}
		}
	}
}

void BendingManager::applyOffsets()
{

}

void BendingManager::applyPosture(const std::vector<trajPoint>& Points)
{
	DOUBLE dPos[7];
	ULONG pointID;
	INT PositionCount = 7;

	for (auto p : Points)
	{
		dPos[0] = p.x;
		dPos[1] = p.y;
		dPos[2] = p.z;
		dPos[3] = p.q1;
		dPos[4] = p.q2;
		dPos[5] = p.q3;
		dPos[6] = p.q4;
	}
}

void BendingManager::getID(std::vector<ULONG>& listID, __MIDL___MIDL_itf_RPC_0000_0000_0005 PQDAtatype)
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



