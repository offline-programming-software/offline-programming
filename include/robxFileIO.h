#pragma once
#include <string>
#include <qvector.h>
#include <archive.h>
#include <archive_entry.h>
#include <Correction.h>
#include <nlohmann/json.hpp>

#include "spaceCalculate.h"
/**
 * @class RobxIO
 * @brief 用于读取、写入、更新 .robx 数据的 IO 类。
 *
 * 负责 robx 目录路径管理、数据写入/更新操作，以及初始化时的资源准备。
 *
 * * **依赖库**
 *		1. libarchive （解压与创建 robx 包）
 *		2. filesystem （文件与目录操作,需要C++17）
 *		3. nlohmann/json
 * * **安装依赖**\n
 * _shell_
 * @code
 * vcpkg install libarchive
 * vcpkg install nlohmann-json
 * @endcode
 *
 * * **使用示例**\n
 * @code
 *  // 创建 IO 操作对象
 *  m_io = new RobxIO();
 *  // 写入 JSON 至 robx 临时目录，在析构时调用
 *  io.writeData(dataSetList, "dataSet.json");
 *  // 从robx中读取JSON文件，更新本地QList中的数据，在构造函数中调用
 *  io.updateData(dataSetList, "dataSet.json");
 * @endcode
 *
 * * **准备工作**\n
 * 由于加入了新的数据结构类型，需要在读写接口的源文件中重载特定类型的读写规则。\n
 * 例如下面的数据类型dataSet
 * @code
 *  //myModule.h
 *  //例如你定义了新的数据结构（对象、结构体均可）
 *  class dataSet{
 *  public:
 *		string m_name;
 *		bool m_flag;
 *		vector<double> m_points;
 *		double m_ranges;
 *  }
 *  //myModule.cpp
 *  //在代码运行过程中，产生了几个实例，将这些实例存储到QList中
 *  QList<dataSet> dataSetList;
 *  dataSet d;
 *  d.m_name = "test";
 *  d.m_flag = true;
 *  d.m_points = { 0.0 , 0.1, 0.2};
 *  d.m_ranges = 1.0;
 *  dataSetList.append(d);
 * @endcode
 * 接下来你需要做：
 * @code
 *  //在robxFileIO.h  robxFileIO.cpp中重载读写函数
 *  void writeData(const QVector<dataSet>& list , const std::string& fileName)
 *	{
 *		...
 *			jc = {
 *					{"name", d.m_name.toStdString()},
 *					{"flag", d.m_flag},
 *					{"point", d.m_points},
 *					{"ranges", d.m_ranges},

 *			    	};
 *      ...
 *  }
 *	void updateData(QVector<dataSet>& list , const std::string& fileName);
 *	{
 *		//见模板
 *  }
 *  @endcode
 *	准备工作完成\n
 *
 */


class RobxIO
{
public:
	RobxIO();
	~RobxIO();

public:
	/**
	 * @brief 向robx文件中写入数据
	 *
	 * @param [in] list 要写入的数据List
	 * @param [in] fileName 写进目标robx文件对应的json文件名称
	 *
	 * @details
	 * 一个writeData/updateData函数对应一种数据类型, 本函数对应数据类型是correction对象
	 * 函数中包含了输入数据向json的转换规则，如果需要保存新的数据类型需要重载writeData函数
	 */
	void writeData(const QVector<Correction>& list,
		const std::string& fileName);

	void writeData(const QVector<RobotWorkspaceBoundary>& list,
		const std::string& fileName);

	void writeData(const QVector<workSpaceInformation>& list,
		const std::string& fileName);
	
	void writeData(QVector<std::tuple<QString, QString, QString>>& list,
		const std::string& fileName);
	/**
	  * @brief 从robx文件中读取数据
	  * 
	  * @param [out] list 要读入数据的QList
	  * @param [in] fileName robx文件名称
	  */
	void updateData(QVector<Correction>& list,
		const std::string& fileName);

	void updateData(QVector<RobotWorkspaceBoundary>& list,
		const std::string& fileName);

	void updateData(QVector<workSpaceInformation>& list,
		const std::string& fileName);

<<<<<<< HEAD
	void updateData(QVector<std::tuple<QString, QString, QString>>& list,
		const std::string& fileName);

	//void writeData(const QVector<鏂扮殑鏁版嵁缁撴瀯绫诲瀷>& list,
=======
	//void writeData(const QVector<新的数据结构类型>& list,
>>>>>>> origin/fix/RobxFileIO-error
	//	const std::string& fileName);

	//void updateData(QVector<新的数据结构类型>& list,
	//	const std::string& fileName);
	//

private:
	std::string m_robxPath;
	std::string m_tempDir = "./temp/jsons";
	std::string m_jsonName;

protected:

};

namespace RobxFileIO{
	/**
	 * @fn uploadJson.
	 * 
	 * @brief 将本地temp目录下的内容写入目标ROBX文件.
	 */
	void uploadJson(std::wstring& robxPath);

	/**
	 * @fn downloadJson
	 * 
	 * @brief 将目标ROBX文件的内容读取到本地temp目录下.
	 */
	void downloadJson(std::wstring& robxPath);
	/**
	 * @fn updateRobxData.
	 * 
	 * @brief 将保存后的robx文件的data部分更新写入到temp文件夹中
	 */
	void updateRobxData(std::wstring& robxPath);
	void setPath(std::wstring path);
	std::wstring& GlobalPath();
	void clearFolder(const std::wstring& path);


}
namespace fs = std::filesystem;

/**
 * @fn addFileToArchive.
 *
 * @brief 将单个文件写入 libarchive 归档对象。
 *
 * @details
 * - `filePath` 为待写入的真实文件路径；
 * - 归档内的相对路径由 `filePath` 相对于 `baseDir` 计算得到（保持目录结构）；
 * - 函数通常由 `addDirToArchive` 递归遍历目录时调用；
 * - 要求 `a` 已经通过 `archive_write_*` 正确初始化并打开输出。
 *
 * @param [in,out] a libarchive 归档写入句柄
 * @param [in] filePath 待写入的文件路径
 * @param [in] baseDir 用于计算归档内相对路径的基准目录
 */
void addFileToArchive(struct archive* a, const fs::path& filePath, const fs::path& baseDir);
/**
 * @brief 递归遍历目录并将其中所有内容写入 libarchive 归档对象。
 *
 * @details
 * - 遍历 `dirPath` 下的文件与子目录；
 * - 文件通过 `addFileToArchive` 写入；
 * - 归档内路径统一以 `baseDir` 作为相对路径基准，确保还原时目录结构一致。
 *
 * @param [in,out] a libarchive 归档写入句柄
 * @param [in] dirPath 待写入的目录路径
 * @param [in] baseDir 用于计算归档内相对路径的基准目 */
void addDirToArchive(struct archive* a, const fs::path& dirPath, const fs::path& baseDir);

void writeRobx(const std::wstring& outname, const std::vector<std::string>& dirList);
void readRobx(const std::wstring& robxName, const std::string targetPath);

