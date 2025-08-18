#pragma once
#include "Common.h"
#include "PageCache.h"

// 单例模式（饿汉模式）实现中央缓存类，用于管理不同大小的内存块
class CentralCache
{
public:
	// 获取单例实例
	static CentralCache* GetInstance()
	{
		return &_instance; // 返回静态成员变量的地址
	}

	// 获取一个非空的span
	SpanNode* GetOneSpan(SpanList& list, size_t byte_size);

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);


private:
	SpanList _SpanList[CACHENUM]; // 哈希桶数组，每个哈希桶对应一个SpanList，用于存储不同大小的内存块
private:
	// 禁止三板斧
	CentralCache() = default; // 私有构造函数，禁止外部实例化
	CentralCache(const CentralCache&) = delete; // 禁止拷贝构造
	CentralCache& operator=(const CentralCache&) = delete; // 禁止赋值操作

	static CentralCache _instance; // 静态成员变量，存储单例实例
};
