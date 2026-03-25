#include "LFUCache.h"

std::optional<int> LFUCache::get(int key) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = key_table_.find(key);
    if (it == key_table_.end()) return std::nullopt;
    auto now = std::chrono::steady_clock::now();
    int cnt = it->second->freq;

    if (now > it->second->expire_at) {
        freq_table_[cnt].erase(it->second);
        key_table_.erase(key);
        return std::nullopt;
    }

    freq_table_[cnt+1].splice(freq_table_[cnt+1].begin(), freq_table_[cnt], it->second);
    if (min_freq_ == cnt && freq_table_[cnt].size() == 0)min_freq_++;
    it->second->freq++;
    return it->second->val;
}


void LFUCache::put(int key, int value, int ttl_ms) {
    std::lock_guard<std::mutex> lock(mtx);

    int final_ttl = ttl_gen.getJitteredTTL(ttl_ms, 5000);
    auto now = std::chrono::steady_clock::now();
    TimePoint expire_at = now + std::chrono::milliseconds(final_ttl);

    auto it = key_table_.find(key);

    if (it != key_table_.end()){
        int cnt = it->second->freq;
        freq_table_[cnt+1].splice(freq_table_[cnt+1].begin(), freq_table_[cnt], it->second);
        if (min_freq_ == cnt && freq_table_[cnt].size() == 0)min_freq_++;
        it->second->freq++;
        it->second->val = value;
        it->second->expire_at = expire_at;
    }
    else {
        if (key_table_.size() == capacity_) {
            key_table_.erase(freq_table_[min_freq_].rbegin()->key);
            freq_table_[min_freq_].pop_back();
        }

        freq_table_[1].push_front({key, value, 1, expire_at});
        key_table_[key] = freq_table_[1].begin();
        min_freq_ = 1;
    }
}