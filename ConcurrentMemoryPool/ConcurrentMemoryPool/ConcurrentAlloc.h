#pragma once

#include "common.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	
	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr, size_t size) // 为pTLSThreadCache释放ptr指针指向的size大小的对象
{
	assert(pTLSThreadCache);

	pTLSThreadCache->Deallocate(ptr, size);
}