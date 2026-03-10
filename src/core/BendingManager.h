#pragma once
#include <vector>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include <comdef.h>
#include <array>
#include "core\Correction.h"
#include "model\CorrectionModel.h"
#include <QVector>	

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
	//getter & setter
	



public:
	void allocatePointsToCorrectionModels();
	void applyOffsets();
	void applyPosture(const std::vector<trajPoint>& Points);
	/**
	 * \fn getID.
	 * 
	 * \brief 按对象类型获取ID
	 *
	 * \param[out] listID
	 * \param[in] PQDAtatype
	 */
	std::vector<ULONG> m_vAllTrajPointIDs/**所有轨迹点的ID*/;
	std::vector<ULONG> m_vAllPathIDs;/**所有路径的ID*/
	std::vector<trajPoint> m_vAllPointInfo;
	CorrectionModel* m_correctionModel;
	
private:
	CComPtr<IPQPlatformComponent> m_ptrKit;

};

