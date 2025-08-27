#pragma once
//#include<iostream>
//#include<vector>
//#include<time.h>
//using std::cout;
//using std::endl;
//
//#ifdef _WIN32
//#include<windows.h>
//#else
////
//#endif
#include"common.h"
// 直接去堆上按页申请空间
//inline static void* SystemAlloc(size_t kpage)
//{
//#ifdef _WIN32 // 一页大小固定为 8 KiB（8192字节）
//	// kpage << 13 等价于 kpage * 8192
//	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//#else
//	// linux下brk mmap等
//#endif
//
//	if (ptr == nullptr)
//		throw std::bad_alloc();
//
//	return ptr;
//}

template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		if (_freelist) // 还回来的代码区不为空则把freelist的内存优先给出去
		{
			/* *(void**) obj = _freelist;
			_freelist = *(void**)_freelist;*/
			// 头删
			void* next = *(void**)_freelist; // 保证freelist节点大小能存下next地址
			obj = (T *)_freelist; // 给出的就是T指针
			_freelist = next;
		}
		else
		{ 
			if (_remainmem < sizeof(T)) //当内存不足时开辟新内存
			{
				_remainmem = 128 * 1024;
				//_memory = (T)malloc(MEMORYNUM); // 错误写法
				//_memory = (char*)malloc(MEMORYNUM); // 调用malloc
				cout << (_remainmem >> 13); // MEMORYNUM 需要开辟多少页
				_memory = (char*)(SystemAlloc(_remainmem >> 13)); // 创建1块内存池需要_remainmem >> 13页
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory; //返回内存池第一块区域
			// 每次都添加一整块内存
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memory += objSize;
			_remainmem -= objSize;
			//_memory += sizeof(T);
			//_remainmem -= sizeof(T);
		}
		// 定位new，显示调用T的构造函数初始化
		new(obj)T;
		return obj;
	}

	void Delete(T* obj)
	{
		// 显示调用析构函数清理对象
		obj->~T();
		/*obj = *(void**)_freelist ;
		_freelist = obj;*/
		*(void**)obj = _freelist;
		_freelist = obj;
	}
private:
	char* _memory = nullptr; // 大块内存池的头指针
	size_t _remainmem = 0;
	void* _freelist = nullptr; //释放后回到内存池的链表头指针
};

//struct TreeNode
//{
//	int _val;
//	TreeNode* _left;
//	TreeNode* _right;
//
//	TreeNode()
//		:_val(0)
//		, _left(nullptr)
//		, _right(nullptr)
//	{}
//};

//void TestObjectPool()
//{
//	// 申请释放的轮次
//	const size_t Rounds = 5;
//
//	// 每轮申请释放多少次
//	const size_t N = 100000;
//
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//
//	size_t begin1 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v1.push_back(new TreeNode);
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			delete v1[i];
//		}
//		v1.clear();
//	}
//
//	size_t end1 = clock();
//
//	std::vector<TreeNode*> v2;
//	v2.reserve(N);
//
//	ObjectPool<TreeNode> TNPool;
//	size_t begin2 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v2.push_back(TNPool.New());
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			TNPool.Delete(v2[i]);
//		}
//		v2.clear();
//	}
//	size_t end2 = clock();
//
//	cout << "new cost time:" << end1 - begin1 << endl;
//	cout << "object pool cost time:" << end2 - begin2 << endl;
//}