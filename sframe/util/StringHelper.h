
#ifndef SFRAME_STRING_HELPER_H
#define SFRAME_STRING_HELPER_H

#include <string>
#include <vector>

namespace sframe {

// ·Ö¸î×Ö·û´®
std::vector<std::string> SplitString(const std::string & str, const std::string & sep);

// ²éÕÒ×Ö·ûÔÚ×Ö·û´®ÖÐ×î´óÁ¬Ðø³öÏÖ´ÎÊý
int32_t GetCharMaxContinueInString(const std::string & str, char c);

// ²éÕÒ×Ó´®
int32_t FindFirstSubstr(const char * str, int32_t len, const char * sub_str);

// ½«×Ö·û´®×ª»»Îª´óÐ´
void UpperString(std::string & str);

// ½«×Ö·û´®×ª»»ÎªÐ¡Ð´
void LowerString(std::string & str);

// ½«×Ö·û´®×ª»»Îª´óÐ´
std::string ToUpper(const std::string & str);

// ½«×Ö·û´®×ª»»ÎªÐ¡Ð´
std::string ToLower(const std::string & str);

// È¥µôÍ·²¿¿Õ´®
std::string TrimLeft(const std::string & str, char c = ' ');

// È¥µôÎ²²¿¿Õ´®
std::string TrimRight(const std::string & str, char c = ' ');

// È¥µôÁ½±ß¿Õ´®
std::string Trim(const std::string & str, char c = ' ');

// wstring -> string
std::string WStrToStr(const std::wstring & src);

// string -> wstring
std::wstring StrToWStr(const std::string & src);

// utf8 -> wchar
size_t UTF8ToWChar(const char * str, size_t len, wchar_t * wc);

// wchar -> utf8
size_t WCharToUTF8(wchar_t wc, char * buf, size_t buf_size);

// ÊÇ·ñºÏ·¨µÄutf8
bool IsValidUTF8(const std::string & str);

// ÊÇ·ñºÏ·¨µÄutf8
bool IsValidUTF8(const char * str, size_t len);

// wstring -> utf8
std::string WStrToUTF8(const std::wstring & src);

// utf8 -> wstring
std::wstring UTF8ToWStr(const std::string & src);

// ÊÇ·ñÆ¥ÅäÍ¨Åä·û£¨?ºÍ*£©
// str     :   ²»´øÍ¨Åä·ûµÄ×Ö·û´®
// match   :   ´øÍ¨Åä·ûµÄ×Ö·û´®
bool MatchWildcardStr(const std::string & real_name, const std::string & wildcard_name, bool ignore_case = false);

// ½âÎöÀàÐÍÃû³Æ£¨×ª»»Îª A::B::C µÄÐÎÊ½£©
std::string ReadTypeName(const char * name);

bool ParseCommandLine(const std::string & data, std::string & cmd_name, std::vector<std::string> & cmd_param);

}

#endif
