#include<iostream>
#include<vector>
#include<time.h>
#include <cassert>

using std::cout;
using std::endl;

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

private:
	void* _freelist;// 空闲链表的头指针
};