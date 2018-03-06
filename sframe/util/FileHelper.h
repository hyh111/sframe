
#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#include <vector>
#include <string>

namespace sframe {

class FileHelper
{
public:

	// ¶ÁÈ¡ÎÄ¼þËùÓÐÄÚÈÝ
	static bool ReadFile(const std::string & full_name, std::string & content);

	// Ð´ÈëÎÄ¼þ
	static size_t WriteFile(const std::string & full_name, std::string & content);

	// ÔÚÈ«Â·¾¢ÖÐ»ñÈ¡ÎÄ¼þÃû
	static std::string GetFileName(const char * fullname);

	// ÔÚÈ«Â·¾¢ÖÐ»ñÈ¡ÎÄ¼þÃû
	static std::string GetFileName(const std::string & fullname)
	{
		return GetFileName(fullname.c_str());
	}

	// È¥³ýÎÄ¼þÀ©Õ¹Ãû
	static std::string RemoveExtension(const std::string & name);

	// Ä¿Â¼ÊÇ·ñ´æÔÚ
	static bool DirectoryExisted(const std::string & path);

	// ´´½¨Ä¿Â¼
	static bool MakeDirectory(const std::string & path);

	// µÝ¹é´´½¨
	static bool MakeDirectoryRecursive(const std::string & path);

	enum ScanType
	{
		kScanType_All,
		kScanType_OnlyDirectory,
		kScanType_OnlyNotDirectory,
	};

	// É¨ÃèÄ¿Â¼
	// dir_path   :   Ä¿Â¼Â·¾¶£¬²»ÄÜ°üº¬Í¨Åä·û
	// match_name :   Ãû×ÖÆ¥Åä£¬Ö§³ÖÍ¨Åä·û£¬±ÈÈçÒªÉ¨Ãè /data Ä¿Â¼ÏÂ£¬ËùÓÐ·ûºÏ*.cppµÄÃû×ÖµÄÄÚÈÝ£¬µ÷ÓÃScanDirectory("/data", "*.cpp")
	static std::vector<std::string> ScanDirectory(const std::string & dir_path, const std::string & match_name = "", ScanType scan_type = kScanType_All);

	// Õ¹¿ªÍ¨Åä·û(* ?)
	// path: Â·¾¶£¬×îºóÊÇ·ñÒÔ/½áÎ²£¬±íÊ¾ÎÄ¼þ£¬·ñÔòÎªÄ¿Â¼
	// parent_dir: ËùÔÚÄ¿Â¼
	static std::vector<std::string> ExpandWildcard(const std::string & path, const std::string & parent_dir = "");
};

}

#endif
