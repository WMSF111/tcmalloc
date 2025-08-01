#pragma once
#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
	return nullptr;
}

void* ThreadCache::Allocate(size_t size) // 获取size大小的内存块
{
	assert(size < MAX_SIZE);
	size_t alignSize =  SizeClass::RoundSize(size);
	size_t index = SizeClass::IndexUp(size);
 	if (!_FreeList[index].Empty()) // 如果哈希桶index下标链表不为空
	{
		return _FreeList[index].Pop(); // 返回该链表头指针，并删除头节点
	}
	else// 如果哈希桶index下标链表为空
	{
		return FetchFromCentralCache(index, alignSize); // 向CentralCache借alignSize大小内存存入index下标哈希桶内
	}

}

void ThreadCache::Deallocate(void* ptr, size_t size) // 释放ptr指针指向的size大小的对象
{
	assert(ptr);
	assert(size <= MAX_SIZE);

	// 找对映射的自由链表桶，对象插入进入
	size_t index = SizeClass::IndexUp(size); // 获得哈希桶下标
	_FreeList[index].Push(ptr); // 在该下标链表插入ptr
}
