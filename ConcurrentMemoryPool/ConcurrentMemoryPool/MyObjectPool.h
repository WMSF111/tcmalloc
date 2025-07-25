#include<iostream>
#include<vector>
#include<time.h>

using std::cout;
using std::endl;

#ifdef _WIN32
#include<Windows.h>
#else
//
#endif

size_t POOLNUM = 128 * 1024;

template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		if (_freelist)// Í·É¾
		{
			void* next = *(void**)_freelist;
			obj = (T*)_freelist;
			_freelist = next;
		}
		else
		{
			if (_remainpool < sizeof(T))
			{
				_remainpool = POOLNUM;
				_memory = (char*)malloc(POOLNUM);
				if (_memory == nullptr)
					throw std::bad_alloc();
					/*throw std::bad_alloc;*/
			}
			obj = (T*)_memory;
			size_t objsize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			_memory += objsize;
			_remainpool -= objsize;
		}
		/*new(T);*/
		new(obj)T;
		return obj;
	}

	void Delete(T* obj) // Í·²å
	{
		//~T;
		obj->~T();
		*(void**)obj = _freelist;
		_freelist = obj;
	}
private:
	char* _memory = nullptr;
	size_t _remainpool = 0;
	void* _freelist = nullptr;
};

struct TreeNode
{
	int val;
	int* left = nullptr;
	int* right = nullptr;
	TreeNode()
		:val(0)
		,left(nullptr)
		,right(nullptr)
	{}
};

void TestObjectPool()
{
	const size_t Round = 5;
	const size_t N = 10000;
	
	std::vector<TreeNode*> v1;
	v1.reserve(N);
	size_t begin1 = clock();
	for (int i = 0; i < Round; i++)
	{
		for (int j = 0; j < N; j++)
		{
			v1.push_back(new TreeNode);
		}
		for (int j = 0; j < N; j++)
		{
			delete v1[j];
		}
		v1.clear();
	}
	size_t end1 = clock();

	std::vector<TreeNode*> v2;
	v2.reserve(N);
	ObjectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	for (int i = 0; i < Round; i++)
	{
		for (int j = 0; j < N; j++)
		{
			v2.push_back(TNPool.New());
		}
		for (int j = 0; j < N; j++)
		{
			TNPool.Delete(v2[j]);
		}
		v2.clear();
	}
	size_t end2 = clock();
	cout << "new cost time:" << end1 - begin1 << endl;
	cout << "object pool cost time:" << end2 - begin2 << endl;
}
