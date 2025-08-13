#pragma once

#include "common.h"
#include "ThreadCache.h"

<<<<<<< HEAD
static void* ConcurrentAlloc(size_t size) // 为pTLSThreadCache分配size大小的内存块
=======
static void* ConcurrentAlloc(size_t size)
>>>>>>> 806d0c0b8398d55c87e438c4262547c725d81bc0
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	
	return pTLSThreadCache->Allocate(size);
}

<<<<<<< HEAD
static void ConcurrentFree(void* ptr, size_t size) // 为pTLSThreadCache释放ptr指针指向的size大小的对象
=======
static void ConcurrentFree(void* ptr, size_t size)
>>>>>>> 806d0c0b8398d55c87e438c4262547c725d81bc0
{
	assert(pTLSThreadCache);

	pTLSThreadCache->Deallocate(ptr, size);
}