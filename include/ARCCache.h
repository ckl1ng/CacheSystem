#ifndef ARCCACHE_H
#define ARCCACHE_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <optional>
#include "ICachePolicy.hpp"
#include "ExpirationGenerator.hpp"

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

/**
 * @brief 自适应替换缓存 (Adaptive Replacement Cache)
 * 
 * 动态结合 LRU 和 LFU 的优势，通过维护四个链表来应对真实业务下多变的访问模型：
 * - T1: 存放最近只访问过一次的真实数据 (类似 LRU)
 * - T2: 存放最近且被频繁访问的真实数据 (类似 LFU)
 * - B1: T1 的淘汰历史记录 (幽灵链表)
 * - B2: T2 的淘汰历史记录 (幽灵链表)
 */
 
class ARCCache : public ICachePolicy {
private:
    struct Entry {
        int value;
        std::list<int>::iterator it;
        int type;             
        TimePoint expire_at;  // TTL 过期时间
    };

    int c; // 缓存最大容量
    int p; // T1 的目标大小 (自适应权重参数)
    std::list<int> T1, T2, B1, B2;
    std::unordered_map<int, Entry> cache;
    
    std::mutex mtx;                 // 并发读写锁
    ExpirationGenerator ttl_gen;    // TTL 随机抖动生成器

    // 辅助函数：从当前所属的链表中移除节点
    void remove_from_list(Entry& e, int key);

    // 辅助函数：将节点晋升/移动到 T2 头部 (表示被高频访问)
    void promote_to_t2(int key);
    
    // 核心逻辑：根据权重 p 决定是从 T1 还是 T2 淘汰数据到幽灵链表
    void replace(int key);

    // 辅助函数：彻底删除幽灵链表末尾的节点，释放哈希表内存
    void delete_ghost_tail(std::list<int>& ghost_list);

public:
    ARCCache(int capacity = 10) : c(capacity), p(0) {}

    std::optional<int> get(int key) override;

    void put(int key, int val, int ttl_ms = 60000) override;
};

#endif