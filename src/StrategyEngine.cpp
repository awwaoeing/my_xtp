#include "StrategyEngine.h"


//===================================================================
// 构造函数
// 接收指向全局队列的指针，并初始化成员变量
//===================================================================
StrategyEngine::StrategyEngine(
    ThreadSafeQueue<std::unique_ptr<MarketDataPacket>>* market_data_queue,
    ThreadSafeQueue<std::unique_ptr<OrderRequest>>* order_queue)
	: 
    	g_market_signal_queue(market_data_queue),
    	g_order_request_queue(order_queue),
    	m_is_running(false) 
{
    // 检查传入的指针是否有效，这是一个好习惯
    if (g_market_signal_queue == nullptr || g_order_request_queue == nullptr) {
        // 在实际项目中，这里应该抛出异常或记录严重错误
        std::cerr << "FATAL ERROR: StrategyEngine received null queue pointers!" << std::endl;
    }
    std::cout << "StrategyEngine created." << std::endl;
}

//===================================================================
// 析构函数
// 确保在对象销毁时，其内部线程被正确停止和清理
//===================================================================
StrategyEngine::~StrategyEngine() {
    stop(); // 调用 stop 来确保线程被 join
    std::cout << "StrategyEngine destroyed." << std::endl;
}

//===================================================================
// start()
// 启动策略引擎，创建并运行其内部的策略逻辑线程
//===================================================================
void StrategyEngine::start() {
    if (m_is_running.load() == false) {
        m_is_running.store(true);
        // 创建线程，将 run() 方法作为入口点。
        // this 指针作为第一个参数，因为 run 是一个成员函数。
        m_strategy_thread = std::thread(&StrategyEngine::run, this);
        std::cout << "StrategyEngine started." << std::endl;
    }
}

//===================================================================
// stop()
// 停止策略引擎，等待内部线程执行完毕并 join
//===================================================================
void StrategyEngine::stop() {
    if (m_is_running.load() == true) {
        m_is_running.store(false);
        // 注意：这个函数本身并不关闭队列。
        // 队列的关闭是由外部的 main 函数在协调所有组件关闭时完成的。
        // 当队列被外部关闭后，run() 方法中的 pop() 会返回 false，线程会自然退出。
        
        if (m_strategy_thread.joinable()) {
            m_strategy_thread.join(); // 等待线程执行完毕
        }
        std::cout << "StrategyEngine stopped." << std::endl;
    }
}

//===================================================================
// run()
// 策略线程的主循环。它不断地从行情队列中取出数据，
// 然后分发给不同的策略规则函数进行处理。
//===================================================================
void StrategyEngine::run() {
    std::cout << "Strategy thread [run()] started. Processing market data..." << std::endl;
    
    // 这个 unique_ptr 将被重复用于接收从队列中弹出的数据
    std::unique_ptr<MarketDataPacket> md_ptr;

    // 循环会一直运行，直到 m_market_data_queue_ptr 被外部关闭，此时 pop() 会返回 false。
    while (g_market_signal_queue->pop(md_ptr)) {
        
        // 检查指针是否有效
        if (md_ptr) { 

			// T3: 策略线程从队列中取出
            record_latency_log("T3_market_signal_queue_pop", md_ptr->market_data_copy.ticker,md_ptr->trace_id);

            // 将解引用后的对象传递给策略函数, 策略函数接收一个 const MarketDataPacket&，这避免了再次拷贝。
            apply_strategy_simple_threshold(*md_ptr);
            // apply_strategy_ma_crossover(*md.market_data_copy.ptr); // 如果有其他策略，也可以在这里调用
        }
        // 当循环进入下一次迭代时，md.market_data_copy.ptr 会被 pop() 的结果重新赋值，
        // 它之前指向的 MarketDataPacket 对象会被自动销毁，内存被回收。
    }

    std::cout << "Strategy thread [run()] has finished processing." << std::endl;
}

//===================================================================
// apply_strategy_simple_threshold()
// 一个具体的策略规则实现。
// 如果满足条件，它会创建一个订单请求并将其推入订单队列。
//===================================================================
void StrategyEngine::apply_strategy_simple_threshold(const MarketDataPacket& md)
{
    std::string ticker_str = md.market_data_copy.ticker;

    // 示例策略：当平安银行价格低于9.5时买入
    if ((ticker_str == "000001" || ticker_str == "600000")  && md.market_data_copy.last_price > 0) 
	{
        std::cout << "Strategy[Threshold]: BUY signal for " << ticker_str << " at " << md.market_data_copy.last_price << std::endl;

		auto order_ptr = std::make_unique<OrderRequest>(
		    md.market_data_copy.ticker,     // ticker from行情数据
		    XTP_MKT_SZ_A,                   // market
		    XTP_SIDE_BUY,                   // side
		    XTP_PRICE_BEST_OR_CANCEL,       // price_type
		    0.0,                             // price
		    1,                             // quantity
		    XTP_BUSINESS_TYPE_CASH,         // business_type
		    XTP_POSITION_EFFECT_INIT,        // position_effect
			md.trace_id
		);

		// T4: 策略计算完毕，决定下单
        record_latency_log("T4_order_created", md.market_data_copy.ticker,md.trace_id);

        g_order_request_queue->push(std::move(order_ptr));
    }
}
