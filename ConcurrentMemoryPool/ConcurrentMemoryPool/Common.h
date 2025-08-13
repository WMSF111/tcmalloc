#pragma once

#include<iostream>
#include<vector>
#include<time.h>
#include <cassert>
#include <thread>
#include <mutex>

using std::cout;
using std::endl;

static const size_t MAX_SIZE = 256 * 1024; //单位是byte
static const size_t CACHENUM = 208; // 哈希桶数量

#ifdef _WIN64
	typedef unsigned long long ID_SIZE;
#elif _WIN32
	typedef unsigned size_t ID_SIZE;
#endif

//void*& NextObj(void* obj) // 获取obj的下一个指针
// 获取obj的下一个指针,*&使调用者可以直接改写这个指针，而不是得到一份拷贝。
//void*& NextObj(void* obj) // 获取obj的下一个指针
static void*& NextObj(void* obj) // 获取obj的下一个指针
{
	return *(void**)obj; // 将obj强制转换为void**类型，并返回其指向的下一个对象的指针
}

class FreeList // 自由链表类，用于管理内存池中的空闲对象
{
public:
	void Push(void* obj) //头插
	{
		assert(obj); // 确保obj不为空
		NextObj(obj) = _freelist; // 将obj的下一个指针指向当前空闲链表的头指针
		_freelist = obj; // 将空闲链表的头指针指向obj
	}

	void* Pop() //头删
	{
		assert(_freelist); // 确保空闲链表不为空	
		void* obj = _freelist; // 记录当前空闲链表的头指针
		_freelist = NextObj(obj); // 将空闲链表的头指针指向下一个对象
		return obj;
	}
	bool Empty()
	{
		return _freelist == nullptr;
	}

private:
	void* _freelist = nullptr;// 空闲链表的头指针
};

// 内部链接属性（internal linkage）:表示这个函数仅在当前翻译单元（当前.cpp 文件）可见。
class SizeClass  // 大小类，用于处理内存对齐和哈希桶下标计算
{
public:
	// inline: 提示编译器将函数体直接插入到调用点，解决重复定义问题（所有.cpp 共享同一个实体）
	// static: 让每个包含它的源文件都拿到一份独立的拷贝, 但会导致膨胀
	// static inline: 每个翻译单元各一份，互不干扰, 同时可以进行“内联优化”和“允许头文件定义”。
	static inline size_t _RoundSize(size_t size, size_t Alignsize) // 将size对齐为Aliansize大小
	{
		return (size + Alignsize - 1) & ~(Alignsize - 1);
	}

	static inline size_t RoundSize(size_t size) // 对size进行对齐，并返回对齐后大小
	{
		if (size <= 128) // [0,16)
		{
			return _RoundSize(size, 8);
		}
		else if (size <= 1024)// [16,72)
		{
			return _RoundSize(size, 16);
		}
		else if (size <= 8 * 1024) // [72,128)
		{
			return _RoundSize(size, 128);
		}
		else if (size <= 64 * 1024)// [128, 184)
		{
			return _RoundSize(size, 1024);
		}
		else if (size <= 256 * 1024)// [184, 208)
		{
			return _RoundSize(size, 8 * 1024);
		}
		else
		{
			assert(false);
			return -1;
		}
	}

	static inline size_t _IndexUp(size_t num, size_t byte) // 返回num对应对齐（1<<btye)，大小对应的下标
	{
		return ((num + (1 << byte) - 1) >> byte) - 1;
	}

	static inline size_t IndexUp(size_t size) // 返回size对应的哈希桶下标
	{
		assert(size <= MAX_SIZE);
		static size_t Freelist_size[4] = { 16, 56, 56, 56 };
		if (size <= 128) // [0,16) 8byte
		{
			return _IndexUp(size, 3);
		}
		else if (size <= 1024)// [16,72) 16byte
		{
			return _IndexUp(size - 128, 4) + Freelist_size[0];
		}
		else if (size <= 8 * 1024) // [72,128) 128byte
		{
			return _IndexUp(size - 1024, 7) + Freelist_size[0] + Freelist_size[1];
		}
		else if (size <= 64 * 1024)// [128, 184) 1024byte
		{
			return _IndexUp(size - 8 * 1024, 10) + Freelist_size[0] + Freelist_size[1] + Freelist_size[2];
		}
		else if (size <= 256 * 1024)// [184, 208) 8*1024byte
		{
			return _IndexUp(size - 256 * 1024, 13) + Freelist_size[0] + Freelist_size[1] + Freelist_size[2] + Freelist_size[3];
		}
		else
		{
			assert(false);
			return -1;
		}
	}
};

// 管理多个连续页大块内存跨度结构
struct SpanNode // 跨度节点
{
	ID_SIZE _pageId = 0; // 跨度起始页号
	size_t _n = 0; // 跨度包含的页数

	SpanNode* _next = nullptr; // 指向下一个跨度的指针
	SpanNode* _prev = nullptr; // 指向上一个跨度的指针

	size_t n_count = 0; // 分配给threadcahe的对象数量
	void* freeList = nullptr; // 跨度内存块的空闲链表
};

// 带头双向循环链表
class SpanList // 跨度链表
{
public:
	SpanList()
	{
		_head = new SpanNode;
		_head->_next = _head;
		_head->_prev = _head;
	}
	
	void Insert(SpanNode* pos, SpanNode* newspan) // 在pos位置前插入newspan节点
	{
		assert(pos && newspan);

		SpanNode* prev = pos->_prev;
		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = pos;
		pos->_prev = newspan;
	}

	void Erase(SpanNode* span) // 删除span节点
	{
		assert(span);

		SpanNode* prev = span->_prev;
		SpanNode* next = span->_next;

		prev->_next = next;
		next->_prev = prev;
		// delete span; 不需要释放span节点
	}

private:
	SpanNode* _head = nullptr;
public:
	std::mutex _mutex; // 跨度链表的互斥锁
};