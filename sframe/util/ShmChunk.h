
#pragma once
#ifndef SFRAME_SHM_CHUNK_H
#define SFRAME_SHM_CHUNK_H

#include <inttypes.h>

#ifndef __GNUC__
#include <string>
#include <windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

// ¹²ÏíÄÚ´æ¿é
class ShmChunk
{
public:
	/*
	¹¹Ôìº¯Êý
	@shm_key: ¹²ÏíÄÚ´æµÄkey(windowsÏÂÊ¹ÓÃ×Ö·û´®£¬linuxÏÂÊ¹ÓÃÊý×Ö£¬ÕâÀïÍ³Ò»ÎªÊý×Ö)
	@shm_size: ´óÐ¡
	*/
	ShmChunk(int32_t shm_key, int32_t shm_size) : _shm_key(shm_key), _shm_size(shm_size), _shm_ptr(nullptr)
	{
#ifndef __GNUC__
		_h_map = NULL;
#endif
	}

	~ShmChunk() {}

	int32_t GetShmKey() const
	{
		return _shm_key;
	}

	int32_t GetShmSize() const
	{
		return _shm_size;
	}

	void * GetShmPtr() const
	{
		return _shm_ptr;
	}

	bool Open(bool & is_new);

	void Close();

private:
	int32_t _shm_key;
	int32_t _shm_size;
	void * _shm_ptr;
#ifndef __GNUC__
	HANDLE _h_map;
#endif
};

#endif
