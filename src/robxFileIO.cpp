#include "robxFileIO.h"
#include <filesystem>
#include <fstream>
#include <qwidget.h>
#include <qmessagebox.h>
#include <archive.h>
#include <archive_entry.h>
#include<iostream>
#include<Windows.h>
using json = nlohmann::json;

RobxIO::RobxIO()
{

	std::filesystem::create_directories(m_tempDir);
}

RobxIO::~RobxIO()
{
	
}

// ========================
// ? 写入 temp/correctionList.json
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
// ? 从 temp/correctionList.json 读取
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

// 将 wstring 转换为 UTF-8 编码的 std::string
std::string to_utf8_from_wide(const std::wstring& input)
{
	if (input.empty())
		return "";

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

void addFileToArchive(struct archive* a, const fs::path& filePath, const fs::path& baseDir) {
	struct archive_entry* entry = archive_entry_new();
	fs::path relativePath = fs::relative(filePath, baseDir);
	
	// 转换为 UTF-8 编码的路径
	std::string utf8Path = to_utf8_from_wide(relativePath.wstring());
	archive_entry_set_pathname(entry, utf8Path.c_str());
	
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

void addDirToArchive(struct archive* a, const fs::path& dirPath, const fs::path& baseDir) {
	struct archive_entry* entry = archive_entry_new();
	fs::path relativePath = fs::relative(dirPath, baseDir);
	
	// 转换为 UTF-8 编码的路径
	std::string utf8Path = to_utf8_from_wide(relativePath.wstring());
	archive_entry_set_pathname(entry, utf8Path.c_str());
	
	archive_entry_set_filetype(entry, AE_IFDIR);
	archive_entry_set_perm(entry, 0755);
	archive_entry_set_size(entry, 0);
	archive_write_header(a, entry);
	archive_entry_free(entry);
}

std::wstring to_wide_string(const std::string& input)
{
    if (input.empty())
        return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string to_utf8_string(const char* utf8_str)
{
	if (utf8_str == nullptr)
		return "";
	return std::string(utf8_str);
}

// 主函数：写 
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
	archive_read_support_format_zip(a);
	archive_read_support_filter_all(a);

	if (archive_read_open_filename_w(a, robxName.c_str(), 10240) != ARCHIVE_OK) {
		archive_read_free(a);
		return;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		const char* pathname = archive_entry_pathname(entry);

		if (pathname == nullptr) {
			continue;
		}

		// 将 UTF-8 路径转换为 wstring，然后构建 fs::path
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, pathname, -1, nullptr, 0);
		std::wstring widePath(size_needed - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, pathname, -1, &widePath[0], size_needed);

		std::wstring targetPathW = to_wide_string(targetPath);
		std::wstring outPathW = targetPathW + L"/" + widePath;

		fs::path outFsPath(outPathW);

		if (archive_entry_filetype(entry) == AE_IFDIR) {
			fs::create_directories(outFsPath);
		}
		else {
			if (outFsPath.has_parent_path()) {
				fs::create_directories(outFsPath.parent_path());
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

