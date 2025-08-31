#pragma once

#include "Common.h"
#include "MyObjectPool.h"
#include "PageMap.h"
//#include "ObjectPool.h"

// PageCache类，用于管理内存页缓存
class PageCache
{
public:
	// 获取单例实例
	static PageCache* GetInstance()
	{
		return &_sinst; // 返回静态成员变量的地址
	}

	// 获取从对象到span的映射
	SpanNode* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCache(SpanNode* span);

	SpanNode* newSpan(size_t size); // 创建新的跨度节点，size为跨度大小

	std::mutex _pageMtx; // PageCache的互斥锁
	
private:
	SpanList _spanlist[MAX_PAGE]; // 哈希桶数组，每个哈希桶对应一个SpanList，用于存储不同大小的内存页

	//std::unordered_map<ID_SIZE, SpanNode*> _idSpanMap; // 对象到span的映射表，key为对象的页号，value为对应的span节点
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap; // 使用单级页映射表代替unordered_map
	PageCache() {};
	PageCache(const PageCache&) = delete; // 禁止拷贝构造
	PageCache& operator=(const PageCache&) = delete; // 禁止赋值操作
	static PageCache _sinst; // 静态成员变量，存储单例实例

	ObjectPool<SpanNode> _spanPool; // 用于管理Span节点的对象池
};
