#pragma once
#include <vector>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include <comdef.h>
#include <array>
#include "core\Correction.h"
#include "model\CorrectionModel.h"
#include <QVector>	
#include "utils/utils.h"

class PQUtils;
struct edgePoint {
	double x;
	double y;
	double z;
};
/**
 * @class BendingManager.
 *
 * @brief 服务于BendingManagerWidget负责管理所有的轨迹点信息，计算偏移量，并将偏移量应用到轨迹点上。
 * 
 * @details 核心思想是rebuild，
 * 
 * @relates BendingManagerWidget CorrectionModel Correction
 */
class BendingManager
{
public:
	BendingManager(CComPtr<IPQPlatformComponent> ptrKit,CorrectionModel *model);
	~BendingManager();

public:

	/**
	 * \brief 初始化轨迹点快照，在应用修正前保存一份轨迹点数据，以便撤销修正时恢复,
	 *		  同样用于为corrections提供轨迹点数据
	 */
	void initOriginPointsSnapshot();

	/**
	 * \brief 这个是主函数，当用户在树形视图中勾选或取消勾选某个修正对象时，调用这个函数来应用或撤销对应的修正
	 *		  同步修改pq内轨迹点。一次只处理一个correction对象，处理顺序由correctionModel中的corrections的顺序决定。
	 * \data trajPointstoCorrect >> cor
	 */
	void rebuildPoints(Correction& cor);

	/**
	 * \brief 重建当前输入对象的父子关系（根据作用域）.
	 * 
	 * \param cor
	 */
	void rebuildParentChildRelation();

	/**
	 * \brief 计算等效作用域.
	 */
	std::array<edgePoint,8> calEqRange(const Correction& parent,const Correction& child);
private:

	/**
	 * \brief 传入correction对象，为其分配待修正轨迹点.
	 */
	void allocatePoints(Correction& cor);

	void allocatePoints(Correction& cor,std::array<edgePoint,8> eqRange);
	
	/**
	 * \brief do a snapshot.
	 * 
	 * \return trajPoint Vector
	 */
	std::vector<trajPoint> snapShot();
private:
	CComPtr<IPQPlatformComponent> m_ptrKit;
	std::vector<trajPoint> m_pointsSnapshot; /**轨迹点快照，在应用修正前保存一份轨迹点数据，以便撤销修正时恢复*/
	CorrectionModel* m_correctionModel;
	PQUtils* m_utils = nullptr;

};

