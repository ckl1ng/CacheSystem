#include "LRUCache.h"

std::optional<int> LRUCache::get(int key) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = fd_.find(key);
    if (it == fd_.end()) return std::nullopt;
    auto now = std::chrono::steady_clock::now();

    if (now > it->second->expire_at) {
        lst_.erase(it->second);
        fd_.erase(key);
        return std::nullopt;
    }

    lst_.splice(lst_.begin(), lst_, it->second);
    return it->second->val;
}

void LRUCache::put(int key, int value, int ttl_ms) {
    std::lock_guard<std::mutex> lock(mtx);

    int final_ttl = ttl_gen.getJitteredTTL(ttl_ms, 5000);
    auto now = std::chrono::steady_clock::now();
    TimePoint expire_at = now + std::chrono::milliseconds(final_ttl);

    if (fd_.count(key)){
        auto it = fd_[key];
        it->val = value;
        it->expire_at = expire_at;
        lst_.splice(lst_.begin(), lst_, it);
    }
    else {
        if (fd_.size() == capacity_) {
            auto it = lst_.rbegin();
            fd_.erase(it->key);
            lst_.pop_back();
        }

        lst_.push_front({key, value, expire_at});
        fd_[key] = lst_.begin();
    }
}