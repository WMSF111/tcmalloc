#pragma once
#include "ThreadCache.h"


void* ThreadCache::Allocate(size_t size) // 获取size大小的内存块
{
	assert(size < MAX_SIZE);
	size_t alignSize =  SizeClass::RoundSize(size);
	size_t index = SizeClass::IndexUp(size);
	if (!_Freelist[index].Empty())
	{
		return _Freelist[index].Pop();
	}
	else
	{
		
	}
	

}

void ThreadCache::Deallocate(void* ptr, size_t size) // 删除ptr指针指向的size大小的对象
{
	assert(ptr);
	assert(size <= MAX_SIZE);

	// 找对映射的自由链表桶，对象插入进入
	size_t index = SizeClass::IndexUp(size);
	_Freelist[index].Push(ptr);
}




void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	// 找对映射的自由链表桶，对象插入进入
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);
}
