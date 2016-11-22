
#ifndef SFRAME_HTTP_H
#define SFRAME_HTTP_H

#include <assert.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace sframe {

enum HttpType
{
	kHttpType_Request = 1,      // Http请求
	kHttpType_Response = 2,     // Http响应
};

class Http
{
public:

	typedef std::unordered_map<std::string, std::vector<std::string>> Header;

	// 标准化头部属性Key
	static std::string StandardizeHeaderKey(const std::string & key);

	Http() {}

	virtual ~Http() {}

	virtual HttpType GetHttpType() const = 0;

	const std::string & GetHeader(const std::string & key) const;

	const std::vector<std::string> & GetHeaders(const std::string & key) const;

	void SetHeader(const std::string & key, const std::string &value);

	const std::string & GetContent() const;

	void SetContent(const std::string & data);

	void SetContent(std::string && data);

	std::string ToString() const;

private:

	virtual void WriteFirstLine(std::ostringstream & oss) const = 0;

protected:
	Header _header;
	std::string _content;
};

class HttpRequest : public Http
{
public:
	static const int kMaxUrlLen = 4096;

	HttpRequest();

	~HttpRequest() {}

	HttpType GetHttpType() const
	{
		return kHttpType_Request;
	}

	void SetMethod(const std::string & method);

	const std::string & GetMethod() const;

	void SetRequestUrl(const std::string & req_url);

	const std::string & GetRequestUrl() const;

	void SetRequestParam(const std::string & req_param);

	const std::string & GetRequestParam() const;

	void SetProtoVersion(const std::string & proto_version);

	const std::string & GetProtoVersion() const;

private:

	void WriteFirstLine(std::ostringstream & oss) const override;

private:
	std::string _method;
	std::string _req_url;
	std::string _req_param;
	std::string _proto_ver;
};

class HttpResponse : public Http
{
public:
	static const int kMaxUrlLen = 4096;

	HttpResponse();

	~HttpResponse() {}

	HttpType GetHttpType() const
	{
		return kHttpType_Response;
	}

	void SetProtoVersion(const std::string & proto_version);

	const std::string & GetProtoVersion() const;

	void SetStatusCode(int32_t status_code);

	int32_t GetStatusCode() const;

	void SetStatusDesc(const std::string & status_desc);

	const std::string & GetStatusDesc() const;

private:

	void WriteFirstLine(std::ostringstream & oss) const override;

private:
	std::string _proto_ver;
	int32_t _status_code;
	std::string _status_desc;
};


class HttpDecoder
{
	enum DecodeState
	{
		kDecodeState_FirstLine = 0,       // 正在第一行
		kDecodeState_HttpHeader = 1,      // 正在解析头部属性
		kDecodeState_Content = 2,         // 正在解析内容部分
		kDecodeState_Completed = 3,       // 解析完成
	};

public:
	HttpDecoder(int32_t http_type);

	~HttpDecoder() {}

	void Reset();

	bool IsDecodeCompleted() const
	{
		return _state == kDecodeState_Completed;
	}

	std::shared_ptr<HttpRequest> GetHttpRequest() const
	{
		return _http_request;
	}

	std::shared_ptr<HttpResponse> GetHttpResponse() const
	{
		return _http_response;
	}

	// 解析
	// 返回解析了的有效数据数据的长度
	size_t Decode(const std::string & data, std::string & err_msg)
	{
		return Decode(data.data(), data.length(), err_msg);
	}

	// 解析
	// 返回解析了的有效数据数据的长度
	size_t Decode(const char * data, size_t len, std::string & err_msg);

private:
	size_t DecodeFirstLine(const char * data, size_t len, std::string & err_msg);

	size_t DecodeHttpHeader(const char * data, size_t len, std::string & err_msg);

	size_t DecodeContent(const char * data, size_t len, std::string & err_msg);

	const std::string & GetHeader(const std::string & k);

private:
	int32_t _http_type;
	std::shared_ptr<HttpRequest> _http_request;
	std::shared_ptr<HttpResponse> _http_response;
	int32_t _state;
	int32_t _remain_content_len;      // -1.chunked不确定长度; -2.内容知道连接关闭才读完; >0.定长
	std::vector<std::string> _data_list;
};

}

#endif
