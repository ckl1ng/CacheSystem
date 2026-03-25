cmake_minimum_required(VERSION 3.10)
project(CacheSystem VERSION 1.0 LANGUAGES CXX)

# 1. 设置 C++ 标准为 17 (支持 std::optional, std::variant 等)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 编译选项优化：开启 -O3 极致优化（对缓存压测非常关键）
# 并开启常用的警告，帮助排查代码隐患
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

# 3. 寻找多线程库 (pthread)
find_package(Threads REQUIRED)

# 4. 指定头文件包含目录
# 这样你在 .cpp 中写 #include "SafeCache.hpp" 就不需要带相对路径了
include_directories(${PROJECT_SOURCE_DIR}/include)

# 5. 搜集核心库的源文件（排除含有 main 函数的文件）
set(SOURCE_FILES
    src/ARCCache.cpp
    src/LFUCache.cpp
    src/LRUCache.cpp
    src/SafeCache.cpp
    src/BloomFilter.cpp
)

# 6. 生成核心静态库 cache_lib
add_library(cache_lib STATIC ${SOURCE_FILES})

# 7. 生成可执行文件 (链接你的测试入口 main.cpp)
add_executable(CacheBenchmark src/main.cpp)

# 8. 链接静态库和多线程库
target_link_libraries(CacheBenchmark PRIVATE cache_lib Threads::Threads)

# (可选) 如果你以后在 tests/ 下有别的测试文件，可以继续添加：
# add_executable(PerformanceTest tests/benchmark_test.cpp)
# target_link_libraries(PerformanceTest PRIVATE cache_lib Threads::Threads)