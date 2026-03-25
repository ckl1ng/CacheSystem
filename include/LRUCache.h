#ifndef LRUCACHE_H
#define LRUCACHE_H

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
 * @brief 最近最少使用缓存算法 (Least Recently Used Cache)
 * 
 * LRU 算法通过维护数据的访问时间顺序，在缓存满时优先淘汰最久未被访问的数据。
 * 核心设计：
 * 1. 双向链表 (std::list)：按访问顺序存储节点，表头为最近访问，表尾为最久未访问。
 * 2. 哈希表 (std::unordered_map)：存储 Key 到链表迭代器的映射，实现 O(1) 查找。
 * 3. 线程安全：内置互斥锁，支持多线程并发访问。
 * 4. 自动过期：每个节点带有 TTL，防止陈旧数据堆积。
 */
class LRUCache : public ICachePolicy {
private: 
    /**
     * @brief 缓存节点结构
     */
    struct Node {
        int key;            // 业务键
        int val;            // 业务值
        TimePoint expire_at; // 绝对过期时间点
    };

    int capacity_;          // 缓存最大容量限制
    std::mutex mtx;         // 互斥锁，保护链表和哈希表的操作原子性

    /**
     * @brief 访问顺序链表
     * 新插入或被访问的节点会移动到 list 的 front（头部）。
     * 当容量不足时，从 list 的 back（尾部）移除最旧的节点。
     */
    std::list<Node> lst_;

    /**
     * @brief 快速索引表
     * Key: 业务键
     * Value: 指向 lst_ 中对应节点的迭代器，用于 O(1) 速度提取或修改节点位置
     */
    std::unordered_map<int, std::list<Node>::iterator> fd_;

    ExpirationGenerator ttl_gen; // 过期时间生成器，支持随机抖动（防止雪崩）

public:
    /**
     * @brief 构造函数
     * @param now_capacity 缓存容量，默认为 10
     */
    LRUCache(int now_capacity = 10) : capacity_(now_capacity) {}
    
    /**
     * @brief 获取缓存值
     * 逻辑：
     * 1. 查找哈希表。
     * 2. 若不存在，返回 nullopt。
     * 3. 若存在但已过期：从哈希表和链表中删除，返回 nullopt。
     * 4. 若存在且有效：将节点移动到链表头部（表示最近刚用过），返回数据。
     * 
     * @param key 键
     * @return std::optional<int> 命中且有效则返回数值，否则返回 nullopt
     */
    std::optional<int> get(int key) override;
    
    /**
     * @brief 插入或更新缓存
     * 逻辑：
     * 1. 若 Key 已存在：更新 Value，移动到链表头部，更新过期时间。
     * 2. 若 Key 不存在：
     *    a. 若缓存已满：删除链表尾部（最旧）的节点，并同步清理哈希表。
     *    b. 创建新节点，存入链表头部，并在哈希表中记录位置。
     * 
     * @param key 键
     * @param value 值
     * @param ttl_ms 基础生存时间
     */
    void put(int key, int value, int ttl_ms = 60000) override;
};

#endif