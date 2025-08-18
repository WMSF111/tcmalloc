#include "PageCache.h"

PageCache PageCache::_sinst; // 静态成员变量初始化


SpanNode* PageCache::newSpan(size_t k) // 获取新的跨度节点，k为跨度大小
{
	assert(k > 0 && k <= MAX_PAGE); // 确保k在合法范围内
	if( !_spanlist[k].Empty())
		return _spanlist[k].PopFront(); // 如果跨度链表不为空，弹出头节点并返回
	// 如果跨度链表为空，创建新的跨度节点
	// 检测后面的页是否有空闲页可用
	for (int i = k; i < MAX_PAGE; i++)
	{
		if (!_spanlist[i].Empty()) // 如果跨度链表不为空
		{
			SpanNode* nSpan = _spanlist[i].PopFront(); // 弹出头节点
			SpanNode* kSpan = new SpanNode; // 创建新的跨度节点
			//在nSpan的头部切一个k页
			//k页返回
			//将nSpan挂到对应的哈希桶上
			kSpan->_pageId = nSpan->_pageId; // 设置新的跨度节点的起始页号
			kSpan->_n = k; // 设置新的跨度节点的页数

			nSpan->_pageId += k; // 更新nSpan的起始页号
			nSpan->_n -= k; // 更新nSpan的页数
			_spanlist[nSpan->_n].PushFront(nSpan);
			return kSpan; // 返回新的跨度节点
		}
	}
	// 如果没有找到合适的跨度节点，创建新的跨度节点128
	SpanNode* bigkSpan = new SpanNode; // 创建新的跨度节点
	void* ptr = SystemAlloc(MAX_PAGE - 1); // 分配128页的内存(每页2^13字节)
	bigkSpan->_pageId = (ID_SIZE)ptr >> PAGE_SHIFT; // 设置新的跨度节点的起始页号
	bigkSpan->_n = MAX_PAGE - 1; // 设置新的跨度节点的页数为PAGE_MAX
	_spanlist[bigkSpan->_n - 1].PushFront(bigkSpan); // 将新的跨度节点插入到对应的跨度链表中

	return newSpan(k); // 递归调用获取k页的跨度节点
}