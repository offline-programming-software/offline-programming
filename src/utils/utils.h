#pragma once
#include<comdef.h>
#include<qstring.h>
#include<qstringlist.h>
#include<Eigen/dense>
#include<vector>
#include "PQKitInitThread.h"
#include "core/Correction.h"
#include "PQKitCallback.h"
#include "core/BendingManager.h"
#include "core/Correction.h"
#include <comdef.h>
struct edgePoint;
namespace utils {
	/** @brief 井号分割的BSTR如"轨迹1#轨迹2#轨迹3#..."转换为QStringlist */
	QStringList BSTR2QStringList(BSTR bstr);
//
///**
//  * @fn plotInCertainDirection
//  * @brief 画图函数，输入一个点集，绘制某一方向的视图
//  * @param[out] gr 图
//  * @param[in] pointData 点集
//  * @param[in] viewDir 视线投影的方向，单位向量

}

/**
 * @class RobMathUtils.
 * 
 * @brief 包含一些机器人学相关的工具函数，如齐次变换、CSV导出等.
 */
class RobMathUtils
{
public:
	RobMathUtils();
	~RobMathUtils();

	/**
	 * \brief 对单个点进行齐次坐标变换.
	 * 
	 * \param point 输入的轨迹点 （x,y,z,q1,q2,q3,q4,id）id是pq中轨迹点的id，没有就不写
	 * \param T 齐次变换矩阵
	 * \return trajPoint
	 */
	trajPoint homoTransform(const trajPoint& point, const Eigen::Matrix4d& T);
	void exportToCSV(const std::vector<trajPoint>& points, const std::string& filename,bool isOutPutID = false);
	void exportToCSV(const Eigen::MatrixX3d, const std::string& filename);
	Eigen::Matrix4d inv(Eigen::MatrixX4d inputMat);
};

class PQUtils
{
public:
	PQUtils(CComPtr<IPQPlatformComponent>pqkit);
	~PQUtils();
public:
	/**
	 * \brief 设置轨迹点的姿态.
	 * 
	 * \param[in] pointID 轨迹点ID
	 * \param[in] posture 轨迹点位姿，注意顺序为x,y,z,w,x,y,z
	 *
	 * \note 测试通过
	 */
	void setTrajPointPosture(ULONG pointID, const std::array<double, 7>& posture);

	/**
	 * \brief 获取所有轨迹点的位姿信息
	 * 
	 * \return 包含所有轨迹点信息的vector
	 * 
	 * \note 测试通过
	 */
	std::vector<trajPoint> getAllTrajPointPosture();

	/**
	 * \brief 获取PQ对象ID.
	 * 
	 * \param[out] listID 返回的ID列表
	 * \param[in] PQDAtatype 要获取的对象类型，如PQ_PATH、PQ_POINT等，详见PQ说明文档
	 * 
	 * \note 测试通过
	 */
	void getID(std::vector<ULONG>& listID, __MIDL___MIDL_itf_RPC_0000_0000_0005 PQDAtatype);

	/**
	 * \brief 根据xyz的最大最小值画一个方框.
	 * 
	 * \param range
	 */
	void drawBox(const std::array<double,6>& range);

	QString getName(const ulong id);

	ulong getPathIDOfPoint(ulong pointID);


	void drawBox(std::array<edgePoint, 8> pts);
private:
	CComPtr<IPQPlatformComponent> m_ptrKit;
};

