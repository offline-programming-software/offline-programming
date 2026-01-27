#pragma once
#include <vector>
#include "PQKitInitThread.h"
#include "PQKitCallback.h"
#include <comdef.h>
#include <array>
#include "core\Correction.h"
#include "model\CorrectionModel.h"
#include <QVector>	


class BendingManager
{
public:
	BendingManager(CComPtr<IPQPlatformComponent> ptrKit,CorrectionModel *model);
	~BendingManager();

public:
	//getter & setter
	



public:
	/**
	 * @fn getAllPointInfo
	 * 
	 */
	void getAllPointInfo();
	
	void allocatePointsToCorrectionModels();
	/**
	 * @fn calAllOffsets.
	 * 
	 * 为model中所有的轨迹点计算偏移量
	 */
	void applyOffsets();
	/**
	 * @fn correctPoints.
	 * 
	 * 批量修改轨迹点
	 */

	
	void applyPosture(const std::vector<trajPoint>& Points);
	void getID(std::vector<ULONG>& listID, __MIDL___MIDL_itf_RPC_0000_0000_0005 PQDAtatype);
	
private:
	CComPtr<IPQPlatformComponent> m_ptrKit;

	std::vector<ULONG> m_vAllTrajPointIDs/**所有轨迹点的ID*/;
	std::vector<ULONG> m_vAllPathIDs;/**所有路径的ID*/

	std::vector<trajPoint> m_vAllPointInfo;
	

	CorrectionModel* m_correctionModel;
	std::vector<ULONG> m_listPath;

};

