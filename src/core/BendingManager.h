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

/**
 * @class BendingManager.
 *
 * @brief 服务于BendingManagerWidget负责管理所有的轨迹点信息，计算偏移量，并将偏移量应用到轨迹点上。
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
	//1. 主函数入口，当correction对象被选中或取消选中时，调用这个函数来应用或撤销对应的修正
	//2. 
private:
	/**
	 * \brief 传入correction对象，为其分配待修正轨迹点.
	 */
	void allocatePoints(Correction& cor);
	/**
	 * \brief 计算等效作用域.
	 */
	std::array<double,6> calEqRange(const Correction& parent,const Correction& child);
private:
	CComPtr<IPQPlatformComponent> m_ptrKit;
	std::vector<trajPoint> m_pointsSnapshot; /**轨迹点快照，在应用修正前保存一份轨迹点数据，以便撤销修正时恢复*/
	CorrectionModel* m_correctionModel;
	PQUtils* m_utils = nullptr;

};

