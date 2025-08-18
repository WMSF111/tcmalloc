#pragma once

#include "Common.h"

// PageCache类，用于管理内存页缓存
class PageCache
{
public:
	// 获取单例实例
	static PageCache* GetInstance()
	{
		return &_sinst; // 返回静态成员变量的地址
	}

	SpanNode* newSpan(size_t size); // 创建新的跨度节点，size为跨度大小

	std::mutex _pageMtx;

private:
	SpanList _spanlist[MAX_PAGE]; // 哈希桶数组，每个哈希桶对应一个SpanList，用于存储不同大小的内存页
	PageCache() {};
	PageCache(const PageCache&) = delete; // 禁止拷贝构造
	PageCache& operator=(const PageCache&) = delete; // 禁止赋值操作
	static PageCache _sinst; // 静态成员变量，存储单例实例
};
