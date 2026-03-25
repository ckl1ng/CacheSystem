#ifndef SAFECACHE_H
#define SAFECACHE_H

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include "BloomFilter.h"
#include "ICachePolicy.hpp"
#include "LRUCache.h"
#include "LFUCache.h"
#include "ARCCache.h"

/**
 * @brief 缓存算法枚举
 */
enum class CacheAlgo { LRU, LFU, ARC };

/**
 * @brief 线程安全的分布式缓存系统封装
 * 
 * 功能特性：
 * 1. 分片（Sharding）：降低全局锁竞争。
 * 2. 布隆过滤器（Bloom Filter）：防止“缓存穿透”。
 * 3. 互斥锁池（Mutex Pool）：防止“缓存击穿”（同一热点 Key 只允许一个线程回源 DB）。
 * 4. 多算法支持：支持动态切换底层 LRU/LFU/ARC 策略。
 */
class SafeCache {
private:
    std::vector<std::unique_ptr<ICachePolicy>> shards; // 实际存储数据的分片容器
    size_t shard_num;                                  // 分片数量

    // 互斥锁池：用于“缓存击穿”保护。
    // 根据 Key 的 Hash 值映射到对应的锁，确保同一时刻只有一个线程去加载数据库。
    static const int SF_LOCK_POOL_SIZE = 128;
    std::mutex sf_locks[SF_LOCK_POOL_SIZE];

    BloomFilter blf; // 布隆过滤器，用于拦截非法 Key 请求

public:
    /**
     * @brief 构造一个安全的缓存系统
     * @param num 分片数量（建议设为 CPU 核心数的 1-4 倍）
     * @param capacity 总容量（平均分配到每个分片）
     * @param algo 淘汰算法类型
     */
    SafeCache(size_t num, size_t capacity, CacheAlgo algo = CacheAlgo::LRU);

    /**
     * @brief 将 Key 预热/注册到布隆过滤器
     */
    void addToBloomFilter(int key);

    /**
     * @brief 基础读取操作
     */
    std::optional<int> get(int key);

    /**
     * @brief 高级回源读取操作（核心防击穿逻辑）
     * 
     * 流程：
     * 1. 查布隆过滤器（防穿透）
     * 2. 查缓存，命中则返回
     * 3. 缓存失效，竞争“热点锁”
     * 4. 只有拿到锁的线程回源数据库（db_loader），其他线程阻塞等待
     * 5. 回源成功后写入缓存并唤醒其他线程
     * 
     * @param key 键
     * @param db_loader 数据库查询的回调函数
     * @param ttl_ms 基础过期时间
     * @return int 最终获取到的结果
     */
    int get(int key, std::function<int(int)> db_loader, int ttl_ms = 60000);

    /**
     * @brief 写入操作
     */
    void put(int key, int value, int ttl_ms);
};

#endif