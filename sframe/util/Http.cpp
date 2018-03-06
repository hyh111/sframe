
#include <algorithm>
#include "Http.h"
#include "Convert.h"
#include "StringHelper.h"

using namespace sframe;

// 标准化头部Key
std::string Http::StandardizeHeaderKey(const std::string & key)
{
	static const char kUpperLower = 'a' - 'A';
	std::string standardized_key;
	standardized_key.reserve(key.size());

	bool upper = true;
	for (std::string::const_iterator it = key.begin(); it != key.end(); it++)
	{
		char c = *it;
		if (c >= 'a' && c <= 'z')
		{
			c = upper ? c - kUpperLower : c;
		}
		else if (c >= 'A' && c <= 'Z')
		{
			c = !upper ? c + kUpperLower : c;
		}

		standardized_key.push_back(c);
		upper = (c == '-');
	}

	return standardized_key;
}

// URL编码
std::string Http::UrlEncode(const std::string & str)
{
	static char sHexTable[17] = "0123456789ABCDEF";

	std::ostringstream oss;

	for (size_t i = 0; i < str.length(); i++)
	{
		char cur_char = str[i];
		if (cur_char == '-' || cur_char == '_' || cur_char == '.' ||
			(cur_char >= '0' && cur_char <= '9') ||
			(cur_char >= 'a' && cur_char <= 'z') ||
			(cur_char >= 'A' && cur_char <= 'Z'))
		{
			oss << cur_char;
		}
		else
		{
			oss << '%' << (sHexTable[(cur_char >> 4) & 0x0f]) << (sHexTable[(cur_char & 0x0f)]);
		}
	}

	return oss.str();
}

// URL解码
std::string Http::UrlDecode(const std::string & str)
{
	std::ostringstream oss;
	size_t pos = 0;

	while (pos < str.length())
	{
		char cur_char = str[pos];
		if (cur_char == '%')
		{
			if (pos + 2 >= str.length())
			{
				break;
			}

			char c1 = str[pos + 1];
			c1 = (c1 >= 'a' && c1 <= 'z') ? c1 - 32 : c1;
			char c2 = str[pos + 2];
			c2 = (c2 >= 'a' && c2 <= 'z') ? c2 - 32 : c2;

			cur_char = (((c1 >= 'A' ? (c1 - 'A' + 10) : (c1 - '0')) << 4) & 0xf0);
			cur_char += ((c2 >= 'A' ? (c2 - 'A' + 10) : (c2 - '0')) & 0x0f);
			oss << cur_char;

			pos += 3;
		}
		else
		{
			oss << cur_char;
			pos++;
		}
	}

	return oss.str();
}

// 解析HTTP参数
Http::Param Http::ParseHttpParam(const std::string para_str)
{
	Http::Param para;

	size_t cur_word_start_pos = para_str.find_first_not_of(' ');
	if (cur_word_start_pos == std::string::npos)
	{
		return para;
	}

	std::string k;
	size_t i = 0;

	for (; i < para_str.length(); i++)
	{
		char c = para_str[i];

		switch (c)
		{
		case '=':

			if (i > cur_word_start_pos)
			{
				k = UrlDecode(para_str.substr(cur_word_start_pos, i - cur_word_start_pos));
			}
			cur_word_start_pos = i + 1;

			break;

		case '&':

			assert(i >= cur_word_start_pos);
			if (!k.empty())
			{
				para[std::move(k)] = UrlDecode(para_str.substr(cur_word_start_pos, i - cur_word_start_pos));
			}
			else if (i > cur_word_start_pos)
			{
				para[UrlDecode(para_str.substr(cur_word_start_pos, i - cur_word_start_pos))] = std::string();
			}
			cur_word_start_pos = i + 1;

			break;
		}
	}

	assert(i == para_str.length() && i >= cur_word_start_pos);
	if (!k.empty())
	{
		para[std::move(k)] = UrlDecode(para_str.substr(cur_word_start_pos, i - cur_word_start_pos));
	}
	else if (i > cur_word_start_pos)
	{
		para[UrlDecode(para_str.substr(cur_word_start_pos, i - cur_word_start_pos))] = std::string();
	}

	return para;
}

// HttpParam转换为string
std::string Http::HttpParamToString(const Http::Param & para)
{
	std::ostringstream oss;

	for (Http::Param::const_iterator it = para.begin(); it != para.end(); it++)
	{
		if (it->first.empty())
		{
			continue;
		}

		if (it != para.begin())
		{
			oss << '&';
		}
		oss << UrlEncode(it->first) << '=' << UrlEncode(it->second);
	}

	return oss.str();
}




const std::string & Http::GetHeader(const std::string & key) const
{
	auto it = _header.find(key);
	if (it == _header.end())
	{
		static std::string empty;
		return empty;
	}

	assert(!it->second.empty());
	return it->second[0];
}

const std::vector<std::string> & Http::GetHeaders(const std::string & key) const
{
	auto it = _header.find(key);
	if (it == _header.end())
	{
		static std::vector<std::string> empty;
		return empty;
	}

	assert(!it->second.empty());
	return it->second;
}

void Http::SetHeader(const std::string & key, const std::string &value)
{
	std::string standardized_key = StandardizeHeaderKey(Trim(key));
	if (!standardized_key.empty())
	{
		_header[standardized_key].push_back(Trim(value));
	}
}

const std::string & Http::GetContent() const
{
	return _content;
}

void Http::SetContent(const std::string & data)
{
	_content = data;
}

void Http::SetContent(std::string && data)
{
	_content = std::move(data);
}

std::string Http::ToString() const
{
	std::ostringstream oss;
	bool have_conetnt_len = false;

	WriteFirstLine(oss);
	for (auto & pr : _header)
	{
		if (pr.first.empty())
		{
			assert(false);
			continue;
		}

		if (pr.first == "Content-Length" && !pr.second.empty())
		{
			have_conetnt_len = true;
		}

		for (const std::string & v : pr.second)
		{
			oss << pr.first << ": " << v << "\r\n";
		}
	}

	if (!have_conetnt_len)
	{
		oss << "Content-Length: " << _content.length() << "\r\n";
	}

	oss << "\r\n";

	if (!_content.empty())
	{
		oss << _content;
	}

	return oss.str();
}



HttpRequest::HttpRequest() : _method("GET"), _req_url("/"), _proto_ver("HTTP/1.1") {}

void HttpRequest::SetMethod(const std::string & method)
{
	_method = ToUpper(method);
}

const std::string & HttpRequest::GetMethod() const
{
	return _method;
}

void HttpRequest::SetRequestUrl(const std::string & req_url)
{
	if (req_url.empty() || (*req_url.begin()) != '/')
	{
		_req_url.clear();
		_req_url.reserve(req_url.length() + 1);
		_req_url.push_back('/');
		_req_url.append(req_url);
	}
	else
	{
		_req_url = req_url;
	}
}

const std::string & HttpRequest::GetRequestUrl() const
{
	return _req_url;
}

void HttpRequest::SetRequestParam(const std::string & req_param)
{
	_req_param = req_param;
}

const std::string & HttpRequest::GetRequestParam() const
{
	return _req_param;
}

void HttpRequest::SetProtoVersion(const std::string & proto_version)
{
	if (!proto_version.empty())
	{
		_proto_ver = proto_version;
	}
}

const std::string & HttpRequest::GetProtoVersion() const
{
	return _proto_ver;
}

void HttpRequest::WriteFirstLine(std::ostringstream & oss) const
{
	assert(!_method.empty() && !_req_url.empty() && !_proto_ver.empty());
	oss << _method << ' ' << _req_url;
	if (!_req_param.empty())
	{
		oss << '?' << _req_param;
	}
	oss << ' ' << _proto_ver << "\r\n";
}


HttpResponse::HttpResponse() : _proto_ver("HTTP/1.1"), _status_code(200), _status_desc("OK") {}

void HttpResponse::SetProtoVersion(const std::string & proto_version)
{
	_proto_ver = proto_version;
}

const std::string & HttpResponse::GetProtoVersion() const
{
	return _proto_ver;
}

void HttpResponse::SetStatusCode(int32_t status_code)
{
	_status_code = status_code;
}

int32_t HttpResponse::GetStatusCode() const
{
	return _status_code;
}

void HttpResponse::SetStatusDesc(const std::string & status_desc)
{
	_status_desc = status_desc;
}

const std::string & HttpResponse::GetStatusDesc() const
{
	return _status_desc;
}

void HttpResponse::WriteFirstLine(std::ostringstream & oss) const
{
	assert(!_proto_ver.empty() && !_status_desc.empty());
	if (!_proto_ver.empty())
	{
		oss << _proto_ver << ' ';
	}
	oss << _status_code << ' ' << _status_desc << "\r\n";
}



HttpDecoder::HttpDecoder(int32_t http_type) : _http_type(http_type), _state(kDecodeState_FirstLine), _remain_content_len(0)
{
	if (_http_type == kHttpType_Request)
	{
		_http_request = std::make_shared<HttpRequest>();
	}
	else
	{
		_http_response = std::make_shared<HttpResponse>();
	}

	_data_list.reserve(32);
}

void HttpDecoder::Reset()
{
	_state = kDecodeState_FirstLine;
	if (_http_type == kHttpType_Request)
	{
		_http_request = std::make_shared<HttpRequest>();
	}
	else
	{
		_http_response = std::make_shared<HttpResponse>();
	}
	_remain_content_len = 0;
	_data_list.clear();
}

size_t HttpDecoder::Decode(const char * data, size_t len, std::string & err_msg)
{
	size_t readed = 0;
	bool conn = true;

	while (conn)
	{
		size_t tmp = 0;
		conn = false;

		switch (_state)
		{
		case kDecodeState_FirstLine:
			tmp = DecodeFirstLine(data + readed, len - readed, err_msg);
			readed += tmp;
			assert(readed <= len);
			if (!err_msg.empty())
			{
				return readed;
			}

			if (_state == kDecodeState_HttpHeader)
			{
				conn = len > readed ? true : false;
			}
			else
			{
				assert(_state == kDecodeState_FirstLine);
			}

			break;

		case kDecodeState_HttpHeader:
			tmp = DecodeHttpHeader(data + readed, len - readed, err_msg);
			readed += tmp;
			assert(readed <= len);
			if (!err_msg.empty())
			{
				return readed;
			}

			if (_state == kDecodeState_Content)
			{
				conn = len > readed ? true : false;
			}
			else
			{
				assert(_state == kDecodeState_HttpHeader || _state == kDecodeState_Completed);
			}

			break;

		case kDecodeState_Content:
			tmp = DecodeContent(data + readed, len - readed, err_msg);
			readed += tmp;
			assert(readed <= len);
			if (!err_msg.empty())
			{
				return readed;
			}
			break;
		}
	}

	return readed;
}

struct StrLocation
{
	size_t start_index;
	size_t len;
};

size_t HttpDecoder::DecodeFirstLine(const char * data, size_t len, std::string & err_msg)
{
	if (_state != kDecodeState_FirstLine)
	{
		assert(false);
		return 0;
	}

	StrLocation loc_word1 = { 0 };
	StrLocation loc_word2 = { 0 };
	StrLocation loc_word3 = { 0 };
	int cur_read = 0;   //正在读什么, 0.请求方法, 1.URL, 2. 协议版本, 3. 完成
	size_t readed = 0;

	for (size_t i = 0; i < len; i++)
	{
		if (data[i] == ' ')
		{
			switch (cur_read)
			{
			case 0:
				loc_word1.len = i;
				if (loc_word1.len == 0)
				{
					err_msg = "http first line error";
					return 0;
				}
				loc_word2.start_index = i + 1;
				cur_read = 1;
				break;
			case 1:
				loc_word2.len = i - loc_word2.start_index;
				if (loc_word2.len == 0)
				{
					err_msg = "http first line error";
					return 0;
				}
				loc_word3.start_index = i + 1;
				cur_read = 2;
				break;
			}
		}
		else if (data[i] == '\r' && i + 1 < len && data[i + 1] == '\n')
		{
			if (cur_read != 2)
			{
				err_msg = "http first line error";
				return 0;
			}

			loc_word3.len = i - loc_word3.start_index;
			if (loc_word2.len == 0)
			{
				err_msg = "http first line error";
				return 0;
			}

			cur_read = 3;
			readed = i + 2;
			break;
		}
	}

	assert(cur_read >= 0 && cur_read <= 3);
	if (cur_read < 3)
	{
		return 0;
	}

	assert(readed > 0);

	if (_http_type == kHttpType_Request)
	{
		assert(_http_request);

		_http_request->SetMethod(std::string(data + loc_word1.start_index, loc_word1.len));

		std::string url_str(data + loc_word2.start_index, loc_word2.len);
		size_t param_pos = url_str.find('?');
		if (param_pos == std::string::npos)
		{
			_http_request->SetRequestUrl(url_str);
		}
		else
		{
			_http_request->SetRequestUrl(url_str.substr(0, param_pos));
			_http_request->SetRequestParam(url_str.substr(param_pos + 1));
		}

		_http_request->SetProtoVersion(std::string(data + loc_word3.start_index, loc_word3.len));
	}
	else
	{
		assert(_http_response);
		_http_response->SetProtoVersion(std::string(data + loc_word1.start_index, loc_word1.len));
		_http_response->SetStatusCode(StrToAny<int32_t>(std::string(data + loc_word2.start_index, loc_word2.len)));
		_http_response->SetStatusDesc(std::string(data + loc_word3.start_index, loc_word3.len));
	}

	// 转换状态为解析请求头部
	_state = kDecodeState_HttpHeader;

	return readed;
}

size_t HttpDecoder::DecodeHttpHeader(const char * data, size_t len, std::string & err_msg)
{
	if (_state != kDecodeState_HttpHeader)
	{
		assert(false);
		return 0;
	}

	size_t readed = 0;

	while (readed < len)
	{
		const char * p = data + readed;
		int32_t remain_len = (int32_t)(len - readed);
		int32_t line_len = FindFirstSubstr(p, remain_len, "\r\n");
		if (line_len < 0)
		{
			break;
		}

		readed += (line_len + 2);

		if (line_len > 0)
		{
			int32_t kv_sep_pos = FindFirstSubstr(p, line_len, ":");
			if (kv_sep_pos < 0)
			{
				continue;
			}

			std::string k = std::string(p, kv_sep_pos);
			std::string v = std::string(p + kv_sep_pos + 1, line_len - (kv_sep_pos + 1));
			if (!k.empty())
			{
				if (_http_type == kHttpType_Request)
				{
					assert(_http_request);
					_http_request->SetHeader(k, v);
				}
				else
				{
					assert(_http_response);
					_http_response->SetHeader(k, v);
				}
			}
		}
		else
		{
			// 空行, 解析头部结束，确定是否有数据部分
			const std::string trans_encoding = ToLower(GetHeader("Transfer-Encoding"));
			if (trans_encoding == "chunked")
			{
				_state = kDecodeState_Content;
				_remain_content_len = -1;
			}
			else
			{
				const std::string & content_len_str = GetHeader("Content-Length");
				if (content_len_str.empty())
				{
					if (_http_type == kHttpType_Request)
					{
						_state = kDecodeState_Completed;
					}
					else
					{
						_state = kDecodeState_Content;
						_remain_content_len = -2;
					}
				}
				else
				{
					_remain_content_len = std::max(0, StrToAny<int32_t>(content_len_str));
					if (_remain_content_len > 0)
					{
						_state = kDecodeState_Content;
					}
					else
					{
						_state = kDecodeState_Completed;
					}
				}
			}

			// 退出循环
			break;
		}
	}

	return readed;
}


size_t HttpDecoder::DecodeContent(const char * data, size_t len, std::string & err_msg)
{
	if (_state != kDecodeState_Content)
	{
		assert(false);
		return 0;
	}

	size_t readed = 0;

	if (_remain_content_len > 0)
	{
		readed = std::min(len, (size_t)_remain_content_len);
		if (readed > 0)
		{
			_data_list.push_back(std::string(data, readed));
			_remain_content_len -= (int32_t)readed;
		}
	}
	else if (_remain_content_len == -1)
	{
		size_t surplus = len;
		const char * p = data;

		while (surplus > 2)
		{
			// 块的长度
			uint32_t chunk_len = 0;
			char chunk_len_str[9];
			int chunk_len_str_len = 0;

			while (surplus >= 2 && !(p[0] == '\r' && p[1] == '\n'))
			{
				if (chunk_len_str_len < ((int)sizeof(chunk_len_str) - 1))
				{
					chunk_len_str[chunk_len_str_len++] = p[0];
				}
				surplus--;
				p++;
			}

			// 长度是否读取完全
			if (!(p[0] == '\r' && p[1] == '\n'))
			{
				break;
			}

			surplus -= 2;
			p += 2;

			// 转换chunk_len
			if (chunk_len_str_len > 0)
			{
				chunk_len_str[chunk_len_str_len] = 0;
				chunk_len = (uint32_t)strtol(chunk_len_str, nullptr, 16);
			}

			// 数据若不够
			if (surplus < chunk_len + 2)
			{
				break;
			}

			if (chunk_len > 0)
			{
				_data_list.push_back(std::string(p, chunk_len));
			}

			readed += (chunk_len_str_len + 2 + chunk_len + 2);
			surplus -= (chunk_len + 2);
			p += (chunk_len + 2);

			// chunk_len 为0，表是为结尾的chunk
			if (chunk_len == 0)
			{
				_remain_content_len = 0; // 读取完成
				break;
			}
		}
	}
	else if (_remain_content_len == -2)
	{
		if (len > 0)
		{
			readed = len;
			_data_list.push_back(std::string(data, readed));
		}
		else
		{
			_remain_content_len = 0;    // 内容读取结束
		}
	}
	else
	{
		assert(false);
	}

	// 数据部分是否完成
	if (_remain_content_len == 0)
	{
		_state = kDecodeState_Completed;
		size_t all_len = 0;
		for (const std::string & s : _data_list)
		{
			assert(!s.empty());
			all_len += s.size();
		}

		std::string content;
		assert(all_len > 0);
		content.reserve(all_len);
		for (const std::string & s : _data_list)
		{
			content.append(s);
		}

		if (_http_type == kHttpType_Request)
		{
			assert(_http_request);
			_http_request->SetContent(std::move(content));
		}
		else
		{
			assert(_http_response);
			_http_response->SetContent(std::move(content));
		}
	}

	return readed;
}

const std::string & HttpDecoder::GetHeader(const std::string & k)
{
	if (_http_type == kHttpType_Request)
	{
		assert(_http_request);
		return _http_request->GetHeader(k);
	}

	assert(_http_response);
	return _http_response->GetHeader(k);
}