#include "trade_spi.h"

using std::string;


//{{{ 外部变量
extern XTP::API::TraderApi* m_pTraderApi;//全局变量，程序共用的api
extern string trader_server_ip;//交易服务器ip地址
extern int trader_server_port;//交易服务器端口port
extern string trader_username;//交易服务器的登陆账户名
extern string trader_password;//交易服务器的登陆密码
extern string local_ip;//用户的本地ip，需要用户自行修改为网卡对应的ip
extern uint64_t session_id_;//用户登陆后对应的session_id_
//}}}

//{{{ MyTraderSpi() 默认构造函数
MyTraderSpi::MyTraderSpi()
	: g_order_request_queue(nullptr)
{
}
//}}}

//{{{ MyTraderSpi() 带参数构造函数，传入共享队列g_order_request_queue在策略和交易之间进行OrderRequest的传输
MyTraderSpi::MyTraderSpi(ThreadSafeQueue<std::unique_ptr<OrderRequest>>* g_order_request_queue)
    : 
	  //传入共享队列g_order_request_queue在策略和交易之间进行OrderRequest的传输
      g_order_request_queue(g_order_request_queue),
	  m_order_thread(&MyTraderSpi::run_order_execution, this)
{
}                                                                                                                                      
//}}}


//{{{ MyTraderSpi() 析构函数
MyTraderSpi::~MyTraderSpi()
{
	// 确保线程可以被安全地 join
    if (m_order_thread.joinable()) {
        m_order_thread.join();
    }
}
//}}}

//{{{  OnDisconnected
void MyTraderSpi::OnDisconnected(uint64_t session_id_, int reason)
{
	//交易服务器断线后，此函数会被调用
	//TODO:重新login，并在login成功后，更新session_id_

	std::cout << "Disconnect from Trader server. " << std::endl;

	//断线后，重新连接
	uint64_t temp_session_id_ = 0;
	while (temp_session_id_ == 0)
	{

		temp_session_id_ = m_pTraderApi->Login(trader_server_ip.c_str(), trader_server_port, trader_username.c_str(), trader_password.c_str(), XTP_PROTOCOL_TCP, local_ip.c_str());
		if (temp_session_id_ == 0)
		{
			// 登录失败，获取错误信息
			XTPRI* error_info = m_pTraderApi->GetApiLastError();
			std::cout << "login to server error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;

			//等待10s以后再次连接，可修改此等待时间，建议不要小于3s
#ifdef _WIN32
			Sleep(10000);
#else
			sleep(10);
#endif // _WIN32    
		}
	} ;

	//重新登录成功后更新session_id_
	std::cout << "login to server success. " << std::endl;
	session_id_ = temp_session_id_;
}
//}}}

//{{{  OnOrderEvent
void MyTraderSpi::OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id_)
{

	std::cout << "-------------------- OnOrderEvent -------------------------" << std::endl;

	//TODO:处理订单响应，此处仅以做屏幕输出为例，用户可以用自己的处理逻辑改写
	std::cout << "ticker: " << order_info->ticker << std::endl;
	std::cout << "price: " << order_info->price << std::endl;
	std::cout << "quantity: " << order_info->quantity << std::endl;
	std::cout << "side: " << SideTypeToString(static_cast<int>(order_info->side)) << std::endl;
	std::cout << "price_type: " << PriceTypeToString(order_info->price_type) << std::endl;
	std::cout << "order_submit_status: " << OrderSubmitStatusToString(order_info->order_submit_status) << std::endl;
	std::cout << "status: " << OrderStatusToString(order_info->order_status)  << std::endl;
	std::cout << "qty_traded: " << order_info->qty_traded << std::endl;
	std::cout << "qty_left: " << order_info->qty_left << std::endl;
	std::cout << "xtp_id: " << order_info->order_xtp_id << std::endl;
	std::cout << "client_id: " << order_info->order_client_id << std::endl;
	std::cout << "cancel_xtp_id: " << order_info->order_cancel_xtp_id << std::endl;
	std::cout << "cancel_client_id: " << order_info->order_cancel_client_id << std::endl;
	std::cout << "market: " << order_info->market << std::endl;
	std::cout << "position_effect: " << static_cast<int>(order_info->position_effect) << std::endl;
	std::cout << "insert_time: " << order_info->insert_time << std::endl;
	std::cout << "update_time: " << order_info->update_time << std::endl;
	std::cout << "cancel_time: " << order_info->cancel_time << std::endl;
	std::cout << "trade_amount: " << order_info->trade_amount << std::endl;
	std::cout << "order_local_id: " << order_info->order_local_id << std::endl;
	std::cout << "order_type: " << order_info->order_type << std::endl;
	


	//TODO:根据报单响应情况来处理
	switch (order_info->order_status)
	{
		case XTP_ORDER_STATUS_NOTRADEQUEUEING:
			{
				//订单确认状态，表示订单被交易所接受
				break;
			}
		case XTP_ORDER_STATUS_ALLTRADED:
			{
				//订单全部成交状态，表示订单到达已完结状态
				break;
			}
		case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING:
			{
				//订单部分撤单状态，表示订单到达已完结状态
				break;
			}
		case XTP_ORDER_STATUS_CANCELED:
			{
				//订单全部撤单状态，表示订单到达已完结状态
				break;
			}
		case XTP_ORDER_STATUS_REJECTED:
			{
				//订单拒单状态，表示订单因有错误而被拒绝，此时可以关注拒单原因
				if (error_info && (error_info->error_id > 0))
				{
					//TODO:说明有错误导致拒单，此处仅以屏幕输出错误信息为例，用户可以用自己的处理逻辑改写
					std::cout << "error_id:" << error_info->error_id << ",error_msg:" << error_info->error_msg << std::endl;
				}
				break;
			}
		default:
			break;
	}
}
//}}}

//{{{  OnTradeEvent
void MyTraderSpi::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id_)
{
	std::cout << "-------------------- OnTradeEvent -------------------------" << std::endl;

	std::cout << "xtp_id:" << trade_info->order_xtp_id << std::endl;
    std::cout << "client_id:" << trade_info->order_client_id << std::endl;
    std::cout << "ticker:" << trade_info->ticker << std::endl;
    std::cout << "market:" << trade_info->market << std::endl;
    std::cout << "price:" << trade_info->price << std::endl;
    std::cout << "quantity:" << trade_info->quantity << std::endl;
    std::cout << "side:" << (int)trade_info->side << std::endl;
    std::cout << "position_effect:" << (int)trade_info->position_effect << std::endl;
    std::cout << "trade_time:" << trade_info->trade_time << std::endl;
    std::cout << "trade_amount:" << trade_info->trade_amount << std::endl;
    std::cout << "exec_id:" << trade_info->exec_id << std::endl;
    std::cout << "report_index:" << trade_info->report_index << std::endl;
    std::cout << "order_exch_id:" << trade_info->order_exch_id << std::endl;
    std::cout << "trade_type:" << trade_info->trade_type << std::endl;
    std::cout << "branch_pbu:" << trade_info->branch_pbu << std::endl;


	//TODO:用户可以根据成交回报来更新本地的持仓
}
//}}}

//{{{  OnCancelOrderError
void MyTraderSpi::OnCancelOrderError(XTPOrderCancelInfo * cancel_info, XTPRI * error_info, uint64_t session_id_)
{
	std::cout << "-----------------OnCancelOrderError---------------------" << std::endl;
	std::cout << "cancel_order_xtp_id:" << cancel_info->order_cancel_xtp_id << "   order_xtp_id:" << cancel_info->order_xtp_id << "  error code:" << error_info->error_id << "  msg:" << error_info->error_msg << std::endl;

}
//}}}

//{{{  OnQueryPosition
void MyTraderSpi::OnQueryPosition(XTPQueryStkPositionRsp * investor_position, XTPRI * error_info, int request_id, bool is_last, uint64_t session_id_)
{
	if (error_info && error_info->error_id !=0)
	{
		//查询出错
		if (error_info->error_id == 11000350)
		{
			//账户里没有持仓
			std::cout << "------------------- Position is empty.-----------" << std::endl;
		}
		else
		{
			//真正的出错
		}
		return;
	}

	//TODO:处理查询持仓回报逻辑，此处仅以屏幕输出为例
	std::cout << "request_id:" << request_id << ",is_last:" << is_last << ",";
	std::cout << "ticker:" << investor_position->ticker << ",ticker_name:" << investor_position->ticker_name;
	std::cout << ",total_qty:" << investor_position->total_qty << ",sellable_qty:" << investor_position->sellable_qty << ",avg_price:" << investor_position->avg_price;
	std::cout << ",unrealized_pnl:" << investor_position->unrealized_pnl << std::endl;

	//{{{  持仓回报结束后开始发送订单 InsertOrder
	if (is_last)
	{
		//TODO：为最后一条持仓回报，可以进行后续的处理逻辑，此处以报单为例

		// {{{ 官方示例
//		//报单 
//		//初始化报单结构体，以9.01的限价买入沪市"600000"1000股为例。
//		XTPOrderInsertInfo order;
//		memset(&order, 0, sizeof(XTPOrderInsertInfo));
//		order.market = XTP_MKT_SH_A;
//		string stdstr_ticker = "600000";
//		strncpy(order.ticker, stdstr_ticker.c_str(), XTP_TICKER_LEN);
//		order.business_type = XTP_BUSINESS_TYPE_CASH;
//		order.side = XTP_SIDE_BUY;
//		order.position_effect = XTP_POSITION_EFFECT_INIT;
//		order.price_type = XTP_PRICE_LIMIT;
//		order.price = 9.01;
//		order.quantity = 1000;
//
//		uint64_t ret = m_pTraderApi->InsertOrder(&order, session_id_);
//		if (ret == 0)
//		{
//			// 报单失败，获取失败原因
//			XTPRI* error_info = m_pTraderApi->GetApiLastError();
//			std::cout << "Insert order error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;
//		}
//		else
//		{
//			// 报单成功会返回order_xtp_id，它保证一个交易日内唯一
//			//TODO:其他逻辑，建议用户将报单在本地按照order_xtp_id保存，此时可以视报单状态为初始状态
//		}
		//}}}
		
		send_order();

	}
	//}}}
}
//}}}


//{{{  OnQueryAsset
void MyTraderSpi::OnQueryAsset(XTPQueryAssetRsp * trading_account, XTPRI * error_info, int request_id, bool is_last, uint64_t session_id_)
{
	std::cout << "------------------- OnQueryTradingAccount-----------" << std::endl;
	//TODO:处理查询资金逻辑，此处仅以屏幕输出并触发查询持仓为例
	std::cout << "request_id:" << request_id << std::endl;
	std::cout << "total_asset:" << trading_account->total_asset << std::endl;
	std::cout << "security_asset:" << trading_account->security_asset << std::endl;
	std::cout << "buying_power:" << trading_account->buying_power << std::endl;


	//查询用户所有持仓
	int ret = m_pTraderApi->QueryPosition(NULL, session_id_, 2);//request_id用户可自定义，此处以2为例
	if (ret != 0)
	{
		//查询持仓请求发送失败，打印失败原因
		XTPRI* error_info = m_pTraderApi->GetApiLastError();
		std::cout << "Query position send error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;
	}
	
}
//}}}


//{{{ send_order() 
void MyTraderSpi::send_order()
{
    //{{{ 从 tickers_SZ.csv 中订阅SZ股票
    auto tickers_SZ = LoadTickersFromCSV("/root/my_xtp/src/tickers_SZ.csv");
    for (const auto& ticker : tickers_SZ) {
		XTPOrderInsertInfo order;
		memset(&order, 0, sizeof(XTPOrderInsertInfo));
		order.market = XTP_MKT_SZ_A;
		std::string stdstr_ticker = ticker;
		strncpy(order.ticker, stdstr_ticker.c_str(), XTP_TICKER_LEN);
		order.business_type = XTP_BUSINESS_TYPE_CASH;
		order.side = XTP_SIDE_BUY;
		order.position_effect = XTP_POSITION_EFFECT_INIT;
		order.price_type = XTP_PRICE_BEST_OR_CANCEL; //即时成交剩余转撤销，市价单-深
		order.price = 0;
		order.quantity = 1000;

		uint64_t ret = m_pTraderApi->InsertOrder(&order, session_id_);
		if (ret == 0)
		{
			// 报单失败，获取失败原因
			XTPRI* error_info = m_pTraderApi->GetApiLastError();
			std::cout << "Insert order error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;
		}
        std::cout << "Ticker: " << ticker << std::endl;
    }
    //}}}

    //{{{ 从 tickers_SH.csv 中订阅SH股票
    auto tickers_SH = LoadTickersFromCSV("/root/my_xtp/src/tickers_SH.csv");
    for (const auto& ticker : tickers_SH) {
        std::cout << "Ticker: " << ticker << std::endl;
		XTPOrderInsertInfo order;
		memset(&order, 0, sizeof(XTPOrderInsertInfo));
		order.market = XTP_MKT_SH_A;
		std::string stdstr_ticker = ticker;
		strncpy(order.ticker, stdstr_ticker.c_str(), XTP_TICKER_LEN);
		order.business_type = XTP_BUSINESS_TYPE_CASH;
		order.side = XTP_SIDE_BUY;
		order.position_effect = XTP_POSITION_EFFECT_INIT;
		order.price_type = 	XTP_PRICE_BEST5_OR_CANCEL;///<最优5档即时成交剩余转撤销，市价单-沪深
		order.price = 0;
		order.quantity = 1000;

		uint64_t ret = m_pTraderApi->InsertOrder(&order, session_id_);
		if (ret == 0)
		{
			// 报单失败，获取失败原因
			XTPRI* error_info = m_pTraderApi->GetApiLastError();
			std::cout << "Insert order error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;
		}
        std::cout << "Ticker: " << ticker << std::endl;


    }
    //}}}
}
//}}}


//{{{ run_order_execution() 订单执行线程的实现
void MyTraderSpi::run_order_execution() {
	std::unique_ptr<OrderRequest> order_to_send;

    while (g_order_request_queue->pop(order_to_send)) {
		// T5: 订单从队列取出
        record_latency_log("T5_order_request_queue_pop", order_to_send->ticker.c_str(),order_to_send->trace_id);


        // --- 使用方案A：直接创建和推送 XTPOrderInsertInfo 的智能指针 ---
        // 1. 使用 std::make_unique 在堆上创建 XTPOrderInsertInfo 对象。
        auto order_ptr_xtp = std::make_unique<XTPOrderInsertInfo>();
        memset(order_ptr_xtp.get(), 0, sizeof(XTPOrderInsertInfo)); // get() 返回原始指针
        
        strncpy(order_ptr_xtp->ticker, order_to_send->ticker.c_str(), XTP_TICKER_LEN - 1);
        order_ptr_xtp->market = order_to_send->market;
  	    order_ptr_xtp->side = order_to_send->side;
 	    order_ptr_xtp->price_type = order_to_send->price_type;
 	    order_ptr_xtp->price = order_to_send->price;
   	    order_ptr_xtp->quantity = order_to_send->quantity; 
  	    order_ptr_xtp->business_type = order_to_send->business_type;
 	    order_ptr_xtp->position_effect = order_to_send->position_effect;
        // 注意：订单号 order_xtp_id 和委托时间 order_time 是由XTP系统回报时填充的，我们在下单时不需要填写。
 		
		// T6: 调用API后
   	    record_latency_log("T6_onorderevent", order_to_send->ticker.c_str(),order_to_send->trace_id);

        // 直接使用从队列中取出的对象
        m_pTraderApi->InsertOrder(order_ptr_xtp.get(), session_id_);//order_to_send是unique_ptr,.get()获取unique_ptr的裸指针
																	



    }
}
//}}}

