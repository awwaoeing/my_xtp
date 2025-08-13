#pragma once // 防止头文件被重复包含
#include "ThreadSafeQueue.h"
#include "xtp_api_struct.h"
#include "self_define_DataStruct.h"
#include "self_define_function.h"
#include <thread>
#include <string>
#include <vector>
#include <memory>      // 必须包含 <memory> 来使用 std::unique_ptr
#include <atomic>      // 必须包含 <atomic> 来使用 std::atomic
#include "LatencyTracer.h"

class StrategyEngine {
public:
    // 构造函数，接收行情队列和订单队列的引用
    StrategyEngine(
        ThreadSafeQueue<unique_ptr<MarketDataPacket>>* g_market_signal_queue, // 行情数据队列
        ThreadSafeQueue<unique_ptr<OrderRequest>>* g_order_request_queue // 订单请求队列的引用
    );

    // 析构函数，负责清理线程
    ~StrategyEngine();

    // 启动策略引擎的主循环
    void start();

    // 停止策略引擎
    void stop();

private:
    // 策略逻辑的主循环，将作为线程的入口点
    void run(); // 从 g_market_signal_queue 中接收数据,并调用策略函数,策略函数将订单推入g_order_request_queue; 

    // --- 在这里定义你的策略规则 ---
    void apply_strategy_simple_threshold(const MarketDataPacket& md);

    // ... 可以添加更多策略函数

    // 成员变量
    ThreadSafeQueue<unique_ptr<MarketDataPacket>>*  g_market_signal_queue; // 行情数据队列的引用
    ThreadSafeQueue<unique_ptr<OrderRequest>>* g_order_request_queue; // 订单请求队列的引用
    
    std::thread m_strategy_thread; // 策略逻辑线程,执行run
    std::atomic<bool> m_is_running; // 控制线程运行的标志
};
