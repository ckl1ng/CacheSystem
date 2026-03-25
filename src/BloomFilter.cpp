#include "BloomFilter.h"

size_t BloomFilter::hash(const std::string& key, int seed) {
    size_t hash_val = 0;
    for (char c : key) {
        hash_val = hash_val * seed + c;
    }

    return hash_val % bit_size;
}

BloomFilter::BloomFilter(size_t size) : bit_size(size) {
    bit_array.resize(bit_size, false);
}

void BloomFilter::add(const std::string& key) {
    for (int seed : seeds) {
        size_t idx = hash(key, seed);
        bit_array[idx] = true;
    }
}

bool BloomFilter::contains(const std::string& key) {
    for (int seed : seeds) {
        size_t idx = hash(key, seed);
        if (!bit_array[idx]) {
            return false;
        }
    }
    return true;
}