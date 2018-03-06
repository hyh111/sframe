
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

// 共享内存块
class ShmChunk
{
public:
	/*
	构造函数
	@shm_key: 共享内存的key(windows下使用字符串，linux下使用数字，这里统一为数字)
	@shm_size: 大小
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
