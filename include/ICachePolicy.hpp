#ifndef ICACHE_POLICY_H
#define ICACHE_POLICY_H

#include <optional>

/**
 * @brief 缓存替换策略抽象基类 (Interface for Cache Eviction Policies)
 * 
 * 该接口定义了所有缓存算法（如 LRU, LFU, ARC 等）必须遵循的标准操作。
 * 通过多态设计，使得 SafeCache 可以在运行时或编译时切换不同的算法实现，
 * 而无需修改上层业务逻辑。
 */
class ICachePolicy {
public:
    /**
     * @brief 虚析构函数
     * 
     * 确保通过基类指针删除派生类对象时，能够正确调用派生类的析构函数，防止内存泄漏。
     */
    virtual ~ICachePolicy() = default;

    /**
     * @brief 从缓存中检索数据
     * 
     * 根据 key 查找对应的 value。如果找到且数据未过期，则返回该值；
     * 如果 key 不存在或者数据已过期，则返回 std::nullopt。
     * 
     * @param key 要查询的唯一键值
     * @return std::optional<int> 包含查询结果的容器。若命中则有值，若未命中则为空。
     */
    virtual std::optional<int> get(int key) = 0;

    /**
     * @brief 向缓存中插入或更新数据
     * 
     * 将指定的 key-value 对存入缓存。如果 key 已存在，则更新其值并重置过期时间。
     * 如果缓存已满，该方法内部会根据具体的替换算法（如 LRU 的最近最少使用）
     * 自动淘汰旧的数据。
     * 
     * @param key 数据的键
     * @param value 数据的值
     * @param ttl_ms 数据的生存时间 (Time To Live)，单位为毫秒。
     *               默认为 60000ms (1分钟)。
     */
    virtual void put(int key, int value, int ttl_ms = 60000) = 0;
};

#endif