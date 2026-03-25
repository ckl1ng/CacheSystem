# SafeCache: 高性能多策略分片缓存系统

SafeCache 是一个基于 C++17 开发的、面向高并发场景的线程安全缓存库。它不仅支持多种经典的缓存淘汰算法（LRU, LFU, ARC），还通过分片锁（Sharding）技术解决了全局锁竞争瓶颈，并集成了布隆过滤器以抵御缓存穿透攻击。

##  核心特性

*   多策略支持: 
    *   LRU (Least Recently Used): 经典的最近最少使用算法。
    *   LFU (Least Frequently Used): 频率优先算法。
    *   ARC (Adaptive Replacement Cache): 自适应替换缓存，动态平衡 LRU 和 LFU，适应多变的访问模式。
*   高并发优化: 采用分片锁 (Lock Striping) 技术，根据 Key 的哈希值将数据分散到多个独立的分片中，极大降低了多线程环境下的锁竞争。
*   缓存防护体系:
    *   穿透防护: 内置布隆过滤器（Bloom Filter），在查询数据库前拦截非法 Key。
    *   击穿防护: 结合互斥锁逻辑，确保同一时间只有一个线程加载数据库，其他线程原地等待结果。
    *   雪崩防护: 引入 ExpirationGenerator，为 TTL 增加随机抖动（Jitter），防止缓存大规模同时失效。
*   工业级注释: 采用 Doxygen 标准注释 (@brief, @param, @return)，支持 IDE 智能感知。

##  架构设计

[ 客户端请求 ]
       |
       v
[ 布隆过滤器 ] ---- (不存在则直接拦截) ----> [ 返回空 ]
       |
       v
[ SafeCache 分片层 ]
       |-- 分片 0 (独立锁 + 缓存策略)
       |-- 分片 1 (独立锁 + 缓存策略)
       |-- ...
       |-- 分片 N (独立锁 + 缓存策略)
               |
               v
     [ 执行 LRU / LFU / ARC 淘汰逻辑 ]

##  环境要求

*   操作系统: Linux (Ubuntu/WSL 推荐) 或 macOS
*   编译器: GCC 7+ 或 Clang 5+ (需支持 C++17)
*   依赖: Pthread 线程库

##  编译与运行

建议开启 -O3 优化以获得最佳性能测试结果：

# 1. 性能测试模式 (推荐)
g++ main.cpp -o safe_cache_test -std=c++17 -O3 -lpthread

# 2. 开发调试模式 (开启内存检查)
g++ main.cpp -o safe_cache_debug -std=c++17 -g -fsanitize=address -lpthread

# 运行程序
./safe_cache_test

##  使用示例

```cpp
#include "SafeCache.h"

int main() {
    // 初始化：16分片，总容量1000，使用 LRU 算法
    SafeCache cache(16, 1000, CacheAlgo::LRU);

    // 1. 预热布隆过滤器（将合法 Key 加入白名单）
    cache.addToBloomFilter(123);

    // 2. 高并发获取数据（自动处理 DB 加载逻辑，防止击穿）
    auto db_loader = [](int key) {
        return key * 10; // 模拟耗时的数据库操作
    };

    int value = cache.get(123, db_loader, 60000); // 60s TTL
    
    // 3. 拦截恶意请求（不在布隆过滤器中的 Key 不会触发 db_loader，防止穿透）
    int ghost_val = cache.get(-999, db_loader); 
}