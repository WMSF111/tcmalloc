/*   线程级入口分配器：用于实现ThreadCache的获取与释放
注：pTLSThreadCache只在该函数调用，避免与ThreadCache中调用冲突    */

#pragma once

#include "common.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t size)// 获取size大小的pTLSThreadCache对象
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