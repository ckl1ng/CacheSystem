#include "SafeCache.h"

SafeCache::SafeCache(size_t num, size_t capacity, CacheAlgo algo): shard_num(num) {
    shards.reserve(num);
    for (size_t i = 0; i < num; i++) {
        if (algo == CacheAlgo::LRU) {
            shards.emplace_back(std::make_unique<LRUCache>(capacity));
        }
        else if (algo == CacheAlgo::LFU) {
            shards.emplace_back(std::make_unique<LFUCache>(capacity));
        }
        else if(algo == CacheAlgo::ARC) {
            shards.emplace_back(std::make_unique<ARCCache>(capacity));
        }
    }
};

void SafeCache::addToBloomFilter(int key) {
    blf.add(std::to_string(key));
}

std::optional<int> SafeCache::get(int key) {
    size_t hash_val = std::hash<int>{}(key);
    size_t id = hash_val % shard_num;
    return shards[id]->get(key);
}

int SafeCache::get(int key, std::function<int(int)> db_loader, int ttl_ms) {
    std::optional val = get(key);
    if (val.has_value()) {
        return val.value();
    }

    if (!blf.contains(std::to_string(key))) {
        // std::cout << "已被过滤" << std::endl;
        return -1;
    }

    size_t lock_idx = std::hash<int>{}(key) % SF_LOCK_POOL_SIZE;
    std::lock_guard<std::mutex> sf_lock(sf_locks[lock_idx]);

    val = get(key);
    if (val.has_value()) {
        // std::cout << "已有线程命中" << std::endl;
        return val.value();
    }

    addToBloomFilter(key);
    // std::cout << "缓存未命中,当前线程去查DB" << std::endl;
    int db_val = db_loader(key);

    put(key, db_val, ttl_ms);
    return db_val;
}

void SafeCache::put(int key, int value, int ttl_ms) {
    size_t hash_val = std::hash<int>{}(key);
    size_t id = hash_val % shard_num;
    shards[id]->put(key, value, ttl_ms);
}