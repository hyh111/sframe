
#include <string.h>
#include "Log.h"
#include "TimeHelper.h"

using namespace sframe;


#ifndef __GNUC__

static const WORD kConsoleTextColor[kLogLevelCount] =
{
    FOREGROUND_GREEN,
    FOREGROUND_BLUE | FOREGROUND_GREEN,
    FOREGROUND_GREEN | FOREGROUND_RED,
    FOREGROUND_RED
};

#else

static const char kConsoleTextColor[kLogLevelCount][32] =
{
    "\033[34m\033[1m%s\033[0m",
	"\033[32m\033[1m%s\033[0m",
    "\033[33m%s\033[0m",
    "\033[31m%s\033[0m",
};

#endif

Logger::~Logger()
{
    if (_file)
    {
        fclose(_file);
    }
}

// 写日志
void Logger::Write(const char * text, LogLevel log_lv)
{
    AutoLock<Lock> l(_lock);

	if (LoggerMgr::Instance().IsLogInConsole() && log_lv >= kLogLevel_Begin && log_lv < kLogLevel_End)
	{
		WriteInConsole(text, log_lv);
	}
    
    if (!LoggerMgr::Instance().GetLogDir().empty())
    {
        WriteInFile(text);
    }
}

// 准备写入
bool Logger::RepareWrite()
{
	int64_t cur_time = TimeHelper::GetEpochSeconds();

	if (!TimeHelper::IsInSameDay(_file_time, cur_time) && _file)
	{
		fclose(_file);
		_file = nullptr;
	}

    if (!_file)
    {
        std::string full_file_name = LoggerMgr::Instance().GetLogDir() + GetLogFileName(cur_time);
        _file = fopen(full_file_name.c_str(), "a");
        if (_file == nullptr)
        {
            return false;
        }
    }

	return true;
}

// 写入文件
void Logger::WriteInFile(const char * text)
{
    int32_t text_len = (int32_t)strlen(text);

    if (!RepareWrite())
    {
        return;
    }

    if (fwrite(text, text_len, 1, _file) > 0)
    {
        _len += text_len;
        fflush(_file);
    }
}

// 写入控制台
void Logger::WriteInConsole(const char * text, LogLevel lv)
{
    int color_index = lv - kLogLevel_Begin;

#ifndef WIN32

	lv == LogLevel::kLogLevel_Error ? fprintf(stderr, kConsoleTextColor[color_index], text) :
		fprintf(stdout, kConsoleTextColor[color_index], text);

#else
    HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStd == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO oldInfo;
    if (!GetConsoleScreenBufferInfo(hStd, &oldInfo))
    {
        return;
    }
    else
    {
        SetConsoleTextAttribute(hStd, kConsoleTextColor[lv - kLogLevel_Begin]);
        lv == LogLevel::kLogLevel_Error ? fprintf(stderr, "%s", text) : fprintf(stdout, "%s", text);
        SetConsoleTextAttribute(hStd, oldInfo.wAttributes);
    }
#endif
}

// 获取新的日志文件名
const std::string Logger::GetLogFileName(int64_t cur_time)
{
	std::string file_name;
	file_name.reserve(256);
	file_name.append(LoggerMgr::Instance().GetLogBaseName());
	if (!_log_name.empty())
	{
		if (!file_name.empty())
		{
			file_name.push_back('_');
		}

		file_name.append(_log_name);
	}

	char tail[256];
	tm cur_tm;
	TimeHelper::TimeToTM(cur_time, &cur_tm);
	sprintf(tail, "%04d%02d%02d.log", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday);

	if (!file_name.empty())
	{
		file_name.push_back('_');
	}
	file_name.append(tail);

	_file_time = cur_time;

	return std::move(file_name);
}




Logger & LoggerMgr::GetLogger(const std::string & log_name)
{
	AutoLock<SpinLock> l(_lock);

	Logger * logger = nullptr;

	if (log_name.empty())
	{
		logger = &_default_logger;
	}
	else
	{
		auto it = _loggers.find(log_name);
		if (it == _loggers.end())
		{
			std::shared_ptr<Logger> l = std::make_shared<Logger>(log_name);
			assert(l);
			auto insert_ret = _loggers.insert(std::make_pair(log_name, l));
			assert(insert_ret.second);
			it = insert_ret.first;
		}

		logger = it->second.get();
	}

	assert(logger);

	return *logger;
}

void LoggerMgr::SetLogDir(const std::string & dir)
{
	assert(!dir.empty());

	_log_dir = dir;

	for (auto it = _log_dir.begin(); it < _log_dir.end(); it++)
	{
		if (*it == '\\')
		{
			*it = '/';
		}
	}

	char c = _log_dir.at(_log_dir.length() - 1);
	if (c != '/')
	{
		_log_dir.append("/");
	}

	bool ret = FileHelper::MakeDirectoryRecursive(_log_dir);
	assert(ret);
}


LogBuf::LogBuf(Logger & logger, LogLevel lv)
	: _logger(logger), _lv(lv)
{
	setp(_buf, _buf + kMaxBufferLen);
}

int LogBuf::sync()
{
	int32_t len = (int32_t)(pptr() - pbase());
	if (len == 0)
	{
		return 0;
	}

	assert(len > 0);

	*pptr() = '\0';
	_logger.Write(_buf, _lv);
	pbump(-len);

	return 0;
}


// 日志等级的文本
const char LogStream::kLogLevelText[kLogLevelCount][8] =
{
    "TRACE", "INFO", "WARN", "ERROR"
};

LogStream::LogStream(const std::string & log_name, LogLevel log_lv)
    : _log_buf(LoggerMgr::Instance().GetLogger(log_name), log_lv), std::ostream(&_log_buf), _log_lv(log_lv)
{
    tm cur_tm;
    TimeHelper::TimeToTM(TimeHelper::GetEpochSeconds(), &cur_tm);
	(*this) << (cur_tm.tm_year + 1900) << '-' << (cur_tm.tm_mon + 1) << '-' << cur_tm.tm_mday 
		<< ' ' << cur_tm.tm_hour << ':' << cur_tm.tm_min << ':' << cur_tm.tm_sec << '|';
	
	if (log_lv >= kLogLevel_Begin && log_lv < kLogLevel_End)
	{
		(*this) << kLogLevelText[log_lv - kLogLevel_Begin] << '|';
	}
}

LogStream::~LogStream()
{
	this->flush();
}