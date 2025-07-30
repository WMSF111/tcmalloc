#include<iostream>
#include<vector>
#include<time.h>
#include <cassert>

using std::cout;
using std::endl;

static size_t MAX_SIZE = 256 * 1024; //单位是byte
static size_t CACHENUM = 208;

void*& NextObj(void* obj) // 获取obj的下一个指针
{
	return *(void**)obj; // 将obj强制转换为void**类型，并返回其指向的下一个对象的指针
}

class FreeList
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
	void* _freelist;// 空闲链表的头指针
};

class SizeClass
{
public:
	static inline size_t _RoundSize(size_t size, size_t Alignsize)
	{
		return (size + Alignsize - 1) & ~(Alignsize - 1);
	}

	static inline size_t RoundSize(size_t size)
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

	static inline size_t _IndexUp(size_t num, size_t byte)
	{
		return ((num + (1 << byte) - 1) >> byte) - 1;
	}

	static inline size_t IndexUp(size_t size)
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