#include "CentralCache.h"

CentralCache CentralCache::_instance; // 静态成员变量定义，初始化单例实例

// 获取一个非空的span
SpanNode* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	return nullptr;
}

// 从中心缓存获取一定数量的对象给thread cache
// start: 返回的起始地址 end: 返回的结束地址 batchNum: 批量数量 size: 每个对象的大小
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::IndexUp(size); // 获取哈希桶下标
	_SpanList[index]._mutex.lock(); // 锁定对应的哈希桶
	// 获取一个非空的span
	SpanNode* span = GetOneSpan(_SpanList[index], size); // 获取非空的span
	assert(span && span->freeList); // 确保span不为空且有空闲链表
	// 从span中获取batchNum个对象
	// 如果不够batchNum个，有多少拿多少
	start = span->freeList;
	end = start;
	size_t retNum = 0; // 计数器，记录获取的对象数量
	size_t i = 0;
	while(i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end); // 获取下一个对象
		i++;
		retNum++;
	}
	span->freeList = NextObj(end); // 更新span的空闲链表头指针为end的下一个对象指针
	NextObj(end) = nullptr; // 将end的下一个对象指针置空
	_SpanList[index]._mutex.unlock(); // 解锁对应的哈希桶
	return retNum; // 返回获取的对象数量
}