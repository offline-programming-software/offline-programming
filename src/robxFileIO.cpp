#include "robxFileIO.h"
#include <filesystem>
#include <fstream>
#include <qwidget.h>
#include <qmessagebox.h>
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
using json = nlohmann::json;

RobxIO::RobxIO()
{

	std::filesystem::create_directories(m_tempDir);
}

RobxIO::~RobxIO()
{
	// 不自动写回，避免误覆盖
}

// ========================
// ✅ 写入 temp/correctionList.json
// ========================
void RobxIO::writeData(const QVector<Correction>& list,
	const std::string& fileName)
{
	json j = json::array();

	for (const auto& c : list) {
		json jc;
		jc = {
		{"name", c.m_name.toStdString()},
		{"interType", static_cast<int>(c.m_interType)},
		{"isPosCorrect", c.m_isPosCorrect},
		{"range", c.m_range},
		{"flagPoints", c.m_flagPoints},
		{"measurePoints", c.m_measurePoints},
		{"isApply", c.m_isApply}
		};
		j.push_back(jc);
	}

	std::string fullPath = m_tempDir + "/" + fileName;

	std::ofstream ofs(fullPath);
	ofs << j.dump(4);
	ofs.close();
}

void RobxIO::writeData(const QVector<workSpace>& list, const std::string & fileName)
{
	json j = json::array();

	for (const auto& ws : list) {
		json jws;
		jws = {
			{"robotID", ws.robotID},
			{"thickness", ws.thickness},
			{"theta", ws.theta},
			{"points", ws.points}
		};
		j.push_back(jws);
	}

	std::string fullPath = m_tempDir + "/" + fileName;
	std::ofstream ofs(fullPath);
	ofs << j.dump(4);
	ofs.close();
}

void RobxIO::writeData(const QVector<workSpaceInformation>& list,
	const std::string& fileName)
{
	json j = json::array();

	for (const auto& wsInfo : list) {
		json jws;
		jws = {
			{"robotName", wsInfo.robotName.toStdString()},
			{"number", wsInfo.number},
			{"coodinate", wsInfo.coodinate.toStdString()},
			{"mainDir", wsInfo.mainDir.toStdString()},
			{"isLink",wsInfo.isLink},
			{"railName", wsInfo.railName.toStdString()}
		};
		j.push_back(jws);
	}

	std::string fullPath = m_tempDir + "/" + fileName;
	std::ofstream ofs(fullPath);
	ofs << j.dump(5);
	ofs.close();
}

// ========================
// ✅ 从 temp/correctionList.json 读取
// ========================
void RobxIO::updateData(QVector<Correction>& list,
	const std::string& fileName)
{
	std::string fullPath = m_tempDir + "/" + fileName;

	std::ifstream ifs(fullPath);
	if (!ifs.is_open())
		return;

	json j;
	ifs >> j;
	ifs.close();

	list.clear();
	for (const auto& item : j)
	{
		Correction c;
		c.m_name = QString::fromStdString(item.at("name").get<std::string>());
		c.m_interType = static_cast<Correction::interpolationType>(item.at("interType").get<int>());
		c.m_isPosCorrect = item.at("isPosCorrect").get<bool>();
		c.m_range = item.at("range").get<std::array<double, 6>>();
		c.m_flagPoints = item.at("flagPoints").get<std::vector<double>>();
		c.m_measurePoints = item.at("measurePoints").get<std::vector<double>>();
		c.m_isApply = item.at("isApply").get<bool>();
		list.push_back(c);
	}
}

void RobxIO::updateData(QVector<workSpace>& list, const std::string & fileName)
{
	std::string fullPath = m_tempDir + "/" + fileName;
	std::ifstream ifs(fullPath); if (!ifs.is_open())
		return;
	json j;
	ifs >> j;
	ifs.close();

	list.clear();
	for (const auto& item : j) {
		workSpace ws;

		ws.robotID = item.at("robotID").get<ULONG>();
		ws.thickness = item.at("thickness").get<double>();
		ws.theta = item.at("theta").get<double>();
		ws.points = item.at("points").get<std::vector<double>>();
		list.push_back(ws);
	}
		
}

void RobxIO::updateData(QVector<workSpaceInformation>& list,
	const std::string& fileName)
{
	std::string fullPath = m_tempDir + "/" + fileName;

	std::ifstream ifs(fullPath);
	if (!ifs.is_open())
		return;

	json j;
	ifs >> j;
	ifs.close();

	list.clear();
	for (const auto& item : j)
	{
		workSpaceInformation wsInfo;
		wsInfo.robotName = QString::fromStdString(item.at("robotName").get<std::string>());
		wsInfo.number = item.at("number").get<int>();
		wsInfo.coodinate = QString::fromStdString(item.at("coodinate").get<std::string>());
		wsInfo.mainDir = QString::fromStdString(item.at("mainDir").get<std::string>());
		wsInfo.isLink = item.at("isLink").get<bool>();
		wsInfo.railName = QString::fromStdString(item.at("railName").get<std::string>());
		list.push_back(wsInfo);
	}
}


//void RobxIO::flushTempToRobx()
//{
//	struct archive* a = archive_write_new();
//	archive_write_set_format_zip(a);
//	archive_write_open_filename(a, m_robxPath.c_str());
//
//	for (const auto& entry :
//		std::filesystem::directory_iterator(m_tempDir))
//	{
//		struct archive_entry* e = archive_entry_new();
//
//		std::string filePath = entry.path().string();
//		std::string fileName = entry.path().filename().string();
//
//		archive_entry_set_pathname(e, fileName.c_str());
//		archive_entry_set_size(e, std::filesystem::file_size(entry));
//		archive_entry_set_filetype(e, AE_IFREG);
//		archive_entry_set_perm(e, 0644);
//
//		archive_write_header(a, e);
//
//		std::ifstream ifs(filePath, std::ios::binary);
//		std::string buffer(
//			(std::istreambuf_iterator<char>(ifs)),
//			std::istreambuf_iterator<char>());
//
//		archive_write_data(a, buffer.data(), buffer.size());
//
//		archive_entry_free(e);
//	}
//
//	archive_write_close(a);
//	archive_write_free(a);
//}


namespace fs = std::filesystem;

void RobxFileIO::uploadJson(std::wstring& robxPath)
{
	std::vector<std::string> list = { "temp/" };
	std::string tempPath = "./temp";

	try {
		if (fs::exists(robxPath) && fs::is_regular_file(robxPath)) {
			fs::remove(robxPath);  // 删除单个文件
			std::cout << "文件删除成功\n";
		}
		else {
			std::cout << "文件不存在或不是普通文件\n";
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "错误: " << e.what() << '\n';
	}

	writeRobx(robxPath, list);

	try {
		if (fs::exists(tempPath) && fs::is_directory(tempPath)) {
			std::uintmax_t n = fs::remove_all(tempPath);  // 删除目录及所有子文件/子目录
			std::cout << "删除了 " << n << " 个文件/目录\n";
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << e.what() << '\n';
	}
}

void RobxFileIO::downloadJson(std::wstring& robxPath)
{
	std::string path = "temp";
	std::filesystem::create_directory(path);
	readRobx(robxPath, path);
}

void RobxFileIO::setPath(std::wstring path)
{
	GlobalPath() = path;
}

std::wstring & RobxFileIO::GlobalPath()   //引用作为返回值
{
	static std::wstring path;
	return path;
}

void addFileToArchive(struct archive* a, const fs::path& filePath, const fs::path& baseDir) {
	struct archive_entry* entry = archive_entry_new();
	fs::path relativePath = fs::relative(filePath, baseDir); // 保留文件夹结构
	archive_entry_set_pathname(entry, relativePath.string().c_str());
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);

	std::ifstream ifs(filePath, std::ios::binary | std::ios::ate);
	size_t size = ifs.tellg();
	ifs.seekg(0);

	archive_entry_set_size(entry, size);
	archive_write_header(a, entry);

	std::vector<char> buf(size);
	ifs.read(buf.data(), size);
	archive_write_data(a, buf.data(), size);

	archive_entry_free(entry);
}

// 添加目录到 archive（STORE）
void addDirToArchive(struct archive* a, const fs::path& dirPath, const fs::path& baseDir) {
	struct archive_entry* entry = archive_entry_new();
	fs::path relativePath = fs::relative(dirPath, baseDir);
	archive_entry_set_pathname(entry, relativePath.string().c_str());
	archive_entry_set_filetype(entry, AE_IFDIR);
	archive_entry_set_perm(entry, 0755);
	archive_entry_set_size(entry, 0); // 目录必须 size=0
	archive_write_header(a, entry);
	archive_entry_free(entry);
}

std::wstring to_wide_string(const std::string& input)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(input);
}
// 主函数：写 .robx
void writeRobx(const std::wstring& outname, const std::vector<std::string>& dirList) {
	

	struct archive* a = archive_write_new();
	archive_write_set_format_zip(a);
	// 文件使用 Deflate 最大压缩

	archive_write_set_filter_option(a, "zip", "compression-level", "9");
	archive_write_open_filename_w(a, outname.c_str());

	for (const auto& dir : dirList) {
		fs::path folder(dir);

		if (!fs::exists(folder) || !fs::is_directory(folder))
			continue;

		// 外层目录使用 STORE
		addDirToArchive(a, folder, folder.parent_path());

		// 递归添加内容
		for (auto& p : fs::recursive_directory_iterator(folder)) {
			if (fs::is_directory(p.path())) {
				addDirToArchive(a, p.path(), folder.parent_path());
			}
			else if (fs::is_regular_file(p.path())) {
				addFileToArchive(a, p.path(), folder.parent_path());
			}
		}
	}

	archive_write_close(a);
	archive_write_free(a);
}


void readRobx(const std::wstring& robxName, const std::string targetPath) {
	struct archive* a = archive_read_new();
	archive_read_support_format_zip(a);   // 支持 ZIP 格式
	archive_read_support_filter_all(a);   // 支持所有过滤器（Deflate/Store 等）

	if (archive_read_open_filename_w(a, robxName.c_str(), 10240) != ARCHIVE_OK) {
	
		archive_read_free(a);
		return;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		std::string outPath = targetPath + "/" + archive_entry_pathname(entry);

		fs::path outFsPath(outPath);

		if (archive_entry_filetype(entry) == AE_IFDIR) {
			fs::create_directories(outFsPath); // 创建目录
		}
		else {
			if (outFsPath.has_parent_path()) {
				fs::create_directories(outFsPath.parent_path()); // 确保父目录存在
			}
			std::ofstream ofs(outFsPath, std::ios::binary);
			const void* buff;
			size_t size;
			la_int64_t offset;
			while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
				ofs.seekp(offset);
				ofs.write(static_cast<const char*>(buff), size);
			}
			ofs.close();
		}
	}

	archive_read_close(a);
	archive_read_free(a);
}

