#include "SafeCache.h"
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>

// 模拟数据库查询函数
int MockDBLoader(int key) {
    // 模拟数据库查询的延迟（200毫秒）
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return key * 10; 
}

int main() {
    int cnt = 32;
    SafeCache lruSystem(cnt, 100, CacheAlgo::LFU);
    
    const int test_key = 888;
    
    //将合法 Key 注册进布隆过滤器（模拟白名单预热
    lruSystem.addToBloomFilter(test_key);

    std::vector<std::thread> workers;
    std::cout << "--- 开始测试 ---" << std::endl;
    std::cout << "本次测试使用 " << cnt << " 分片的系统" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 16; ++i) {
        workers.emplace_back([&lruSystem]() {
        for (int j = 1; j < 100000; j ++){
                lruSystem.put(j, j, 60000);
                lruSystem.get(j, MockDBLoader, 60000);
            }
        });
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "本次测试耗时：   " << elapsed.count() << "ms" << std::endl;
    
    std::cout << "--- 测试结束 ---" << std::endl;
    return 0;
}