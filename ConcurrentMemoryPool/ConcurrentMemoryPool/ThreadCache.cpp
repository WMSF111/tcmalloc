#pragma once
#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) // 从CentralCache中获取alignSize大小的内存块，并存入哈希桶index下标链表中
{
	//使用慢开始反馈调节
	// 1、最开始不会一次向central cache一次批量要太多，因为要太多了可能用不完
	// 2、如果你不要这个size大小内存需求，那么batchNum就会不断增长，直到上限
	// 3、size越大，一次向central cache要的batchNum就越小
	// 4、size越小，一次向central cache要的batchNum就越大
	size_t batchNum = min(_FreeList[index].MaxSize(), SizeClass::NumMoveSize(size)); //Windows.h 与algrithm.h中都有min函数
	if (batchNum == _FreeList[index].MaxSize())
		_FreeList[index].MaxSize() += 1;
		//batchNum += 1; // batchNum加1，避免每次都只要一个对象
	void* start = nullptr;
	void* end = nullptr;
	size_t FetchNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size); // 从CentralCache获取batchNum个size大小的对象
	assert(FetchNum > 0);
	if (FetchNum == 1)// 如果只获取了一个对象
		assert(start == end);
	else
		_FreeList[index].PushRange(NextObj(start), end); // 将获取的对象范围[start, end]插入到哈希桶index下标链表中
	return start;
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
