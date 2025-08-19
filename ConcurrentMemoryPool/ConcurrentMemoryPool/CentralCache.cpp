#include "CentralCache.h"

CentralCache CentralCache::_instance; // 静态成员变量定义，初始化单例实例

// 获取一个非空的span
SpanNode* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	// 获取freeList不为空的span
	SpanNode* it = list.Begin(); // 获取哈希桶的头节点
	while (it != list.End())
	{
		if (it->freeList != nullptr)
			return it;
		else
			it = it->_next;
	}
	list._mutex.unlock(); // 解锁对应的哈希桶

	// 如果没有找到非空的span，则创建一个新的span
	PageCache::GetInstance()->_pageMtx.lock(); // 锁定PageCache的互斥锁
	//SpanNode* Span = PageCache::GetInstance()->newSpan(byte_size); // 创建新的span
	SpanNode* Span = PageCache::GetInstance()->newSpan(SizeClass::NumMovePage(byte_size)); // 创建新的span
	PageCache::GetInstance()->_pageMtx.unlock(); // 解锁PageCache的互斥锁


	// 对获取span进行切分，不需要加锁，因为这会其他线程访问不到这个span
	// 计算span的大块内存的起始地址和大块内存的大小，方便后续切分
	char* start = (char*)(Span->_pageId << PAGE_SHIFT); // 起始地址
	size_t size = Span->_n << PAGE_SHIFT; // 大块内存的大小
	char* end = start + size; // 结束地址

	// 把大块内存切成自由链表，通过尾插链接起来（地址连续）
	Span->freeList = start; // 设置span的空闲链表头指针为起start址
	start += byte_size; // 更新start为下一个对象的地址
	void* tail = Span->freeList; // tail指针，指向链表的起始地址
	int i =	1;
	while (start < end) // 当起始地址小于结束地址时，继续切分
	{
		i++;
		NextObj(tail) = start; // 将当前尾指针的下一个对象指针指向起始地址
		tail = NextObj(tail); // 更新尾指针为下一个对象指针
		start += byte_size; // 更新起始地址为下一个对象的地址
	}

	// 切好span以后，需要把span挂到桶里面去的时候，再加锁
	list._mutex.lock(); // 锁定对应的哈希桶
	list.PushFront(Span); // 表示获取到了span将其插入Spanlist链表

	return Span;
}

// 从中心缓存获取一定数量的对象给thread cache
// start: 返回的起始地址 end: 返回的结束地址 batchNum: 批量数量 size: 每个对象的大小
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::IndexUp(size); // 获取哈希桶下标
	_SpanList[index]._mutex.lock(); // 锁定对应的哈希桶
	// 获取一个非空的span
	SpanNode* span = GetOneSpan(_SpanList[index], size); // 获取_SpanList[index]中非空的span
	assert(span && span->freeList); // 确保span不为空且有空闲链表
	// 从span中的freeList链表中获取batchNum个对象
	// 如果不够batchNum个，有多少拿多少
	start = span->freeList;
	end = start;
	size_t retNum = 1; // 计数器，记录获取的对象数量
	size_t i = 0;
	while(i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end); // 获取下一个对象
		i++;
		retNum++;
	}//此时end指向未被获取的下一个对象
	span->freeList = NextObj(end); // 更新span的空闲链表头指针为end的下一个对象指针
	NextObj(end) = nullptr; // 将获取到的对象的下一个对象指针置空
	_SpanList[index]._mutex.unlock(); // 解锁对应的哈希桶
	return retNum; // 返回获取的对象数量
}