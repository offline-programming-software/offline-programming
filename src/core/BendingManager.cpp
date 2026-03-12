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



void BendingManager::initOriginPointsSnapshot()
{
}

