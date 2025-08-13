#pragma once
#include <string>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>                 // <--- 必需：提供了 init_thread_pool 和 async_factory
#include <spdlog/sinks/basic_file_sink.h> // <--- 必需：提供了 basic_logger_mt (文件日志接收器)
#include <atomic>



// --- 全局的、线程安全的追踪ID生成器 ---
// g_trace_id_generator 为每一个数据包的声明周期都生成唯一id
// 在C++17及以后的标准中，inline 关键字可以用于全局变量。它的作用是向链接器表明，如果多个编译单元（.cpp文件）都因为包含了这个头文件而定义了 g_correlation_id_generator，链接器应该将它们合并为唯一的实例，而不是报错说“变量被重复定义了”。这是现代C++中在头文件中定义全局变量的标准做法。
inline std::atomic<uint64_t> g_trace_id_generator(0);



//===================================================================
// 初始化日志系统
// 在 main 函数的最开始调用
//===================================================================
inline void InitLatencyLogging() {
    try {

		// --- 清空旧的 latency_log.csv 文件 ---
        std::ofstream ofs("latency_log.csv", std::ofstream::trunc);  // trunc 表示清空文件
        ofs.close();  // 立即关闭文件


        // 设置一个拥有8192个槽位的队列和1个后台工作线程的线程池
        spdlog::init_thread_pool(8192, 1);

        // 创建一个名为 "latency_logger" 的异步文件日志记录器
        auto latency_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
            "latency_logger", 
            "latency_log.csv"
        );

        // 设置日志格式为只记录原始消息
        latency_logger->set_pattern("%v");

		// 设置定时自动 flush，每 1 秒刷新一次
    	spdlog::flush_every(std::chrono::seconds(1));
        
        // 写入CSV表头
        latency_logger->info("Timestamp_ns,EventTag,Ticker,TraceID");

    } catch (const spdlog::spdlog_ex& ex) {
        // 在实际项目中，处理初始化失败非常重要
        fprintf(stderr, "Log initialization failed: %s\n", ex.what());
        exit(-1);
    }
}


//===================================================================
// 获取高精度时间戳的函数
// --- 定义一个高性能的打点函数 ---
// 获取当前高精度时间戳的字符串形式（纳秒）
//===================================================================
inline std::string get_timestamp_ns_str() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::to_string(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()
    );
}

inline int64_t get_timestamp_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}


//===================================================================
// 全局打点函数
// 这是你将在所有业务逻辑中调用的唯一函数
// 使用 const char* 而不是 std::string 作为参数可以避免一些不必要的内存分配
// 供所有业务线程调用的打点函数,每次需要打点时调用该函数
// 使用 spdlog 的格式化字符串，比手动拼接更高效、更安全                                                                                
//===================================================================
inline void record_latency_log(const char* event_tag, const char* ticker,uint64_t trace_id) {
     // 获取名为 "latency_logger" 的记录器，并记录日志
     // 格式: "Timestamp,EventTag,Ticker"
    try {
        auto logger = spdlog::get("latency_logger");
        if (logger) {
            // 格式: "Timestamp,EventTag,CorrelationID"
            logger->info("{},{},{},{}", get_timestamp_ns(), event_tag, ticker,trace_id);
			
        }
    } catch (const spdlog::spdlog_ex& ex) {
        // 在极少数情况下，如果logger还未初始化或已关闭，get()可能会抛出异常
        // 在高性能代码中，我们通常不希望有异常处理，但这里为了健壮性可以加上
    }
}


//===================================================================
// 关闭日志系统
// 在 main 函数完全退出前调用
//===================================================================
inline void ShutdownLatencyLogging() {
	// 最后再手动 flush 一次，确保剩余内容写入磁盘
    auto logger = spdlog::get("latency_logger");
    if (logger) {
        logger->flush();
    }
    spdlog::shutdown();
}
