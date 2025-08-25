/*   线程级入口分配器：用于实现ThreadCache的获取与释放
注：pTLSThreadCache只在该函数调用，避免与ThreadCache中调用冲突    */

#pragma once

#include "common.h"
#include "ThreadCache.h"
#include "PageCache.h"

static void* ConcurrentAlloc(size_t size)// 获取size大小的pTLSThreadCache对象
{
	if (size > MAX_SIZE) // 如果size大于MAX_SIZE，则直接向PageCache申请内存
	{
		size_t alignsize = SizeClass::RoundSize(size); // 计算对齐后的大小
		size_t pagenum = alignsize >> PAGE_SHIFT; // 计算需要的页数
		PageCache::GetInstance()->_pageMtx.lock(); // 锁定PageCache的互斥锁
		SpanNode* Span = PageCache::GetInstance()->newSpan(pagenum); // 向PageCache申请pagenum页内存
		PageCache::GetInstance()->_pageMtx.unlock(); // 解锁PageCache的互斥锁
		return (void*)(Span->_pageId << PAGE_SHIFT); // 返回申请的内存起始地址
	}
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			pTLSThreadCache = new ThreadCache;
		}
		cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

		return pTLSThreadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr, size_t size) // 为pTLSThreadCache释放ptr指针指向的size大小的对象
{
	if (size > MAX_SIZE) // 如果size大于MAX_SIZE，则直接向PageCache释放内存
	{
		assert(ptr);
		SpanNode* Span = PageCache::GetInstance()->MapObjectToSpan(ptr); // 获取ptr指针所在的span
		PageCache::GetInstance()->_pageMtx.lock(); // 锁定PageCache的互斥锁
		//assert(Span && Span->_isUse == true); // 确保span存在且正在使用
		PageCache::GetInstance()->ReleaseSpanToPageCache(Span); // 释放span
		PageCache::GetInstance()->_pageMtx.unlock(); // 解锁PageCache的互斥锁
	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}