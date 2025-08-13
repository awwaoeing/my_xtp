#pragma once
#include "xtp_quote_api.h"
#include <string>
#include <map>
#include <mutex>              // 用于线程安全
#include <condition_variable> // 用于同步
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <queue>
#include <iomanip>
#include <fstream>
#include "ThreadSafeQueue.h"
#include "self_define_DataStruct.h"
#include "self_define_function.h"
#include "LatencyTracer.h"

using namespace XTP::API;
using namespace std;


//{{{ MyQuoteSpi 结构体定义
class MyQuoteSpi : public QuoteSpi
{
	public:
		MyQuoteSpi();// 默认构造函数
 		MyQuoteSpi(ThreadSafeQueue<std::unique_ptr<MarketDataPacket>>* g_market_signal_queue);  // 指针传入队列,带参数构造函数

		~MyQuoteSpi();

		///行情服务器断线通知
		void OnDisconnected(int reason);

		///订阅快照行情应答
		void OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last);

		///订阅快照行情应答
		void OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last);

		///查询合约完整静态信息的应答
		void OnQueryAllTickersFullInfo(XTPQFI* ticker_info, XTPRI *error_info, bool is_last);

		///快照行情通知，包含买一卖一队列
		void OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count);





	private:
		void subscribeMarketData();//订阅行情
		void print_market_data_packet(const MarketDataPacket& packet);   



		//OnQueryAllTickersFullInfo  ////////////////////////////////////
		map<string, XTPQFI> ticker_full_infos_SZ; // 存所有SZ合约的信息
		map<string, XTPQFI> ticker_full_infos_SH; // 存所有SH合约的信息
		int query_full_info_callback_count; // 存储查询到第几个合约
		bool SZ_QueryAllTickersFullInfo_finished_down; //SZ是否已经查询完
		bool SH_QueryAllTickersFullInfo_finished_down; //SH是否已经查询完



		//{{{ OnDepthMarketData  ////////////////////////////////////
		//安全队列，推入自定义行情数据包结构体
		ThreadSafeQueue<unique_ptr<MarketDataPacket>> depth_market_data_queue_; // 存储行情数据包指针的队列
		thread depth_market_data_processing_thread_;// 数据处理工作线程

		// 这个函数在depth_market_data_processing_thread_线程中运行
		void depth_market_data_worker();
		//}}}


		//{{{ 传入共享队列g_market_signal_queue，实现行情与策略之间的数据传输
		ThreadSafeQueue<std::unique_ptr<MarketDataPacket>>* g_market_signal_queue;
		//}}}

};
//}}}









