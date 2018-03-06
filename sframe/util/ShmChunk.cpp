
#include "ShmChunk.h"

#ifndef __GNUC__

bool ShmChunk::Open(bool & is_new)
{
	is_new = false;
	std::string shm_name = std::to_string(_shm_key);
	_h_map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name.c_str());
	if (_h_map == NULL)
	{
		_h_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)_shm_size, shm_name.c_str());
		if (_h_map == NULL)
		{
			return false;
		}
		is_new = true;
	}

	_shm_ptr = MapViewOfFile(_h_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (_shm_ptr == nullptr)
	{
		CloseHandle(_h_map);
		_h_map = NULL;
		return false;
	}

	return true;
}

void ShmChunk::Close()
{
	if (_shm_ptr)
	{
		UnmapViewOfFile(_shm_ptr);
		CloseHandle(_h_map);
	}
}

#else

bool ShmChunk::Open(bool & is_new)
{
	is_new = true;
	//Èç¹ûkey¹²ÏíÄÚ´æÒÑÓÐ ·µ»Ø-1 ·ñÔòÔò´´½¨
	int shm_id = shmget(_shm_key, _shm_size, IPC_CREAT | IPC_EXCL | 0666);
	if (shm_id < 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}

		is_new = false;
	}

	//ÊÇÒÑÓÐµÄ Ôò»ñÈ¡ÒÔÇ°µÄShmId
	if (shm_id < 0)
	{
		shm_id = shmget(_shm_key, _shm_size, 0666);
		if (shm_id < 0)
		{
			return false;
		}
	}

	_shm_ptr = shmat(shm_id, nullptr, 0);
	if (_shm_ptr == nullptr || (int64_t)_shm_ptr == -1)
	{
		return false;
	}

	return true;
}

void ShmChunk::Close()
{
	if (_shm_ptr)
	{
		shmdt(_shm_ptr);
		_shm_ptr = nullptr;
	}
}

#endif