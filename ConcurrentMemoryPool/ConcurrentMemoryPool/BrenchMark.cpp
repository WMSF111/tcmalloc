#include"ConcurrentAlloc.h"

// ntimes 一轮申请和释放内存的次数
// nworks 线程数
// rounds 轮次
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0; // 原子型无符号整数变量 在多线程环境中安全地累加或读取计时/计数，不会出现数据竞争
	std::atomic<size_t> free_costtime = 0;

	//这里用lambda函数在 线程启动时 把“循环次数、计时变量、容器”这些局部状态一次性打包带走，而普通函数做不到这种“闭包”能力
	for (size_t k = 0; k < nworks; ++k) // 循环创建nworks个线程
	{
		//& 表示“按引用捕获外层作用域所有变量”
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(ntimes); // 预分配内存 避免频繁扩容

			for (size_t j = 0; j < rounds; ++j) // 每个线程执行rounds轮次
			{
				size_t begin1 = clock(); //申请内存 轮数开始时间
				for (size_t i = 0; i < ntimes; i++) // 每轮次申请ntimes次内存
				{
					v.push_back(malloc(16));
					//v.push_back(malloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock(); // 申请内存 轮数结束时间

				size_t begin2 = clock(); //释放内存 轮数开始时间
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();// 释放内存 轮数结束时间
				v.clear();

				malloc_costtime += (end1 - begin1); // 累加申请内存所花费的时间
				free_costtime += (end2 - begin2); // 累加释放内存所花费的时间
			}
			});
	}

	for (auto& t : vthread) //等待所有线程结束
	{
		t.join();  // std::thread 对象在析构前必须被 join() 或 detach()
	}

	printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);
}


// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(ConcurrentAlloc(16));
					//v.push_back(ConcurrentAlloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);
}

int main()
{
	size_t n = 1000;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 10);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 10);
	cout << "==========================================================" << endl;

	return 0;
}