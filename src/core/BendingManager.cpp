#include "BendingManager.h"
#include <atlstr.h>
#include<QDebug>

BendingManager::BendingManager(CComPtr<IPQPlatformComponent> ptrKit,
	CorrectionModel* model)
	: m_ptrKit(ptrKit),
	m_correctionModel(model)
{
	m_utils = new PQUtils(m_ptrKit);
}

BendingManager::~BendingManager()
{
	delete m_utils;
}



void BendingManager::initOriginPointsSnapshot()
{
	m_pointsSnapshot = m_utils->getAllTrajPointPosture();
	qDebug() << "init snapshot, point num: " << m_pointsSnapshot.size();
	for(auto pts:m_pointsSnapshot)
	{
		qDebug()<<"point id: "<<pts.id<<" posture: "<<pts.x<<" "<<pts.y<<" "<<pts.z<<" "<<pts.q1<<" "<<pts.q2<<" "<<pts.q3<<" "<<pts.q4;
	}
}

void BendingManager::rebuildPoints(Correction& cor)
{
	bool isApply = cor.isApplied();
	auto ChildrenCorrections = cor.findChildren();
	Correction *ParentCorrection = cor.findParent();
	if(isApply)
	{
		//应用修正
		if (ParentCorrection) {
			//如果有父修正，先计算等效作用域，再分配轨迹点
		}
		else
		{
			//没有父修正，直接分配轨迹点
			allocatePoints(cor);
			
		}
		qDebug()<<"apply correction: "<<cor.name();
	}
	else
	{
		//撤销修正
		qDebug() << "undo correction: " << cor.name();
	}
}

void BendingManager::allocatePoints(Correction& cor)
{
	//根据全局snapshot和cor的range分配轨迹点变量m_trajtocorrect
	//根据全局snapshot和cor的range分配轨迹点变量m_trajtocorrect
	if (m_pointsSnapshot.empty())
	{
		initOriginPointsSnapshot();
	}

	cor.m_newTrajPoints.clear();
	cor.m_originTrajPoints.clear();

	const std::array<double, 6> range = cor.range();

	const double xMin = (std::min)(range[0], range[1]);
	const double xMax = (std::max)(range[0], range[1]);
	const double yMin = (std::min)(range[2], range[3]);
	const double yMax = (std::max)(range[2], range[3]);
	const double zMin = (std::min)(range[4], range[5]);
	const double zMax = (std::max)(range[4], range[5]);

	// 约定：全0范围视为未定义
	if (xMin == 0.0 && xMax == 0.0 &&
		yMin == 0.0 && yMax == 0.0 &&
		zMin == 0.0 && zMax == 0.0)
	{
		qDebug() << "allocatePoints skip, invalid zero range for correction:" << cor.name();
		return;
	}

	for (const auto& pt : m_pointsSnapshot)
	{
		if (pt.x >= xMin && pt.x <= xMax &&
			pt.y >= yMin && pt.y <= yMax &&
			pt.z >= zMin && pt.z <= zMax)
		{
			cor.m_newTrajPoints.push_back(pt);
			cor.m_originTrajPoints.push_back(pt);
		}
	}

	qDebug() << "allocatePoints done, correction:" << cor.name()
		<< ", selected point num:" << cor.m_newTrajPoints.size();
}

