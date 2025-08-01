#pragma once  
#include "common.h"  

class ThreadCache
{
public:
	void* Allocate(size_t size); // 获取size大小的内存块
	void Deallocate(void* ptr, size_t size); // 删除ptr指针指向的size大小的对象
	void* FetchFromCentralCache(size_t index, size_t alignSize);
	
private:
	// FreeList _FreeList[]; 
	FreeList _FreeList[CACHENUM]; // 存储哈希桶
};

// _declspec(thread) 是 Microsoft Visual C++ 的一个扩展属性，用于指定变量的存储类别为线程局部存储
// static：意味着 pTLSThreadCache 只在包含该头文件的当前.cpp 文件中可见，不会被其他.cpp文件访问。
// 确保每个线程都有独立的 ThreadCache 实例，而不会相互干扰。
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;


