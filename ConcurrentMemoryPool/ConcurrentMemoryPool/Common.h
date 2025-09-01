#pragma once
#include<iostream>
#include<vector>
#include<time.h>
#include <cassert>
#include <thread>
#include <mutex>
#include <algorithm>
#include <unordered_map>


#ifdef _WIN32
	#include <windows.h>
#else
  //...
#endif

using std::cout;
using std::endl;

static const size_t MAX_SIZE = 256 * 1024; //单位是byte, 最大的哈希桶下标  == 256kib
static const size_t CACHENUM = 208; // 哈希桶数量
static const size_t MAX_PAGE = 129; // 哈希桶数量
static const size_t PAGE_SHIFT = 13;
//int MEMORYNUM = 128 * 1024; // 内存池大小
static const int MEMORYNUM = 128 * 1024; // 内存池大小 // static 整个项目只有这一个MEMORYNUM，避免多次定义问题

#ifdef _WIN64
	typedef unsigned long long ID_SIZE;
	static const size_t WIN_SIZE = 64;
#elif _WIN32
	typedef size_t ID_SIZE;
	static const size_t WIN_SIZE = 32;
#endif

// 直接去堆上按页申请空间
// inline: 内联函数，尝试将函数体直接嵌入调用处
// static: （当前.cpp文件）内，避免与其他编译单元中的同名函数冲突
// void: 返回指针类型，表示返回一个指向分配内存的指针
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	// windows下VirtualAlloc，参数：地址、大小、分配类型、保护属性，返回值：指向分配内存的指针
	// VirtualAlloc函数用于在进程的虚拟地址空间中保留、提交或更改一块内存区域。
	// 0 ：让系统自动选择分配内存的起始地址（推荐用法）
	// 要申请的总字节数为kpage * 2^13（8192：即 8KB）
	// MEM_RESERVE：预留一段虚拟地址空间（不占用物理内存）。
	// MEM_COMMIT：为预留的地址空间分配物理内存，并映射到虚拟内存。
	// PAGE_READWRITE: 设置内存区域的保护属性为可读写，表示该区域可以被读取和写入。
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		// linux下brk mmap等
#endif

		if (ptr == nullptr)
			throw std::bad_alloc();

		return ptr;
}


inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}

// 获取obj的下一个指针,*&使调用者可以直接改写这个指针，而不是得到一份拷贝。
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
		_Size++;
	}

	void* Pop() //头删
	{
		assert(_freelist); // 确保空闲链表不为空	
		void* obj = _freelist; // 记录当前空闲链表的头指针
		_freelist = NextObj(obj); // 将空闲链表的头指针指向下一个对象
		_Size--;
		return obj;
	}
	bool Empty()
	{
		return _freelist == nullptr;
	}

	size_t& MaxSize() // 返回最大可分配的对象大小
	{
		return _maxSize; // 返回指针大小的两倍，作为最大可分配对象大小
	}

	void PushRange(void* start, void* end, size_t size) //头插start-->end
	{
		assert(start && end);
		NextObj(end) = _freelist; // 将end的下一个指针指向当前空闲链表的头指针
		_freelist = start; // 将空闲链表的头指针指向start
		_Size += size;
	}

	void PopRange(void*& start, void*& end, size_t size) // 从空闲链表中弹出一段范围
	{
		assert(_freelist && _Size >= size);
		start = _freelist; // 记录当前空闲链表的头指针
		end = start; // 初始化end为start
		for (size_t i = 0; i < size - 1; ++i) // 循环size-1次，获取size个对象
		{
			end = NextObj(end); // 获取下一个对象
		}
		_freelist = NextObj(end); // 更新空闲链表的头指针为end的下一个对象
		NextObj(end) = nullptr; // 将end的下一个指针置空
		_Size -= size;
	}

	size_t Size()
	{
		return _Size;
	}
private:
	void* _freelist = nullptr;// 空闲链表的头指针
	size_t _maxSize = 1; // 最大可分配的对象大小，默认为1个指针大小
	size_t _Size = 0; // 当前空闲链表的对象数量
};

// 内部链接属性（internal linkage）:表示这个函数仅在当前翻译单元（当前.cpp 文件）可见。
class SizeClass  // 大小类，用于处理内存对齐和哈希桶下标计算
{
public:
	// inline: 提示编译器将函数体直接插入到调用点，解决重复定义问题（所有.cpp 共享同一个实体）
	// static: 让每个包含它的源文件都拿到一份独立的拷贝, 但会导致膨胀
	// static inline: 每个翻译单元各一份，互不干扰, 同时可以进行“内联优化”和“允许头文件定义”。
	static inline size_t _RoundSize(size_t size, size_t Alignsize) // 把 size 向上对齐到 Alignsize 的整数倍
	{
		return (size + Alignsize - 1) & ~(Alignsize - 1);
	}

	static inline size_t RoundSize(size_t size) // 对size进行对齐，并返回对齐后大小
	{
		if (size <= 128) // [0,16) * 8byte
		{
			return _RoundSize(size, 8);
		}
		else if (size <= 1024)// [16,72) * 16byte
		{
			return _RoundSize(size, 16);
		}
		else if (size <= 8 * 1024) // [72,128) * 128byte
		{
			return _RoundSize(size, 128);
		}
		else if (size <= 64 * 1024)// [128, 184) * 1024byte
		{
			return _RoundSize(size, 1024);
		}
		else if (size <= 256 * 1024)// [184, 208) * 8*1024byte
		{
			return _RoundSize(size, 8 * 1024);
		}
		else
		{
			/*assert(false);*/
			return _RoundSize(size, 1 << PAGE_SHIFT); // 按页对齐 
			return -1;
		}
	}
	// 步骤1：num + (1<<byte) - 1 → 对num向上取整到“对齐粒度”的整数倍（和之前RoundSize的对齐逻辑一致）；
	// 步骤2： >> byte → 等价于 除以(1 << byte)（因为byte是2的幂次，位运算更快）；
	// 步骤3： - 1 → 把“从1开始的序号”转换成“从0开始的下标”。
	static inline size_t _IndexUp(size_t num, size_t byte) // 返回num对应对齐（1<<btye)，大小对应的下标
	{
		return ((num + (1 << byte) - 1) >> byte) - 1; // 这里的byte是对齐粒度的指数，例如8字节对齐对应byte=3，16字节对齐对应byte=4，128字节对齐对应byte=7，1024字节对齐对应byte=10，8*1024字节对齐对应byte=13
	}

	static inline size_t IndexUp(size_t size) // 返回size对应的哈希桶下标
	// 把 “对齐后的内存大小” 映射到内存池哈希桶的唯一下标
	{
		assert(size <= MAX_SIZE);
		static size_t Freelist_size[4] = { 16, 56, 56, 56 }; // 各区间的桶数量
		if (size <= 128) // [0,16) 8byte， 16个桶
		{
			return _IndexUp(size, 3);
		}
		else if (size <= 1024)// [16,72) 16byte， 56个桶
		{
			return _IndexUp(size - 128, 4) + Freelist_size[0];
		}
		else if (size <= 8 * 1024) // [72,128) 128byte， 56个桶
		{
			return _IndexUp(size - 1024, 7) + Freelist_size[0] + Freelist_size[1];
		}
		else if (size <= 64 * 1024)// [128, 184) 1024byte， 56个桶
		{
			return _IndexUp(size - 8 * 1024, 10) + Freelist_size[0] + Freelist_size[1] + Freelist_size[2];
		}
		else if (size <= 256 * 1024)// [184, 208) 8*1024byte，24个桶
		{
			return _IndexUp(size - 256 * 1024, 13) + Freelist_size[0] + Freelist_size[1] + Freelist_size[2] + Freelist_size[3];
		}
		else
		{
			assert(false);
			return -1;
		}
	}
	
	static size_t NumMoveSize(size_t size) // 返回size对应的批量数量
	{
		// 确定 “一次批量拿多少个”
		assert(size <= MAX_SIZE);
		size_t retnum = MAX_SIZE / size;
		if (retnum < 2)
			retnum = 2;
		if (retnum > 512)
			retnum = 512; // 限制批量数量在2到512之间
		return retnum;
	}

	// 计算一次向系统获取几个页
	// 单个对象 8byte
	// ...
	// 单个对象 256KB
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size); // 获取size对应的批量数量
		size_t npage = num * size; // 计算总字节数

		npage >>= PAGE_SHIFT; // 将字节数转换为页数
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

// 管理多个连续页大块内存跨度结构
struct SpanNode // 跨度节点
{
	// 把“指针”降维成“压缩整型索引”,方便计算索引+n
	ID_SIZE _pageId = 0; // 跨度起始页号 
	size_t _n = 0; // 跨度包含的页数

	SpanNode* _next = nullptr; // 指向下一个跨度的指针
	SpanNode* _prev = nullptr; // 指向上一个跨度的指针

	size_t n_count = 0; // 分配给threadcahe的对象数量
	size_t _objsize = 0; // 跨度内存块中每个对象的大小
	void* freeList = nullptr; // 跨度内存块的空闲链表
	bool _isUse = false; // 标记该跨度是否正在被使用
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

	SpanNode* Begin() const // 返回跨度链表的头节点
	{
		return _head->_next; // 返回头节点的下一个节点
	}

	SpanNode* End() const // 返回跨度链表的尾节点
	{
		return _head; // 返回头节点本身作为尾节点
	}

	bool Empty() const // 判断跨度链表是否为空
	{
		return _head->_next == _head; // 如果头节点的下一个节点是头节点本身，则链表为空
	}

	SpanNode* PopFront()
	{
		SpanNode* span = _head->_next; // 获取头节点的下一个节点
		Erase(span); // 删除该节点
		return span;
	}

	void PushFront(SpanNode* Span)
	{
		Insert(Begin(), Span); // 在头节点前插入新的跨度节点
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
	std::mutex _mutex; // 跨度链表SpanList的互斥锁（桶锁） 
};