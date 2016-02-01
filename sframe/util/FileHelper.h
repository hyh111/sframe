
#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#include <string>

namespace sframe {

class FileHelper
{
public:

	// 读取文件所有内容
	static bool ReadFile(const std::string & full_name, std::string & content);

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
};

}

#endif
