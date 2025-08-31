#include "PageCache.h"

PageCache PageCache::_sinst; // 静态成员变量初始化

SpanNode* PageCache::newSpan(size_t k) // 获取新的跨度（页数大小）节点，k为跨度大小
{
	assert(k > 0); // 确保k在合法范围内
	if (k > MAX_PAGE - 1) // 如果k大于最大页数，直接从系统获取内存
	{
		void* ptr = SystemAlloc(k);
		//SpanNode* Span = new SpanNode; // 创建新的跨度节点
		SpanNode* Span = ObjectPool<SpanNode>().New(); // 使用对象池创建新的跨度节点
		Span->_pageId = (ID_SIZE)ptr >> PAGE_SHIFT; // 设置跨度节点的起始页号
		Span->_n = k; // 设置跨度节点的页数
		//_idSpanMap[(ID_SIZE)Span->_pageId] = Span; // 将跨度节点的起始页号映射到跨度节点
		_idSpanMap.set((ID_SIZE)Span->_pageId, Span); // 更新映射表，将span的起始页号映射到span
		return Span; // 返回新的跨度节点
	}
	// 从跨度链表中获取页数为k的节点
	if (!_spanlist[k].Empty()) // 页数为k的节点数量为0
	{
		SpanNode* kSpan = _spanlist[k].PopFront(); // 如果跨度链表不为空，弹出头节点
		//将kSpan划分成2需要的内存大小，并在map中进行映射
		for (ID_SIZE i = 0; i < kSpan->_n; i++)
			//_idSpanMap[(ID_SIZE)kSpan->_pageId + i] = kSpan;
			_idSpanMap.set((ID_SIZE)kSpan->_pageId, kSpan); // 更新映射表，将span的起始页号映射到span
		return kSpan;
	}
		
	// 如果跨度链表为空，创建新的跨度节点
	// 检测后面的页是否有空闲页可用
	for (int i = k; i < MAX_PAGE; i++)
	{
		if (!_spanlist[i].Empty()) // 如果跨度链表不为空
		{
			SpanNode* nSpan = _spanlist[i].PopFront(); // 弹出头节点
			//SpanNode* kSpan = new SpanNode; // 创建新的跨度节点
			SpanNode* kSpan = _spanPool.New();
			//在nSpan的头部切一个k页
			//k页返回
			//将nSpan挂到对应的哈希桶上
			kSpan->_pageId = nSpan->_pageId; // 设置新的跨度节点的起始页号
			kSpan->_n = k; // 设置新的跨度节点的页数

			nSpan->_pageId += k; // 更新nSpan的起始页号
			nSpan->_n -= k; // 更新nSpan的页数
			_spanlist[nSpan->_n].PushFront(nSpan);

			// 存储pageCache中的nSpan 首位ID与Span映射方便PageCache合并
			//_idSpanMap[(ID_SIZE)nSpan->_pageId] = nSpan; // 将新的跨度节点的起始页号映射到nSpan
			//_idSpanMap[(ID_SIZE)nSpan->_pageId + nSpan->_n - 1] = nSpan; // 将新的跨度节点的结尾页号映射到nSpan
			_idSpanMap.set((ID_SIZE)nSpan->_pageId, nSpan); // 更新映射表，将span的起始页号映射到span
			_idSpanMap.set((ID_SIZE)nSpan->_pageId + nSpan->_n - 1, nSpan); // 更新映射表，将span的起始页号映射到span

			for(ID_SIZE i = 0; i < kSpan->_n; i++)
				//_idSpanMap[(ID_SIZE)kSpan->_pageId + i] = kSpan;
				_idSpanMap.set((ID_SIZE)kSpan->_pageId, kSpan); // 更新映射表，将span的起始页号映射到span

			return kSpan; // 返回新的跨度节点
		}
	}
	// 如果没有找到合适的跨度节点，创建新的跨度节点128
	//SpanNode* bigkSpan = new SpanNode; // 创建新的跨度节点
	SpanNode* bigkSpan = _spanPool.New();
	void* ptr = SystemAlloc(MAX_PAGE - 1); // 分配128页的内存(每页2^13字节)
	bigkSpan->_pageId = (ID_SIZE)ptr >> PAGE_SHIFT; // 设置新的跨度节点的起始页号
	bigkSpan->_n = MAX_PAGE - 1; // 设置新的跨度节点的页数为PAGE_MAX
	_spanlist[bigkSpan->_n].PushFront(bigkSpan); // 将新的跨度节点插入到对应的跨度链表中

	return newSpan(k); // 递归调用获取k页的跨度节点
}

// 获取从对象地址到span的映射
SpanNode* PageCache::MapObjectToSpan(void* obj)
{
	//std::unique_lock<std::mutex> lock(_pageMtx); // 锁定PageCache的互斥锁,出了作用域自动解锁
	ID_SIZE pageId = (ID_SIZE)obj >> PAGE_SHIFT; // 将对象地址转换为页号
	//auto it = _idSpanMap.find(pageId); // 在映射表中查找对应的span节点
	//if (it != _idSpanMap.end()) // 找到了span节点
	//	return it->second; // 返回对应的span节点
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//}
	auto it = (SpanNode*)_idSpanMap.get(pageId); // 在映射表中查找对应的span节点
	assert(it != nullptr); // 确保找到了span节点
	return it; // 返回对应的span节点
}

// 释放空闲span回到Pagecache，并合并相邻的span
void PageCache::ReleaseSpanToPageCache(SpanNode* span)
{
	if (span->_n > MAX_PAGE - 1) // 如果span的页数大于最大页数，直接释放内存
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT); // 计算span的起始地址
		SystemFree(ptr); // 释放内存
		//_idSpanMap.erase(span->_pageId); // 从映射表中删除span的起始页号映射
		//delete span; // 删除span节点
		_spanPool.Delete(span); 
		return;
	}
	// 对span前后的页，尝试进行合并，缓解内存碎片问题
	while (1)
	{
		ID_SIZE prevId = span->_pageId - 1;
		//auto ret = _idSpanMap.find(prevId);
		//// 前面的页号没有，不合并了
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}
		auto ret = (SpanNode*) _idSpanMap.get(prevId);
		assert(ret != nullptr);

		// 前面相邻页的span在使用，不合并了
		//SpanNode* prevSpan = ret->second;
		SpanNode* prevSpan = ret;
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// 合并出超过128页的span没办法管理，不合并了
		if (prevSpan->_n + span->_n > MAX_PAGE - 1)
		{
			break;
		}

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanlist[prevSpan->_n].Erase(prevSpan);
		//delete prevSpan;
		ObjectPool<SpanNode>().Delete(prevSpan);
	}

	// 向后合并
	while (1)
	{
		ID_SIZE nextId = span->_pageId + span->_n;
		/*auto ret = _idSpanMap.find(nextId);
		if (ret == _idSpanMap.end())
		{
			break;
		}*/
		auto ret = (SpanNode*)_idSpanMap.get(nextId);
		assert(ret != nullptr);

		//SpanNode* nextSpan = ret->second;
		SpanNode* nextSpan = ret;
		if (nextSpan->_isUse == true)
		{
			break;
		}

		if (nextSpan->_n + span->_n > MAX_PAGE - 1)
		{
			break;
		}

		span->_n += nextSpan->_n;

		_spanlist[nextSpan->_n].Erase(nextSpan);
		//delete nextSpan;
		_spanPool.Delete(nextSpan);
	}
	//前后合并后
	_spanlist[span->_n].PushFront(span); // 将合并后的span插入到对应的跨度链表中
	span->_isUse = false;
	//_idSpanMap[span->_pageId] = span; // 更新映射表，将span的起始页号映射到span
	//_idSpanMap[span->_pageId + span->_n - 1] = span; // 将span的结尾页号映射到span
	_idSpanMap.set(span->_pageId, span); // 更新映射表，将span的起始页号映射到span
	_idSpanMap.set(span->_pageId + span->_n - 1, span); // 将span的结尾页号映射到span

}