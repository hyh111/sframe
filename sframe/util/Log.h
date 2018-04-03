
#ifndef SFRAME_LOG_H
#define SFRAME_LOG_H

#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>
#include <string>
#include <ostream>
#include <unordered_map>
#include <thread>
#include <iomanip>
#include "Lock.h"
#include "Singleton.h"
#include "FileHelper.h"
#include "BlockingQueue.h"

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

	struct LogDataChunk
	{
		static const int32_t kDefaultChunkSize = 256 * 16;

		LogDataChunk() : cur_size(0), time(0) {}

		char data[kDefaultChunkSize];
		int32_t cur_size;
		int64_t time;
	};

	Logger(const std::string & log_name = "");

    ~Logger();

    void Write(int64_t cur_time, const char * text, int32_t len);

	void Flush();

	Lock & GetWriteLock()
	{
		return _lock;
	}

private:

	void FlushLogDataChunk(LogDataChunk * data_chunk, int64_t now);

	bool IsWriteBufferHaveData();

	std::string GetLogFileName(int64_t cur_time);

private:
    Lock _lock;
    FILE * _file;
	int64_t _open_file_time;
	int64_t _log_file_time;
    std::string _log_name;
	std::vector<LogDataChunk*> * _write_log_buffer;
	std::vector<LogDataChunk*> * _flush_log_buffer;
	bool _waiting_flush;
};

// 日志管理器
class LoggerMgr : public singleton<LoggerMgr>
{
public:

	LoggerMgr() : _flush_log_thread(nullptr), _is_running(false) {}

	~LoggerMgr();

	void Initialize(const std::string & log_dir = "", const std::string & log_base_name = "");

	Logger & GetLogger(const std::string & log_name);

	const std::string & GetLogDir() const
	{
		return _log_dir;
	}

	const std::string & GetLogBaseName() const
	{
		return _log_base_name;
	}

	bool IsRunning()
	{
		return _is_running;
	}

	void PushNeedFlushLogger(Logger * logger)
	{
		if (logger)
		{
			_wait_flush_logger_queue.Push(logger);
		}
	}

private:

	static void ExecFlushLog(LoggerMgr * log_mgr);

private:
	Lock _lock;
	std::thread * _flush_log_thread;
	bool _is_running;
	Logger _default_logger;
	std::unordered_map<std::string, Logger *> _loggers;
	std::string _log_dir;
	std::string _log_base_name;
	BlockingQueue<Logger *> _wait_flush_logger_queue;
};


class LogStreamBuf : public std::streambuf
{
	static const int32_t kMaxBufferLen = 255;
public:
	LogStreamBuf(Logger & logger, int64_t cur_time);

	~LogStreamBuf() {}

	int64_t GetCurTime() const
	{
		return _cur_time;
	}

protected:

	std::streambuf::int_type overflow(std::streambuf::int_type c) override;

	int sync() override;

private:
	int64_t _cur_time;
	Logger & _logger;
	char _buf[kMaxBufferLen + 1];
	std::vector<std::vector<char> > _vec_ext_buf;
};

// 日志流
class LogStream : public std::ostream
{
    static const char kLogLevelText[kLogLevelCount][8];

public:
    LogStream(const std::string & log_name, LogLevel lv = kLogLevel_None);

	~LogStream();

private:
	LogStreamBuf _log_buf;
    LogLevel _log_lv;
};

#define ENDL std::endl;

// 初始化日志模块
#define INITIALIZE_LOG(log_dir, log_base_name) sframe::LoggerMgr::Instance().Initialize((log_dir), (log_base_name))

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