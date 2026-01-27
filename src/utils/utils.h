#pragma once
#include<comdef.h>
#include<qstring.h>
#include<qstringlist.h>
#include<Eigen/dense>
#include<vector>

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
