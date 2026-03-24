#include "BendingManager.h"
#include <atlstr.h>
#include<QDebug>
#include <limits> 

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
	RobMathUtils mathUtils;
	bool isApply = cor.isApplied();
	auto ChildrenCorrections = cor.findChildren();
	Correction *ParentCorrection = cor.findParent();
	if(isApply)
	{
		//应用修正
		if (ParentCorrection) {
			//如果有父修正，先计算等效作用域，再分配轨迹点
			std::array<edgePoint, 8> eqRange = calEqRange(*ParentCorrection, cor);
		}
		else
		{
			//没有父修正，直接分配轨迹点
			allocatePoints(cor);
			cor.calOffset();  //calculate The Coeffs
			Eigen:Matrix4d TOB = mathUtils.inv(cor.m_TBO);
	    	for(size_t i = 0; i < cor.m_newTrajPoints.size(); ++i)
			 {
				 const auto& pt = cor.m_newTrajPoints[i];   
				 m_utils->setTrajPointPosture(pt.id, {pt.x, pt.y, pt.z, pt.q1, pt.q2, pt.q3, pt.q4});
			 }

			
			
		}
		qDebug()<<"apply correction: "<<cor.name();
		qDebug() << "apply m_newTrajsPoints, PointsCount = " << cor.m_newTrajPoints.size();
	}
	else
	{
		for(size_t i = 0; i < cor.m_originTrajPoints.size(); ++i)
		 {
			 const auto& pt = cor.m_originTrajPoints[i];   
			 m_utils->setTrajPointPosture(pt.id, {pt.x, pt.y, pt.z, pt.q1, pt.q2, pt.q3, pt.q4});
		 }
		qDebug() << "undo correction: " << cor.name();
		qDebug() << "apply m_originTrajPoints , PointsCount = " << cor.m_originTrajPoints.size();

	}
}

void BendingManager::rebuildParentChildRelation()
{
	auto& corList = m_correctionModel->getItems();
	if (corList.isEmpty())
		return;

	struct RangeBox
	{
		double xMin, xMax;
		double yMin, yMax;
		double zMin, zMax;
	};

	auto makeBox = [](const std::array<double, 6>& r) -> RangeBox
		{
			RangeBox b;
			b.xMin = (std::min)(r[0], r[1]);
			b.xMax = (std::max)(r[0], r[1]);
			b.yMin = (std::min)(r[2], r[3]);
			b.yMax = (std::max)(r[2], r[3]);
			b.zMin = (std::min)(r[4], r[5]);
			b.zMax = (std::max)(r[4], r[5]);
			return b;
		};

	auto isZeroBox = [](const RangeBox& b) -> bool
		{
			return b.xMin == 0.0 && b.xMax == 0.0
				&& b.yMin == 0.0 && b.yMax == 0.0
				&& b.zMin == 0.0 && b.zMax == 0.0;
		};

	auto contains = [&](const RangeBox& parent, const RangeBox& child) -> bool
		{
			if (isZeroBox(parent) || isZeroBox(child))
				return false;

			const bool inX = parent.xMin <= child.xMin && parent.xMax >= child.xMax;
			const bool inY = parent.yMin <= child.yMin && parent.yMax >= child.yMax;
			const bool inZ = parent.zMin <= child.zMin && parent.zMax >= child.zMax;

			// 完全相同范围不建立父子，避免歧义
			const bool exactlySame =
				parent.xMin == child.xMin && parent.xMax == child.xMax &&
				parent.yMin == child.yMin && parent.yMax == child.yMax &&
				parent.zMin == child.zMin && parent.zMax == child.zMax;

			return inX && inY && inZ && !exactlySame;
		};

	auto volume = [](const RangeBox& b) -> double
		{
			return (b.xMax - b.xMin) * (b.yMax - b.yMin) * (b.zMax - b.zMin);
		};

	// 1) 先清空旧关系
	for (int i = 0; i < corList.size(); ++i)
	{
		corList[i].m_parentCorrection = nullptr;
		corList[i].m_childCorrections.clear();
	}

	// 2) 预计算所有 box
	std::vector<RangeBox> boxes;
	boxes.reserve(static_cast<size_t>(corList.size()));
	for (int i = 0; i < corList.size(); ++i)
	{
		boxes.push_back(makeBox(corList[i].range()));
	}

	// 3) 为每个节点找“最小包含父节点”
	std::vector<int> parentIndex(static_cast<size_t>(corList.size()), -1);
	for (int childIdx = 0; childIdx < corList.size(); ++childIdx)
	{
		double bestVol = (std::numeric_limits<double>::max)();
		int bestParent = -1;

		for (int candParentIdx = 0; candParentIdx < corList.size(); ++candParentIdx)
		{
			if (candParentIdx == childIdx)
				continue;

			if (!contains(boxes[static_cast<size_t>(candParentIdx)], boxes[static_cast<size_t>(childIdx)]))
				continue;

			const double v = volume(boxes[static_cast<size_t>(candParentIdx)]);
			if (v < bestVol)
			{
				bestVol = v;
				bestParent = candParentIdx;
			}
		}

		parentIndex[static_cast<size_t>(childIdx)] = bestParent;
	}

	// 4) 回填父子指针
	Correction* dataPtr = corList.data(); // 获取底层连续内存的裸指针，避免 operator[] 触发任何隐式共享分离
	for (int i = 0; i < corList.size(); ++i)
	{
		const int p = parentIndex[static_cast<size_t>(i)];
		if (p >= 0)
		{
			dataPtr[i].m_parentCorrection = &dataPtr[p];
			dataPtr[p].m_childCorrections.push_back(&dataPtr[i]);
		}
	}
	qDebug() << "rebuildParentChildRelation done ";
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

std::array<edgePoint, 8> BendingManager::calEqRange(const Correction& parent, const Correction& child)
{
	//计算等效作用域，输入父对象，子对象，计算一个新的范围，使得在这个范围内应用父对象的修正和应用子对象的修正对轨迹点的影响尽可能接近（但不保证完全相同）。这个函数用于当一个correction有父correction时，计算一个新的作用域范围，以便在这个范围内分配轨迹点，达到更合理的修正效果。
	//大致步骤：
	//1) 先取出子对象的range范围，把八个角点拿出来
	//2) 对每个角点，转化成trajPoint的形式（姿态信息全部为0，id也为0）
	//3) 把每个角点先应用父对象的修正，得到一组新的点A
	//4) 把角点再转换回edgePoint的形式，得到一组新的角点B，计算等效作用域完成
	const std::array<double, 6> range = child.range();

	const double xMin = (std::min)(range[0], range[1]);
	const double xMax = (std::max)(range[0], range[1]);
	const double yMin = (std::min)(range[2], range[3]);
	const double yMax = (std::max)(range[2], range[3]);
	const double zMin = (std::min)(range[4], range[5]);
	const double zMax = (std::max)(range[4], range[5]);

	std::array<edgePoint, 8> corners = { {
		{xMin, yMin, zMin},
		{xMax, yMin, zMin},
		{xMin, yMax, zMin},
		{xMax, yMax, zMin},
		{xMin, yMin, zMax},
		{xMax, yMin, zMax},
		{xMin, yMax, zMax},
		{xMax, yMax, zMax}
	} };

	//2) 对每个角点，转化成trajPoint的形式（姿态信息全部为0，id也为0）
	std::vector<trajPoint> pts;
	pts.reserve(8);
	for (int i = 0; i < 8; ++i)
	{
		trajPoint pt;
		pt.id = 0;
		pt.x = corners[i].x;
		pt.y = corners[i].y;
		pt.z = corners[i].z;
		pt.q1 = 0.0;
		pt.q2 = 0.0;
		pt.q3 = 0.0;
		pt.q4 = 0.0;
		pts.push_back(pt);
	}

	//3) 把每个角点先应用父对象的修正，得到一组新的点A
	// 利用 parent 的复制体来计算偏移，以免改变 const 的父对象
	Correction parentCopy = parent;
	parentCopy.m_originTrajPoints = pts;
	parentCopy.calOffset();

	//4) 把角点再转换回edgePoint的形式，得到一组新的角点B，计算等效作用域完成
	std::array<edgePoint, 8> newCorners;
	for (int i = 0; i < 8; ++i)
	{
		newCorners[i].x = parentCopy.m_newTrajPoints[i].x;
		newCorners[i].y = parentCopy.m_newTrajPoints[i].y;
		newCorners[i].z = parentCopy.m_newTrajPoints[i].z;
	}

	//打印一个调试信息，显示原始角点和新的角点
	qDebug() << "calEqRange for child correction:" << child.name() << " with parent correction:" << parent.name();
	for (int i = 0; i < 8; ++i)
	{
		qDebug() << "corner " << i << ": original(" << corners[i].x << "," << corners[i].y << "," << corners[i].z
			<< ") -> new(" << newCorners[i].x << "," << newCorners[i].y << "," << newCorners[i].z << ")";
	}

	return newCorners;
}

