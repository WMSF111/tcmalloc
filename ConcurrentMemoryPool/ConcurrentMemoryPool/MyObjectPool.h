#pragma once 
// 防止头文件被多次包含（通常称为“头文件保护”）
// 不需要使用传统的条件编译指令（如 #ifndef、#define、#endif）来实现

#include "common.h"

template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		// 这里要修复一个bug，这里是需要加锁的，否则多线程使用
		// 同一个对象池对象，存在bug
		std::unique_lock<std::mutex> lock(_mtx);
		if (_freelist)// 如果空闲链表不为空，头删空闲链表
		{
			void* next = *(void**)_freelist;
			obj = (T*)_freelist; // 返回空闲链表的头节点
			_freelist = next;
		}
		else
		{
			if (_remainpool < sizeof(T)) // 如果内存池剩余空间不足，则重新分配POOLNUM大小的内存
			{
				_remainpool = MEMORYNUM; // 设置剩余空间大小 1024 * 8
				//_memory = (char*)malloc(POOLNUM);
				_memory = (char*)SystemAlloc(_remainpool >> 13); // 分配POOLNUM >> 13 页的内存
				memset(_memory, 0, _remainpool); // 初始化分配的内存，解决了OBJ的指向NULL的问题
				if (_memory == nullptr)
					throw std::bad_alloc();
					/*throw std::bad_alloc;*/
			}
			obj = (T*)_memory;
			//assert(obj != nullptr);
			size_t objsize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			_memory += objsize;
			_remainpool -= objsize;
		}
		/*new(T);*/
		new(obj)T;
		return obj;
	}

	void Delete(T* obj) // 头插
	{
		std::unique_lock<std::mutex> lock(_mtx);
		//~T;
		obj->~T();
		*(void**)obj = _freelist;
		_freelist = obj;
	}
private:
	char* _memory = nullptr;
	size_t _remainpool = 0;
	void* _freelist = nullptr;
	std::mutex _mtx;
};

//struct TreeNode
//{
//	int val;
//	int* left = nullptr;
//	int* right = nullptr;
//	TreeNode()
//		:val(0)
//		,left(nullptr)
//		,right(nullptr)
//	{}
//};

//void TestObjectPool()
//{
//	const size_t Round = 5;
//	const size_t N = 10000;
//	
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//	size_t begin1 = clock();
//	for (int i = 0; i < Round; i++)
//	{
//		for (int j = 0; j < N; j++)
//		{
//			v1.push_back(new TreeNode);
//		}
//		for (int j = 0; j < N; j++)
//		{
//			delete v1[j];
//		}
//		v1.clear();
//	}
//	size_t end1 = clock();
//
//	std::vector<TreeNode*> v2;
//	v2.reserve(N);
//	ObjectPool<TreeNode> TNPool;
//	size_t begin2 = clock();
//	for (int i = 0; i < Round; i++)
//	{
//		for (int j = 0; j < N; j++)
//		{
//			v2.push_back(TNPool.New());
//		}
//		for (int j = 0; j < N; j++)
//		{
//			TNPool.Delete(v2[j]);
//		}
//		v2.clear();
//	}
//	size_t end2 = clock();
//	cout << "new cost time:" << end1 - begin1 << endl;
//	cout << "object pool cost time:" << end2 - begin2 << endl;
//}
