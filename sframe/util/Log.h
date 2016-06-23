
#ifndef SFRAME_LOG_H
#define SFRAME_LOG_H

#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>
#include <string>
#include <ostream>
#include <unordered_map>
#include <memory>
#include "Lock.h"
#include "Singleton.h"
#include "FileHelper.h"

namespace sframe{

// 日志级别
enum LogLevel : int32_t
{
	kLogLevel_None = 0,

    kLogLevel_Begin = 1,

    kLogLevel_Trace = kLogLevel_Begin,   // 跟踪
    kLogLevel_Info,                    // 消息
    kLogLevel_Warn,                    // 警告
    kLogLevel_Error,                   // 错误

    kLogLevel_End,
};

static const int32_t kLogLevelCount = kLogLevel_End - kLogLevel_Begin;

// 日志类
class Logger : public noncopyable
{
public:
    Logger(const std::string & log_name = "") 
		: _file(nullptr), _len(0), _log_name(log_name), _file_time(0) {}

    ~Logger();

    // 写日志
    void Write(const char * text, LogLevel log_lv);

private:
    // 准备写入
    bool RepareWrite();
    // 写入文件
    void WriteInFile(const char * text);
    // 写入控制台
    void WriteInConsole(const char * text, LogLevel log_lv);
	// 获取新的日志文件名
	const std::string GetLogFileName(int64_t cur_time);

private:
    Lock _lock;
    FILE * _file;
    int32_t _len;
    std::string _log_name;
	int64_t _file_time;   // 当前日志的时间
};

// 日志管理器
class LoggerMgr : public singleton<LoggerMgr>
{
public:

	LoggerMgr() : _log_in_console(false) {}

	Logger & GetLogger(const std::string & log_name);

	void SetLogDir(const std::string & dir);

	void SetLogBaseName(const std::string & name)
	{
		_log_base_name = name;
	}

	void SetLogInConsole(bool log_in_console)
	{
		_log_in_console = log_in_console;
	}

	const std::string & GetLogDir() const
	{
		return _log_dir;
	}

	const std::string & GetLogBaseName() const
	{
		return _log_base_name;
	}

	bool IsLogInConsole() const
	{
		return _log_in_console;
	}

private:
	SpinLock _lock;
	Logger _default_logger;
	std::unordered_map<std::string, std::shared_ptr<Logger>> _loggers;
	std::string _log_dir;
	std::string _log_base_name;
	bool _log_in_console;
};

// 日志缓冲区
class LogBuf : public std::streambuf
{
	static const int32_t kMaxBufferLen = 1024;
public:
	LogBuf(Logger & logger, LogLevel lv);

	~LogBuf() {}

	int sync() override;

private:
	Logger & _logger;
	LogLevel _lv;
	char _buf[kMaxBufferLen + 2];
};

// 日志流
class LogStream : public std::ostream
{
    static const char kLogLevelText[kLogLevelCount][8];

public:
    LogStream(const std::string & log_name, LogLevel lv = kLogLevel_None);

	~LogStream();

private:
	LogBuf _log_buf;
    LogLevel _log_lv;
};

#define ENDL std::endl;

// 设置相关
#define SET_LOG_BASENAME(name)    sframe::LoggerMgr::Instance().SetLogBaseName((name))
#define SET_LOG_DIR(dir)          sframe::LoggerMgr::Instance().SetLogDir((dir))

// 按级别日志
#define LOG_LEVEL(lv) sframe::LogStream("", lv) \
	<< sframe::FileHelper::GetFileName(__FILE__) << ':' << __LINE__ << '|'

#define LOG_TRACE    LOG_LEVEL(sframe::LogLevel::kLogLevel_Trace)
#define LOG_INFO     LOG_LEVEL(sframe::LogLevel::kLogLevel_Info)
#define LOG_WARN     LOG_LEVEL(sframe::LogLevel::kLogLevel_Warn)
#define LOG_ERROR    LOG_LEVEL(sframe::LogLevel::kLogLevel_Error)

// 分模块日志
#define FLOG(log_name) sframe::LogStream(log_name) << sframe::FileHelper::GetFileName(__FILE__) << ':' << __LINE__ << '|'

}

#endif