#ifndef LFUCACHE_H
#define LFUCACHE_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <optional>
#include "ICachePolicy.hpp"
#include "ExpirationGenerator.hpp"

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

/**
 * @brief 最不经常使用缓存算法 (Least Frequently Used Cache)
 * 
 * LFU 算法根据数据的访问频率来决定淘汰对象。当缓存满时，优先淘汰访问次数最少的数据。
 * 本实现通过以下技术点保证高效与安全：
 * 1. 双哈希表结构：实现 O(1) 的查询、插入和删除。
 * 2. 频率链表：相同频率的节点按进入先后顺序排列，频率最小时从链表末尾淘汰。
 * 3. TTL 过期机制：结合 ExpirationGenerator 防止缓存雪崩。
 * 4. 线程安全：内置互斥锁保证并发安全。
 */
class LFUCache : public ICachePolicy {
private:
    /**
     * @brief 缓存节点结构
     */
    struct Node {
        int key;            // 键
        int val;            // 值
        int freq;           // 访问频率
        TimePoint expire_at; // 绝对过期时间点
    };

    int capacity_;          // 缓存最大容量
    int min_freq_;          // 记录当前整个缓存中的最小访问频率，用于淘汰
    std::mutex mtx;         // 保证多线程环境下的原子性操作

    /**
     * @brief 快速查找哈希表
     * Key: 业务键
     * Value: 指向对应频率链表中节点的迭代器，用于 O(1) 定位节点
     */
    std::unordered_map<int, std::list<Node>::iterator> key_table_;

    /**
     * @brief 频率分布表
     * Key: 访问频率 (1, 2, 3...)
     * Value: 一个双向链表，存放所有访问频率等于该 Key 的节点
     */
    std::unordered_map<int, std::list<Node>> freq_table_;

    ExpirationGenerator ttl_gen; // 过期时间生成器（带随机抖动）

public:
    /**
     * @brief 构造函数
     * @param now_capacity 缓存容量，默认为 10
     */
    LFUCache(int now_capacity = 10) : capacity_(now_capacity), min_freq_(1) {}
    
    /**
     * @brief 获取缓存值
     * 逻辑：
     * 1. 检查 Key 是否存在且未过期。
     * 2. 若命中，则该节点的访问频率 +1。
     * 3. 将节点从旧频率链表移动到新频率（旧频率+1）链表的头部。
     * 4. 更新 min_freq_。
     * 
     * @param key 键
     * @return std::optional<int> 命中且有效则返回数值，否则返回 nullopt
     */
    std::optional<int> get(int key) override;
    
    /**
     * @brief 插入或更新缓存
     * 逻辑：
     * 1. 若 Key 已存在：更新 Value，增加频率，更新过期时间。
     * 2. 若 Key 不存在：
     *    a. 若缓存已满：在 freq_table_[min_freq_] 的链表末尾删除最旧的节点。
     *    b. 插入新节点，初始频率设为 1，重置 min_freq_ 为 1。
     * 
     * @param key 键
     * @param value 值
     * @param ttl_ms 基础生存时间
     */
    void put(int key, int value, int ttl_ms = 60000) override;
};

#endif