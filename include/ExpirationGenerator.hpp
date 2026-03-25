#ifndef EXPIRATIONGENERATOR_H
#define EXPIRATIONGENERATOR_H

#include <random>

/**
 * @brief 过期时间生成器 (Expiration Generator)
 * 
 * 专门用于生成带有“随机抖动”的缓存过期时间（TTL）。
 * 
 * 设计目的：
 * 解决【缓存雪崩】问题。如果大量缓存 Key 设置相同的过期时间，
 * 它们可能会在同一时刻集体失效，导致瞬间流量全部涌向数据库。
 * 通过在基础过期时间上增加一个随机的微小偏移量（Jitter），
 * 使各 Key 的失效时间点均匀错开，从而平滑数据库的访问压力。
 */
class ExpirationGenerator {
private:
    std::mt19937 rng; // 梅森旋转算法随机数引擎，提供高质量的伪随机数

public:
    /**
     * @brief 构造函数
     * 初始化随机数引擎，并使用硬件随机数设备（std::random_device）作为种子。
     */
    ExpirationGenerator() {
        std::random_device rd;
        rng.seed(rd());
    }

    /**
     * @brief 获取带抖动的过期时间
     * 
     * 计算公式：Final_TTL = base_ttl_ms + random(0, max_jitter_ms)
     * 
     * @param base_ttl_ms 基础过期时间（毫秒），例如 60,000ms (1分钟)
     * @param max_jitter_ms 最大随机抖动范围（毫秒）。
     *                      建议设置为基础时间的 5%~10%。例如 5000ms。
     * @return int 最终生成的、带随机偏移的过期时间（毫秒）
     */
    int getJitteredTTL(int base_ttl_ms, int max_jitter_ms) {
        // 创建均匀分布：[0, max_jitter_ms] 之间的随机偏移
        std::uniform_int_distribution<int> dist(0, max_jitter_ms);
        int jitter = dist(rng);
        
        return base_ttl_ms + jitter;
    }

    /**
     * @brief 生成随机偏移量的重载方法 (可选建议)
     * 
     * 有时为了方便，可以直接传入一个基础时间，由内部计算 10% 的抖动。
     */
    int generate(int base_ttl_ms) {
        // 默认给予 10% 的随机抖动范围
        int max_jitter = base_ttl_ms / 10;
        if (max_jitter <= 0) max_jitter = 1;
        return getJitteredTTL(base_ttl_ms, max_jitter);
    }
};

#endif