
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
	kHttpType_Request = 1,      // HttpÇëÇó
	kHttpType_Response = 2,     // HttpÏìÓ¦
};

class Http
{
public:

	typedef std::unordered_map<std::string, std::string> Param;

	typedef std::unordered_map<std::string, std::vector<std::string>> Header;

	// ±ê×¼»¯Í·²¿ÊôÐÔKey
	static std::string StandardizeHeaderKey(const std::string & key);

	// URL±àÂë
	static std::string UrlEncode(const std::string & str);

	// URL½âÂë
	static std::string UrlDecode(const std::string & str);

	// ½âÎöHTTP²ÎÊý
	static Http::Param ParseHttpParam(const std::string para_str);

	// HttpParam×ª»»Îªstring
	static std::string HttpParamToString(const Http::Param & para);



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
public:

	enum DecodeState
	{
		kDecodeState_FirstLine = 0,       // ÕýÔÚµÚÒ»ÐÐ
		kDecodeState_HttpHeader = 1,      // ÕýÔÚ½âÎöÍ·²¿ÊôÐÔ
		kDecodeState_Content = 2,         // ÕýÔÚ½âÎöÄÚÈÝ²¿·Ö
		kDecodeState_Completed = 3,       // ½âÎöÍê³É
	};

	void Reset();

	bool IsDecodeCompleted() const
	{
		return _state == kDecodeState_Completed;
	}

	// ½âÎö
	// ·µ»Ø½âÎöÁËµÄÓÐÐ§Êý¾ÝÊý¾ÝµÄ³¤¶È
	size_t Decode(const std::string & data, std::string & err_msg)
	{
		return Decode(data.data(), data.length(), err_msg);
	}

	// ½âÎö
	// ·µ»Ø½âÎöÁËµÄÓÐÐ§Êý¾ÝÊý¾ÝµÄ³¤¶È
	size_t Decode(const char * data, size_t len, std::string & err_msg);

protected:

	HttpDecoder(int32_t http_type);

	virtual ~HttpDecoder() {}

	std::shared_ptr<HttpRequest> GetHttpRequest() const
	{
		return _http_request;
	}

	std::shared_ptr<HttpResponse> GetHttpResponse() const
	{
		return _http_response;
	}

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
	int32_t _remain_content_len;      // -1.chunked²»È·¶¨³¤¶È; -2.ÄÚÈÝÖªµÀÁ¬½Ó¹Ø±Õ²Å¶ÁÍê; >0.¶¨³¤
	std::vector<std::string> _data_list;
};

class HttpRequestDecoder : public HttpDecoder
{
public:

	HttpRequestDecoder() : HttpDecoder(kHttpType_Request) {}

	~HttpRequestDecoder() {}

	std::shared_ptr<HttpRequest> GetResult() const
	{
		return GetHttpRequest();
	}
};

class HttpResponseDecoder : public HttpDecoder
{
public:

	HttpResponseDecoder() : HttpDecoder(kHttpType_Response) {}

	~HttpResponseDecoder() {}

	std::shared_ptr<HttpResponse> GetResult() const
	{
		return GetHttpResponse();
	}
};

}

#endif
