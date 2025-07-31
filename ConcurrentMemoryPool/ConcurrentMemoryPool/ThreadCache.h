#pragma once  
#include "common.h"  

class ThreadCache
{
public:
	void* Allocate(size_t size); // 获取size大小的内存块
	void Deallocate(void* ptr, size_t size); // 删除ptr指针指向的size大小的对象
	void* FetchFromCentralCache(size_t index, size_t alignSize);
	
private:
	FreeList _FreeList[]; // 存储哈希桶
};

