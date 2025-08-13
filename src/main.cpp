#include "xtp_quote_api.h"
#include "xtp_trader_api.h"
#include <string>
#include <iostream>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include "quote_spi.h"
#include "trade_spi.h"
#include "self_define_DataStruct.h"
#include "self_define_function.h"
#include "StrategyEngine.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "LatencyTracer.h"



//{{{ pair<int, int> get_current_utc_hour_minute()  获取当前的utc时间 , 返回当前utc时间的小时和分钟
std::pair<int, int> get_current_utc_hour_minute() {
    auto now = std::chrono::system_clock::now(); //获取一个代表“现在”这个瞬间的、与时区无关的、基于UTC的纳秒级时间点。
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	//std::chrono::system_clock::to_time_t(now): 这是一个转换函数，它接收高精度的 time_point 对象 now，并将其转换为 std::time_t 类型。
	//std::time_t: 这是C语言时间库 (<ctime>) 的核心类型，通常是一个整数，表示从纪元（1970年1月1日 00:00:00 UTC）到现在的总秒数。


    // 使用 gmtime 将 time_t 转换为 UTC 时间的 tm 结构体
    std::tm* utc_tm = std::gmtime(&now_time);
	//作用: 这是最关键的一步。它将总秒数分解成一个包含年月日时分秒的日历结构，且这个结构是基于UTC时间的。
	//std::gmtime: 这是C语言时间库中的一个函数。"gm" 代表 "Greenwich Mean" (格林威治标准时间)，也就是UTC时间。它接收一个指向 time_t（总秒数）的指针。
	//与 localtime 的区别:
	//localtime: 将总秒数转换成本地时区的时间。
	//gmtime: 将总秒数转换成 UTC (零时区) 的时间。
	//std::tm* utc_tm: gmtime 的返回值是一个指向 struct tm 结构体的指针。这个结构体包含了被分解好的、基于UTC的各个时间部分。变量 utc_tm 现在就指向了这个结构体。
	//简单来说: 这一行将“从1970年开始的总秒数”直接翻译成了UTC时区下的年月日时分秒。

    if (utc_tm == nullptr) {
        // 在极少数情况下 gmtime 可能会失败
        return {-1, -1};
    }
	cout << "utc_tm->tm_hour" << " " << utc_tm->tm_hour << " " <<  "utc_tm->tm_min" << " " << utc_tm->tm_min << endl;
    return {utc_tm->tm_hour, utc_tm->tm_min};
}
//}}}


//{{{ 绑核
int bind_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pid_t pid = getpid();  // 绑定当前进程
    int ret = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        perror("sched_setaffinity");
        return -1;
    }
    return 0;
}
//}}}

//{{{ 行情配置
XTP::API::QuoteApi* m_pQuoteApi = NULL;//全局变量，程序共用一个api
string quote_server_ip = "119.3.103.38";//行情服务器ip地址
uint64_t session_id_ = 0;//用户登陆后对应的session_id
int quote_server_port = 6002;//行情服务器端口port
string quote_username = "453191001592";//行情服务器的登陆账户名
string quote_password = "VS8JYAUa";//行情服务器的登陆密码
XTP_PROTOCOL_TYPE protocol_type = XTP_PROTOCOL_TCP;//Level1服务器通常使用TCP，Level2服务器请用UDP，公网测试环境均为TCP，具体以运营通知为准
string local_ip = "10.0.0.128";//本地网卡对应的ip
//}}}

//{{{ 交易配置
XTP::API::TraderApi* m_pTraderApi = NULL;//全局变量，程序共用一个api
string trader_server_ip = "122.112.139.0";//交易服务器ip地址，请自行修改
int trader_server_port = 6104;//交易服务器端口port，请自行修改
string trader_username = "453191001592";//交易服务器的登陆账户名，请自行修改
string trader_password = "VS8JYAUa";//交易服务器的登陆密码，请自行修改
//}}}

using namespace std;

int main()
{
	bind_cpu(19);  // 绑定CPU0

	uint8_t client_id = 1;//一个进程一个client id，可在[1, 99]区间内任选，并固定下来
	string save_file_path = "./";//保存xtp api日志的路径，需要有可读写权限
//	XTP_LOG_LEVEL log_level = XTP_LOG_LEVEL_DEBUG;//xtp api日志的输出级别，建议调试时使用debug级别，正常运行时使用info级别
	XTP_LOG_LEVEL log_level = XTP_LOG_LEVEL_INFO; // 将日志级别从 DEBUG 改为 INFO



	//{{{ spdlog 日志打点模块,用LatencyTracker封装
	
	// 1. 在程序最开始初始化日志系统
    InitLatencyLogging();

	//}}}
	


	//{{{ 行情模块
	///创建QuoteApi
	m_pQuoteApi = XTP::API::QuoteApi::CreateQuoteApi(client_id, save_file_path.c_str(), log_level);
	if (NULL == m_pQuoteApi)
	{
		//创建API失败
		return 0;
	}

	///设定行情服务器超时时间，单位为秒，默认是15s，调试时可以设定大点
	uint32_t heat_beat_interval_Quote = 15;
	m_pQuoteApi->SetHeartBeatInterval(heat_beat_interval_Quote);

	//以下代码段如果是TCP连接的话,需要跳过
	//设定UDP本地缓存buffer大小，单位为MB
	//int buffer_size = 512;//2.2.30.7以上版本api，建议不超过512，最大仅支持1024
	//m_pQuoteApi->SetUDPBufferSize(buffer_size);
	//设定是否输出异步日志
	//bool  log_output_flag = true;//刚实盘运行时，或者调试测试时，建议开启，实盘运行正常后，可以关闭
	//m_pQuoteApi->SetUDPSeqLogOutPutFlag(log_output_flag);


	//创建Spi实例
	//// 1. 我们的核心通信管道：线程安全的行情信号队列
	ThreadSafeQueue<unique_ptr<MarketDataPacket>> g_market_signal_queue;
	MyQuoteSpi* m_pQuoteSpi = new MyQuoteSpi(&g_market_signal_queue);
	if (NULL == m_pQuoteSpi)
	{
		//创建行情Spi失败
		return 0;
	}

	//注册Spi
	m_pQuoteApi->RegisterSpi(m_pQuoteSpi);

	//登陆行情服务器
	int ret_quote = m_pQuoteApi->Login(quote_server_ip.c_str(), quote_server_port, quote_username.c_str(), quote_password.c_str(), protocol_type, local_ip.c_str());
	if (0 != ret_quote)
	{
		// 登录失败，获取错误信息
		XTPRI* error_info = m_pQuoteApi->GetApiLastError();
		cout << "login to server error, " << error_info->error_id << " : " << error_info->error_msg << endl;

		return 0;
	}

	// 登录成功
	//TODO: 用户逻辑，例如查询静态数据、订阅行情等，以下以查询沪市静态信息为例



	// 获取当前交易日
	if (m_pQuoteApi)
	{
		cout << "GetTradingDay : " << m_pQuoteApi->GetTradingDay() << endl;
	}


	//查询沪市行情静态信息,查询完沪市信息后接着查询深市信息，将沪深两市要查询的股票的静态合约信息存入map  code->XTPQFI
	m_pQuoteApi->QueryAllTickersFullInfo(XTP_EXCHANGE_SH);
	//}}}


	//{{{ 交易模块
	//初始化交易api
	m_pTraderApi = XTP::API::TraderApi::CreateTraderApi(client_id, save_file_path.c_str(), log_level);
	if (NULL == m_pTraderApi)
	{
		//创建API失败
		cout << "Failed to create trader api, please check the input parameters." << endl;
		return 0;
	}

	//{{{ 配置初始化
	//
	uint32_t heat_beat_interval_Trade = 15;
	///设定与交易服务器交互的超时时间，单位为秒，默认是15s，调试时可以设定大点
	m_pTraderApi->SetHeartBeatInterval(heat_beat_interval_Trade);

	//设定公共流传输方式
	XTP_TE_RESUME_TYPE resume_type = XTP_TERT_QUICK;//第一次登陆所使用的公共流消息传送方式，用户可视自身需要在quick和restart中任选
	m_pTraderApi->SubscribePublicTopic(resume_type);

	//设定用户的开发代码，在XTP申请开户时，由xtp运营人员提供
	char software_key[] = "b8aa7173bba3470e390d787219b2112e";
	m_pTraderApi->SetSoftwareKey(software_key);

	//设定软件版本号，用户自定义（仅可使用如下字符0-9，a-z，A-Z，.）
	char version[] = "MyAlgoTrader.1.0.2";
	m_pTraderApi->SetSoftwareVersion(version);


	//创建Spi类实例
	ThreadSafeQueue<unique_ptr<OrderRequest>> g_order_request_queue;
	MyTraderSpi* m_pTraderSpi = new MyTraderSpi(&g_order_request_queue);
	if (NULL == m_pTraderSpi)
	{
		cout << "Failed to create quote spi, please check the input parameters." << endl;
		return 0;
	}
	//注册Spi
	m_pTraderApi->RegisterSpi(m_pTraderSpi);

	//登录交易服务器
	session_id_ = m_pTraderApi->Login(trader_server_ip.c_str(), trader_server_port, trader_username.c_str(), trader_password.c_str(), XTP_PROTOCOL_TCP, local_ip.c_str());
	if (0 == session_id_)
	{
		//登录失败，获取失败原因
		XTPRI* error_info = m_pTraderApi->GetApiLastError();
		cout << "Login to server error, " << error_info->error_id << " : " << error_info->error_msg << endl;

		return 0;
	}

	// 登录成功
	cout << "Login to server success." << endl;
	//}}}

	//{{{ 查询用户资金
	int ret_trade = m_pTraderApi->QueryAsset(session_id_, 1);//request_id用户可自定义，此处以1为例
	if (ret_trade != 0)
	{
		//查询资金请求发送失败，打印失败原因
		XTPRI* error_info = m_pTraderApi->GetApiLastError();
		cout << "Query asset send error, " << error_info->error_id << " : " << error_info->error_msg << endl;
	}
	//}}}

	
	//}}}

	//{{{  策略模块
	//将全局队列的地址传递给构造函数
	StrategyEngine strategy_engine(&g_market_signal_queue, &g_order_request_queue);

	// 启动
	strategy_engine.start();

	//}}}


	//{{{ 运行三分钟后退出
	sleep(300);
	//{{{ 回收资源
    // --- 6. 优雅关闭 (顺序至关重要！) ---
    std::cout << "Initiating graceful shutdown..." << std::endl;

    // (1) 首先关闭行情队列。这会使策略引擎的循环退出。
    std::cout << "Shutting down market data queue..." << std::endl;
    g_market_signal_queue.shutdown();

    // (2) 停止策略引擎，它会 join 自己的线程。
    std::cout << "Stopping strategy engine..." << std::endl;
    strategy_engine.stop();

    // (3) 策略引擎已停止，不会再产生新订单。现在关闭订单队列。
    std::cout << "Shutting down order queue..." << std::endl;
    g_order_request_queue.shutdown();

    // (4) 所有业务线程都已安全退出，现在可以登出和释放API。
    std::cout << "Logging out and releasing APIs..." << std::endl;

	//{{{ 登出/销毁pTraderApi,pQuoteAPI
	if (m_pQuoteApi) {
	    int ret_logout_m_pQuoteApi = m_pQuoteApi->Logout();
	    if (ret_logout_m_pQuoteApi != 0) {
	        XTPRI* err = m_pQuoteApi->GetApiLastError();
	        std::cerr << "m_pQuoteApi Logout failed, error: " << err->error_id
	                  << ", msg: " << err->error_msg << std::endl;
	    } else {
	        std::cout << "m_pQuoteApi Logout succeeded." << std::endl;
		}
	}


	if (m_pTraderApi) {
	    int ret_logout_m_pTraderApi = m_pTraderApi->Logout(session_id_);
	    if (ret_logout_m_pTraderApi != 0) {
	        XTPRI* err = m_pTraderApi->GetApiLastError();
	        std::cerr << "m_pTraderApi Logout failed, error: " << err->error_id
	                  << ", msg: " << err->error_msg << std::endl;
	    } else {
	        std::cout << "m_pTraderApi Logout succeeded." << std::endl;
	    }
	}

	//}}}


    // 确保API线程已停止后再释放
    std::this_thread::sleep_for(std::chrono::seconds(1)); 

    if (m_pTraderApi) m_pTraderApi->Release();
    if (m_pQuoteApi) m_pQuoteApi->Release();

	// *** 最后，关闭 spdlog。这会确保所有在队列中的日志都被刷入文件。***
	ShutdownLatencyLogging();

    std::cout << "Shutdown complete." << std::endl;
 

    std::this_thread::sleep_for(std::chrono::seconds(60));
	//}}}

	return 0;
	//}}}

	//{{{ 一直运行
//	while(true){
//		sleep(10);
//	}
	//}}}

	//{{{ 收盘后退出
//    // {{{ 在上海/深圳市场 15:00 (即 07:00 UTC) 收盘后10分钟关闭主线程。
//    const int SHUTDOWN_UTC_HOUR = 7;
//    const int SHUTDOWN_UTC_MINUTE = 10; // 留出10分钟缓冲
//
//    while (true) {
//        auto [current_utc_hour, current_utc_minute] = get_current_utc_hour_minute();
//
//        if (current_utc_hour > SHUTDOWN_UTC_HOUR ||
//           (current_utc_hour == SHUTDOWN_UTC_HOUR && current_utc_minute >= SHUTDOWN_UTC_MINUTE))
//        {
//            std::cout << "Target UTC shutdown time ("
//                      << SHUTDOWN_UTC_HOUR << ":" << SHUTDOWN_UTC_MINUTE
//                      << ") reached. Initiating graceful shutdown." << std::endl;
//            break; // 跳出循环，开始关闭流程
//        }
//
//        // 不需要频繁检查，每分钟检查一次足够了
//        std::this_thread::sleep_for(std::chrono::seconds(60));
//    }
//
//    // --- 在这里执行你所有的优雅关闭逻辑 ---
//    std::cout << "Shutting down worker threads and APIs..." << std::endl;
//	//}}}
//
//
//	//{{{ 回收资源
//    // --- 6. 优雅关闭 (顺序至关重要！) ---
//    std::cout << "Initiating graceful shutdown..." << std::endl;
//
//    // (1) 首先关闭行情队列。这会使策略引擎的循环退出。
//    std::cout << "Shutting down market data queue..." << std::endl;
//    g_market_signal_queue.shutdown();
//
//    // (2) 停止策略引擎，它会 join 自己的线程。
//    std::cout << "Stopping strategy engine..." << std::endl;
//    strategy_engine.stop();
//
//    // (3) 策略引擎已停止，不会再产生新订单。现在关闭订单队列。
//    std::cout << "Shutting down order queue..." << std::endl;
//    g_order_request_queue.shutdown();
//
//    // (4) 所有业务线程都已安全退出，现在可以登出和释放API。
//    std::cout << "Logging out and releasing APIs..." << std::endl;
//
//	//{{{ 登出/销毁pTraderApi,pQuoteAPI
//	if (m_pQuoteApi) {
//	    int ret_logout_m_pQuoteApi = m_pQuoteApi->Logout();
//	    if (ret_logout_m_pQuoteApi != 0) {
//	        XTPRI* err = m_pQuoteApi->GetApiLastError();
//	        std::cerr << "m_pQuoteApi Logout failed, error: " << err->error_id
//	                  << ", msg: " << err->error_msg << std::endl;
//	    } else {
//	        std::cout << "m_pQuoteApi Logout succeeded." << std::endl;
//		}
//	}
//
//
//	if (m_pTraderApi) {
//	    int ret_logout_m_pTraderApi = m_pTraderApi->Logout(session_id_);
//	    if (ret_logout_m_pTraderApi != 0) {
//	        XTPRI* err = m_pTraderApi->GetApiLastError();
//	        std::cerr << "m_pTraderApi Logout failed, error: " << err->error_id
//	                  << ", msg: " << err->error_msg << std::endl;
//	    } else {
//	        std::cout << "m_pTraderApi Logout succeeded." << std::endl;
//	    }
//	}
//
//	//}}}
//
//
//    // 确保API线程已停止后再释放
//    std::this_thread::sleep_for(std::chrono::seconds(1)); 
//
//    if (m_pTraderApi) m_pTraderApi->Release();
//    if (m_pQuoteApi) m_pQuoteApi->Release();
//
//	// *** 最后，关闭 spdlog。这会确保所有在队列中的日志都被刷入文件。***
//	ShutdownLatencyLogging();
//
//    std::cout << "Shutdown complete." << std::endl;
// 
//
//    std::this_thread::sleep_for(std::chrono::seconds(60));
//	//}}}

	//}}}
	

}
