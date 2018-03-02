
#include <string.h>
#include <algorithm>
#include <iostream>
#include "Log.h"
#include "TimeHelper.h"

using namespace sframe;

Logger::Logger(const std::string & log_name) 
	: _file(nullptr), _open_file_time(0), _log_file_time(0), _log_name(log_name), _waiting_flush(false)
{
	_write_log_buffer = new std::vector<LogDataChunk*>();
	_write_log_buffer->reserve(4);
	_flush_log_buffer = new std::vector<LogDataChunk*>();
	_flush_log_buffer->reserve(4);
}

Logger::~Logger()
{
    if (_file)
    {
        fclose(_file);
    }

	for (LogDataChunk * chunk : (*_write_log_buffer))
	{
		delete chunk;
	}

	for (LogDataChunk * chunk : (*_flush_log_buffer))
	{
		delete chunk;
	}
}

// 写日志
void Logger::Write(int64_t cur_time, const char * text, int32_t len)
{
	if (len <= 0 || !text || !LoggerMgr::Instance().IsRunning())
	{
		return;
	}

	const char * p = text;
	int32_t surplus_len = len;
	
	while (surplus_len > 0)
	{
		LogDataChunk * cur_write_chunk = nullptr;

		if (_write_log_buffer->empty())
		{
			cur_write_chunk = new LogDataChunk();
			_write_log_buffer->push_back(cur_write_chunk);
			cur_write_chunk->time = cur_time;
		}
		else
		{
			cur_write_chunk = *(_write_log_buffer->end() - 1);
			if (cur_write_chunk->cur_size > 0)
			{
				assert(cur_write_chunk->time > 0);
				if (cur_write_chunk->cur_size >= LogDataChunk::kDefaultChunkSize ||
					!TimeHelper::IsInSameDay(cur_time, cur_write_chunk->time))
				{
					cur_write_chunk = new LogDataChunk();
					_write_log_buffer->push_back(cur_write_chunk);
					cur_write_chunk->time = cur_time;
				}
			}
			else
			{
				assert(cur_write_chunk->time == 0);
				cur_write_chunk->time = cur_time;
			}
		}

		int32_t write_len = std::min(surplus_len, LogDataChunk::kDefaultChunkSize - cur_write_chunk->cur_size);
		assert(write_len > 0);
		memcpy(cur_write_chunk->data + cur_write_chunk->cur_size, p, write_len);
		cur_write_chunk->cur_size += write_len;
		p += write_len;
		surplus_len -= write_len;
	}

	if (!_waiting_flush)
	{
		LoggerMgr::Instance().PushNeedFlushLogger(this);
		_waiting_flush = true;
	}
}

void Logger::Flush()
{
	static int flush_count = 0;
	flush_count++;

	{
		AUTO_LOCK(_lock);
		assert(_waiting_flush);
		// 交换写缓冲区和flush缓冲区
		auto temp_buffer = _write_log_buffer;
		_write_log_buffer = _flush_log_buffer;
		_flush_log_buffer = temp_buffer;
	}

	assert(!_flush_log_buffer->empty());
	int64_t now = TimeHelper::GetEpochSeconds();

	for (LogDataChunk * data_chunk : (*_flush_log_buffer))
	{
		FlushLogDataChunk(data_chunk, now);
		data_chunk->cur_size = 0;
		data_chunk->time = 0;
	}

	// 只保留一个数据块
	while (_flush_log_buffer->size() > 1)
	{
		LogDataChunk * data_chunk = (*(_flush_log_buffer->end() - 1));
		delete data_chunk;
		_flush_log_buffer->pop_back();
	}

	{
		AUTO_LOCK(_lock);
		if (IsWriteBufferHaveData())
		{
			LoggerMgr::Instance().PushNeedFlushLogger(this);
			_waiting_flush = true;
		}
		else
		{
			_waiting_flush = false;
		}
	}
}



void Logger::FlushLogDataChunk(LogDataChunk * data_chunk, int64_t now)
{
	if (data_chunk->cur_size <= 0)
	{
		assert(false);
		return;
	}

	assert(data_chunk->time > 0);

	if (_file)
	{
		assert(_open_file_time > 0 && _log_file_time);
		// 时间不在同一天，需要重新打开文件
		// 打开文件时间超过5秒，重新打开一次，避免外部强制将文件删除了无法写入
		if(!TimeHelper::IsInSameDay(_log_file_time, data_chunk->time) ||
			now - _open_file_time >= 5)
		{
			fclose(_file);
			_file = nullptr;
			_open_file_time = 0;
			_log_file_time = 0;
		}
	}

	// 打开文件
	if (_file == nullptr)
	{
		std::string full_file_name = LoggerMgr::Instance().GetLogDir() + GetLogFileName(data_chunk->time);
		_file = fopen(full_file_name.c_str(), "a");
		if (_file == nullptr)
		{
			std::cerr << "[ERROR] open log file(" << full_file_name << ") error" << std::endl;
			return;
		}

		_log_file_time = data_chunk->time;
		_open_file_time = now;
	}

	// 写入文件
	if (fwrite(data_chunk->data, data_chunk->cur_size, 1, _file) > 0)
	{
		fflush(_file);
	}
}

bool Logger::IsWriteBufferHaveData()
{
	if (_write_log_buffer->empty())
	{
		return false;
	}

	for (LogDataChunk * data_chunk : (*_write_log_buffer))
	{
		if (data_chunk->cur_size > 0)
		{
			return true;
		}
	}

	return false;
}

// 获取新的日志文件名
std::string Logger::GetLogFileName(int64_t cur_time)
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
	TimeHelper::LocalTime(cur_time, &cur_tm);
	sprintf(tail, "%04d%02d%02d.log", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday);

	if (!file_name.empty())
	{
		file_name.push_back('_');
	}
	file_name.append(tail);

	return file_name;
}



LoggerMgr::~LoggerMgr()
{
	if (_is_running)
	{
		_is_running = false;
		_wait_flush_logger_queue.Stop();
		_flush_log_thread->join();
		delete _flush_log_thread;
	}

	for (auto & pr : _loggers)
	{
		if (pr.second)
		{
			delete pr.second;
		}
	}
}

void LoggerMgr::Initialize(const std::string & log_dir, const std::string & log_base_name)
{
	if (_is_running)
	{
		return;
	}

	_log_base_name = log_base_name;
	// 设置日志目录
	_log_dir = log_dir;
	if (!_log_dir.empty())
	{
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

		if (!FileHelper::MakeDirectoryRecursive(_log_dir))
		{
			assert(false);
		}
	}

	_is_running = true;
	// 写日志线程
	assert(!_flush_log_thread);
	_flush_log_thread = new std::thread(LoggerMgr::ExecFlushLog, this);
}

Logger & LoggerMgr::GetLogger(const std::string & log_name)
{
	if (log_name.empty())
	{
		return _default_logger;
	}

	AUTO_LOCK(_lock);

	auto it = _loggers.find(log_name);
	if (it == _loggers.end())
	{
		Logger * logger = new Logger(log_name);
		assert(logger);
		if (!_loggers.insert(std::make_pair(log_name, logger)).second)
		{
			assert(false);
		}

		return *logger;
	}

	return *it->second;
}

void LoggerMgr::ExecFlushLog(LoggerMgr * log_mgr)
{
	while (log_mgr->_is_running)
	{
		Logger * wait_flush_logger = nullptr;
		if (log_mgr->_wait_flush_logger_queue.Pop(&wait_flush_logger))
		{
			assert(wait_flush_logger);
			wait_flush_logger->Flush();
		}
	}
}




LogStreamBuf::LogStreamBuf(Logger & logger, int64_t cur_time) : _cur_time(cur_time), _logger(logger)
{
	setp(_buf, _buf + kMaxBufferLen);
}

std::streambuf::int_type LogStreamBuf::overflow(std::streambuf::int_type c)
{
	if (c != EOF) 
	{
		*pptr() = (char)c;
		pbump(1);
	}

	
	int32_t len = (int32_t)(pptr() - pbase());
	if (len > 0)
	{
		_vec_ext_buf.push_back(std::vector<char>());
		std::vector<char> & buf = (*(_vec_ext_buf.end() - 1));
		buf.assign(_buf, _buf + len);
		pbump(-len);
	}

	return c;
}

int LogStreamBuf::sync()
{
	AUTO_LOCK(_logger.GetWriteLock());

	for (auto & ext_buf : _vec_ext_buf)
	{
		_logger.Write(_cur_time, ext_buf.data(), (int32_t)ext_buf.size());
	}

	_vec_ext_buf.clear();

	int32_t len = (int32_t)(pptr() - pbase());
	if (len > 0)
	{
		_logger.Write(_cur_time, _buf, len);
		pbump(-len);
	}

	return 0;
}

// 日志等级的文本
const char LogStream::kLogLevelText[kLogLevelCount][8] =
{
    "TRACE", "INFO", "WARN", "ERROR"
};

LogStream::LogStream(const std::string & log_name, LogLevel log_lv)
    : std::ostream(&_log_buf), _log_buf(LoggerMgr::Instance().GetLogger(log_name), TimeHelper::GetEpochSeconds()), _log_lv(log_lv)
{
    tm cur_tm;
    TimeHelper::LocalTime(_log_buf.GetCurTime(), &cur_tm);
	(*this) << (cur_tm.tm_year + 1900) << '-' << std::setw(2) << std::setfill('0') << (cur_tm.tm_mon + 1) << '-' << cur_tm.tm_mday 
		<< ' ' << cur_tm.tm_hour << ':' << cur_tm.tm_min << ':' << cur_tm.tm_sec << '|' << std::setw(0);
	
	if (log_lv >= kLogLevel_Begin && log_lv < kLogLevel_End)
	{
		(*this) << kLogLevelText[log_lv - kLogLevel_Begin] << '|';
	}
}

LogStream::~LogStream()
{
	this->flush();
}