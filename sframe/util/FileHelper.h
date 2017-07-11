
#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#include <vector>
#include <string>

namespace sframe {

class FileHelper
{
public:

	// 读取文件所有内容
	static bool ReadFile(const std::string & full_name, std::string & content);

	// 写入文件
	static size_t WriteFile(const std::string & full_name, std::string & content);

	// 在全路劲中获取文件名
	static std::string GetFileName(const char * fullname);

	// 在全路劲中获取文件名
	static std::string GetFileName(const std::string & fullname)
	{
		return GetFileName(fullname.c_str());
	}

	// 去除文件扩展名
	static std::string RemoveExtension(const std::string & name);

	// 目录是否存在
	static bool DirectoryExisted(const std::string & path);

	// 创建目录
	static bool MakeDirectory(const std::string & path);

	// 递归创建
	static bool MakeDirectoryRecursive(const std::string & path);

	enum ScanType
	{
		kScanType_All,
		kScanType_OnlyDirectory,
		kScanType_OnlyNotDirectory,
	};

	// 扫描目录
	// dir_path   :   目录路径，不能包含通配符
	// match_name :   名字匹配，支持通配符，比如要扫描 /data 目录下，所有符合*.cpp的名字的内容，调用ScanDirectory("/data", "*.cpp")
	static std::vector<std::string> ScanDirectory(const std::string & dir_path, const std::string & match_name = "", ScanType scan_type = kScanType_All);

	// 展开通配符(* ?)
	// path: 路径，最后是否以/结尾，表示文件，否则为目录
	// parent_dir: 所在目录
	static std::vector<std::string> ExpandWildcard(const std::string & path, const std::string & parent_dir = "");
};

}

#endif
