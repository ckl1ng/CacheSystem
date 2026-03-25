#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <vector>
#include <string>
#include <iostream>

/**
 * @brief 布隆过滤器 (Bloom Filter)
 * 
 * 一种空间效率极高的概率型数据结构，用于判断一个元素是否在一个集合中。
 * 在缓存系统中主要用于解决“缓存穿透”问题：在访问数据库前先查询布隆过滤器，
 * 如果过滤器判定不存在，则直接返回，不再查询数据库。
 * 
 * 特点：
 * 1. 存在误报率 (False Positive)：如果返回 true，元素可能在集合中，也可能不在。
 * 2. 零漏报率 (False Negative)：如果返回 false，元素一定不在集合中。
 * 3. 不支持删除操作（标准布隆过滤器）。
 */
class BloomFilter {
private:
    std::vector<bool> bit_array;  // 位阵列，使用 std::vector<bool> 的空间优化实现
    size_t bit_size;              // 位阵列的总位数 (m)
    
    /**
     * @brief 哈希种子列表
     * 使用 5 个不同的种子来模拟 5 个独立的哈希函数 (k=5)。
     * 种子通常选择质数以减少哈希碰撞。
     */
    std::vector<int> seeds = {31, 131, 13131, 131313, 1313131};

    /**
     * @brief 字符串哈希函数 (基于多项式滚动哈希)
     * @param key 需要哈希的字符串
     * @param seed 传入的哈希种子
     * @return 映射到 bit_array 范围内的索引值
     */
    size_t hash(const std::string& key, int seed);

public: 
    /**
     * @brief 构造函数
     * @param size 位阵列的大小，默认 1,000,000 位。
     * 大小设置取决于预期存储的数据量和容忍的误报率。
     */
    BloomFilter(size_t size = 1000000);

    /**
     * @brief 向布隆过滤器中添加元素
     * 计算 5 个哈希值，并将位阵列中对应的 5 个索引位置设为 true。
     * @param key 存入的 Key（如数据库中已存在的 ID）
     */
    void add(const std::string& key);

    /**
     * @brief 检查元素是否可能存在
     * 只有当 5 个哈希对应的位全部为 true 时才返回 true。
     * @param key 待查询的 Key
     * @return true 元素可能存在；false 元素一定不存在（可直接拦截请求）
     */
    bool contains(const std::string& key);
};

#endif