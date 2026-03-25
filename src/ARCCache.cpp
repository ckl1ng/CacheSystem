#include "ARCCache.h"

// 定义链表类型常量
enum ListType { TYPE_T1 = 1, TYPE_T2 = 2, TYPE_B1 = 3, TYPE_B2 = 4 };

void ARCCache::remove_from_list(Entry& e, int key) {
    if (e.type == TYPE_T1) T1.erase(e.it);
    else if (e.type == TYPE_T2) T2.erase(e.it);
    else if (e.type == TYPE_B1) B1.erase(e.it);
    else if (e.type == TYPE_B2) B2.erase(e.it);
}

void ARCCache::delete_ghost_tail(std::list<int>& ghost_list) {
    if (!ghost_list.empty()) {
        int tail_key = ghost_list.back();
        ghost_list.pop_back();
        cache.erase(tail_key); // 彻底从哈希表中移除幽灵数据
    }
}

void ARCCache::replace(int key) {
    // 情况 1: T1 占用超过目标 p；或者当 key 在 B2 中且 T1 达到目标 p 时
    // 我们从 T1 淘汰数据到 B1
    if (!T1.empty() && (T1.size() > (size_t)p || (cache.count(key) && cache[key].type == TYPE_B2 && T1.size() == (size_t)p))) {
        int old_key = T1.back();
        T1.pop_back();
        
        // 将数据从 T1 移动到 B1 (变为幽灵，不再存储真实 value)
        B1.push_front(old_key);
        cache[old_key].it = B1.begin();
        cache[old_key].type = TYPE_B1;
        // 注意：在实际工业实现中，此时可以释放 cache[old_key].value 的内存
    } else {
        // 情况 2: 从 T2 淘汰数据到 B2
        if (!T2.empty()) {
            int old_key = T2.back();
            T2.pop_back();
            
            B2.push_front(old_key);
            cache[old_key].it = B2.begin();
            cache[old_key].type = TYPE_B2;
        }
    }
}

std::optional<int> ARCCache::get(int key) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = cache.find(key);
    if (it == cache.end()) return std::nullopt;

    Entry& entry = it->second;

    // 检查过期 (TTL)
    if (std::chrono::steady_clock::now() > entry.expire_at) {
        remove_from_list(entry, key);
        cache.erase(it);
        return std::nullopt;
    }

    // 如果命中幽灵节点 (B1/B2)，说明该 Key 之前被删除了，ARC get 通常只针对 T1/T2
    if (entry.type == TYPE_B1 || entry.type == TYPE_B2) {
        return std::nullopt;
    }

    // 命中 T1 或 T2：数据升级到 T2 头部
    remove_from_list(entry, key);
    T2.push_front(key);
    entry.it = T2.begin();
    entry.type = TYPE_T2;

    return entry.value;
}

void ARCCache::put(int key, int val, int ttl_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    auto expire = std::chrono::steady_clock::now() + std::chrono::milliseconds(ttl_gen.generate(ttl_ms));

    if (cache.count(key)) {
        Entry& entry = cache[key];
        remove_from_list(entry, key);
        
        if (entry.type == TYPE_T1 || entry.type == TYPE_T2) {
            // 已在内存，更新值并移到 T2
            entry.value = val;
            entry.expire_at = expire;
        } else if (entry.type == TYPE_B1) {
            // 在 B1 命中：说明 T1 太小了，增加 p
            p = std::min(c, p + (int)std::max(B1.size() / std::max(B2.size(), (size_t)1), (size_t)1));
            replace(key);
            entry.value = val;
            entry.expire_at = expire;
        } else if (entry.type == TYPE_B2) {
            // 在 B2 命中：说明 T2 太大了（T1 需要更多空间），减小 p
            p = std::max(0, p - (int)std::max(B2.size() / std::max(B1.size(), (size_t)1), (size_t)1));
            replace(key);
            entry.value = val;
            entry.expire_at = expire;
        }
        
        T2.push_front(key);
        entry.it = T2.begin();
        entry.type = TYPE_T2;
    } else {
        // 全新 Key
        // 1. 检查总容量约束 (T1 + B1)
        if (T1.size() + B1.size() == (size_t)c) {
            if (T1.size() < (size_t)c) {
                delete_ghost_tail(B1);
                replace(key);
            } else {
                int old_key = T1.back();
                T1.pop_back();
                cache.erase(old_key);
            }
        } else if (T1.size() + B1.size() + T2.size() + B2.size() >= (size_t)c) {
            // 2. 检查 L2 级的总容量 (2c)
            if (T1.size() + B1.size() + T2.size() + B2.size() == (size_t)2 * c) {
                delete_ghost_tail(B2);
            }
            replace(key);
        }

        // 插入 T1
        T1.push_front(key);
        cache[key] = {val, T1.begin(), TYPE_T1, expire};
    }
}