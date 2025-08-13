#pragma once
#include "xtp_trader_api.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <thread>          
#include "self_define_function.h"
#include "self_define_DataStruct.h"
#include "ThreadSafeQueue.h"
#include "LatencyTracer.h"


using namespace XTP::API;

class MyTraderSpi : public TraderSpi
{
	public:
		MyTraderSpi();
		MyTraderSpi(ThreadSafeQueue<std::unique_ptr<OrderRequest>>* g_order_request_queue);  // 指针传入队列,带参数构造函数
		~MyTraderSpi();

		///交易服务器断线通知
		void OnDisconnected(uint64_t session_id, int reason);

		///报单响应通知
		void OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id);

		///成交回报通知
		void OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id);

		///撤单失败通知
		void OnCancelOrderError(XTPOrderCancelInfo *cancel_info, XTPRI *error_info, uint64_t session_id);

		///查询持仓回调响应
		void OnQueryPosition(XTPQueryStkPositionRsp *investor_position, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);

		///查询资金回调响应
		void OnQueryAsset(XTPQueryAssetRsp *trading_account, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);

		//TODO:根据需要可继续重写其他回调函数

	private:

		//{{{ 自定义函数
		void send_order(); //报单
		//}}}
		

		//{{{ 传入共享队列g_market_signal_queue，实现策略与交易之间的数据传输
        ThreadSafeQueue<std::unique_ptr<OrderRequest>>* g_order_request_queue;
        //}}}
		
		// 下单执行
    	void run_order_execution();		// 作为订单执行线程的入口点
		std::thread m_order_thread;     // 订单执行线程

};
