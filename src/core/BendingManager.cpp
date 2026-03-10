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

