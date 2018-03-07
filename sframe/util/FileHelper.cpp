
#include <stdio.h>
#include <assert.h>
#include <utility>
#include "FileHelper.h"
#include "StringHelper.h"

#ifndef __GNUC__
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi")
#pragma warning(disable:4996)
#else
#include <sys/types.h> 
#include <sys/stat.h>
#include <dirent.h>
#endif

using namespace sframe;

// 读取文件所有内容
bool FileHelper::ReadFile(const std::string & full_name, std::string & content)
{
	FILE * f = fopen(full_name.c_str(), "r");
	if (!f)
	{
		return false;
	}

	if (fseek(f, 0L, SEEK_END) < 0)
	{
		fclose(f);
		return false;
	}

	long file_size = ftell(f);
	if (file_size <= 0)
	{
		fclose(f);
		return false;
	}

	std::string temp(file_size, '\0');
	char * p = &temp[0];

	if (fseek(f, 0L, SEEK_SET) < 0)
	{
		fclose(f);
		return false;
	}

	size_t s = fread(p, sizeof(char), file_size, f);
	if (s < (size_t)file_size)
	{
		temp.erase(temp.begin() + s, temp.end());
	}
	fclose(f);

	content = std::move(temp);

	return true;
}

// 写入文件
size_t FileHelper::WriteFile(const std::string & full_name, std::string & content)
{
	FILE * f = fopen(full_name.c_str(), "w");
	if (!f)
	{
		return 0;
	}

	size_t s = fwrite(content.c_str(), sizeof(char), content.length(), f);
	fclose(f);

	return s;
}

// 在全路劲中获取文件名
std::string FileHelper::GetFileName(const char * fullname)
{
	const char * file = fullname;
	const char * p = file;
	while (*p != '\0')
	{
		if (*p == '\\' || *p == '/')
		{
			file = p + 1;
		}

		p++;
	}

	return std::string(file);
}

// 去除文件扩展名
std::string FileHelper::RemoveExtension(const std::string & name)
{
	size_t pos = name.find_last_of('.');
	return (pos == std::string::npos ? name : name.substr(0, pos));
}

// 目录是否存在
bool FileHelper::DirectoryExisted(const std::string & path)
{
	bool result = false;

#ifndef __GNUC__
	result = PathIsDirectoryA(path.c_str()) ? true : false;
#else
	DIR * pdir = opendir(path.c_str());
	if (pdir != nullptr)
	{
		closedir(pdir);
		result = true;
	}
#endif

	return result;
}

// 创建目录
bool FileHelper::MakeDirectory(const std::string & path)
{
#ifndef __GNUC__
	return CreateDirectoryA(path.c_str(), nullptr) ? true : false;
#else
	return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
}

// 确保路径存在
bool FileHelper::MakeDirectoryRecursive(const std::string & path)
{
	if (DirectoryExisted(path))
	{
		return true;
	}

	std::string::size_type pos = path.find('/');

	while (pos != std::string::npos)
	{
		std::string cur = path.substr(0, pos - 0);

		if (cur.length() > 0 && !DirectoryExisted(cur))
		{
			if (!MakeDirectory(cur))
			{
				return false;
			}
		}

		pos = path.find('/', pos + 1);
	}

	return true;
}

#ifdef __GNUC__

static int SelectOnlyDir(const dirent * d)
{
	return d->d_type == DT_DIR;
}

static int SelectOnlyNotDir(const dirent * d)
{
	return d->d_type != DT_DIR;
}

#endif

// 扫描目录
// dir_path   :   目录路径，不能包含通配符
// match_name :   名字匹配，支持通配符，比如要扫描 /data 目录下，所有符合*.cpp的名字的内容，调用ScanDirectory("/data", "*.cpp")
std::vector<std::string> FileHelper::ScanDirectory(const std::string & dir_path, const std::string & match_name, ScanType scan_type)
{
	std::vector<std::string> vec;
	if (dir_path.empty())
	{
		return vec;
	}

#ifndef __GNUC__

	std::string find_path;
	find_path.reserve(dir_path.size() + 3);
	find_path.append(dir_path);
	find_path.append("/*");

	WIN32_FIND_DATAA find_data;
	HANDLE h_find = FindFirstFileA(find_path.c_str(), &find_data);

	if (INVALID_HANDLE_VALUE == h_find)
	{
		return vec;
	}

	while (true)
	{
		bool ok = true;

		switch (scan_type)
		{
		case sframe::FileHelper::kScanType_OnlyDirectory:
			ok = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			break;
		case sframe::FileHelper::kScanType_OnlyNotDirectory:
			ok = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
			break;
		}

		if (ok)
		{
			std::string file_name = find_data.cFileName;
			if (match_name.empty() || MatchWildcardStr(file_name, match_name, true))
			{
				vec.push_back(std::move(file_name));
			}
		}

		if (!FindNextFileA(h_find, &find_data))
		{
			break;
		}
	}
	FindClose(h_find);

#else

	int(*selector) (const struct dirent *) = NULL;
	switch (scan_type)
	{
	case sframe::FileHelper::kScanType_OnlyDirectory:
		selector = &SelectOnlyDir;
		break;
	case sframe::FileHelper::kScanType_OnlyNotDirectory:
		selector = &SelectOnlyNotDir;
		break;
	default:
		break;
	}

	struct dirent **namelist;
	int n = scandir(dir_path.c_str(), &namelist, selector, alphasort);
	if (n < 0)
	{
		return vec;
	}

	while (n--)
	{
		std::string name(namelist[n]->d_name);
		if (match_name.empty() || MatchWildcardStr(name, match_name, true))
		{
			vec.push_back(std::move(name));
		}
		free(namelist[n]);
	}
	free(namelist);

#endif

	return vec;
}

// 展开通配符（* ?）
// path: 路径，最后是否以/结尾，表示文件，否则为目录
// parent_dir: 所在目录
std::vector<std::string> FileHelper::ExpandWildcard(const std::string & path, const std::string & parent_dir)
{
	std::string p_dir;
	if (!parent_dir.empty())
	{
		p_dir.reserve(parent_dir.size() + 2);
		p_dir.append(parent_dir);
		char last_char = *(p_dir.end() - 1);
		if (last_char != '/' && last_char != '\\')
		{
			p_dir.push_back('/');
		}
	}

	std::vector<std::string> vec_real_file_name;
	if (path.empty())
	{
		if (!p_dir.empty())
		{
			vec_real_file_name.push_back(p_dir);
		}
		return vec_real_file_name;
	}

	int32_t find_flag = 1;
	size_t cur_name_start_pos = 0;
	size_t wildcard_pos = std::string::npos;
	size_t cur_name_end_pos = std::string::npos;
	
	for (size_t i = 0; i < path.size(); i++)
	{
		char c = path[i];
		if (c == '*' || c == '?')
		{
			wildcard_pos = i;
			find_flag = 2;
		}
		else if (c == '/' || c == '\\')
		{
			if (find_flag == 1)
			{
				cur_name_start_pos = i + 1;
			}
			else
			{
				cur_name_end_pos = i;
			}
		}
	}

	if (wildcard_pos == std::string::npos)
	{
		vec_real_file_name.push_back(p_dir + path);
		return vec_real_file_name;
	}

	std::string scan_dir = p_dir + path.substr(0, cur_name_start_pos);
	std::string cur_name = path.substr(cur_name_start_pos, cur_name_end_pos - cur_name_start_pos);
	ScanType scan_type = cur_name_end_pos == std::string::npos ? FileHelper::kScanType_OnlyNotDirectory : FileHelper::kScanType_OnlyDirectory;
	std::string surplus;
	if (cur_name_end_pos != std::string::npos)
	{
		surplus = path.substr(cur_name_end_pos + 1);
	}

	std::vector<std::string> children = ScanDirectory(scan_dir, cur_name, scan_type);
	for (std::string & child : children)
	{
		if (child == "." || child == "..")
		{
			continue;
		}

		if (surplus.empty())
		{
			std::string name;
			name.reserve(scan_dir.size() + child.size() + 2);
			name.append(scan_dir);
			name.append(child);
			if (cur_name_end_pos != std::string::npos)
			{
				assert(cur_name_end_pos == path.size() - 1);
				name.push_back(path[cur_name_end_pos]);
			}
			vec_real_file_name.push_back(std::move(name));
		}
		else
		{
			std::string new_parent_dir;
			new_parent_dir.reserve(scan_dir.size() + child.size() + 2);
			new_parent_dir.append(scan_dir);
			new_parent_dir.append(child);
			new_parent_dir.push_back(path[cur_name_end_pos]);
			std::vector<std::string> ret = ExpandWildcard(surplus, new_parent_dir);

			for (std::string & s : ret)
			{
				vec_real_file_name.push_back(std::move(s));
			}
		}
		
	}

	return vec_real_file_name;
}