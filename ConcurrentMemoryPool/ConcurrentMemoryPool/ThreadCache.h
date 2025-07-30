#pragma once  
#include "common.h"  

class ThreadCache
{
public:
	void* Allocate(size_t size) // 获取size大小的内存块
	{

	}

	void Deallocate(void* ptr, size_t size) // 删除ptr指针指向的size大小的对象
	{

	}

	
private:
	FreeList _Freelist[];
};

