#include "quote_spi.h"

using namespace std;

//{{{ 外部变量
extern XTP::API::QuoteApi* m_pQuoteApi;
extern string quote_server_ip;//行情服务器ip地址
extern int quote_server_port;//行情服务器端口port
extern string quote_username;//行情服务器的登陆账户名
extern string quote_password;//行情服务器的登陆密码
extern XTP_PROTOCOL_TYPE protocol_type;//Level1服务器使用TCP，Level2服务器请用UDP，公网测试环境均为TCP，具体以运营通知为准
extern string local_ip;//本地网卡对应的ip
//}}}


//{{{ MyQuoteSpi() 默认构造函数
MyQuoteSpi::MyQuoteSpi()
				//QueryAllTickersFullInfo
    : query_full_info_callback_count(0),
      SZ_QueryAllTickersFullInfo_finished_down(false),
      SH_QueryAllTickersFullInfo_finished_down(false),
				//depth_market_data_processing_thread_ = thread(&MyQuoteSpi::depth_market_data_worker, this);
      depth_market_data_processing_thread_(&MyQuoteSpi::depth_market_data_worker, this),
				// 共享队列g_market_signal_queue默认为空指针
	  g_market_signal_queue(nullptr)
{
				//QueryAllTickersFullInfo
    ticker_full_infos_SZ.clear();
    ticker_full_infos_SH.clear();
}

//}}}

//{{{ MyQuoteSpi() 带参数构造函数，传入共享队列在行情和策略之间进行MarketDataPacket的传输
MyQuoteSpi::MyQuoteSpi(ThreadSafeQueue<unique_ptr<MarketDataPacket>>* g_market_signal_queue)
				//QueryAllTickersFullInfo
    : query_full_info_callback_count(0),
      SZ_QueryAllTickersFullInfo_finished_down(false),
      SH_QueryAllTickersFullInfo_finished_down(false),
				//depth_market_data_processing_thread_ = thread(&MyQuoteSpi::depth_market_data_worker, this);
      depth_market_data_processing_thread_(&MyQuoteSpi::depth_market_data_worker, this),
	  	        // 传入共享队列g_market_signal_queue，实现行情与策略之间的数据传输
	  g_market_signal_queue(g_market_signal_queue)
{
				//QueryAllTickersFullInfo
    ticker_full_infos_SZ.clear();
    ticker_full_infos_SH.clear();
}
//}}}


//{{{ MyQuoteSpi()析构函数
MyQuoteSpi::~MyQuoteSpi()
{
	depth_market_data_queue_.shutdown();

	if (depth_market_data_processing_thread_.joinable()) {
		depth_market_data_processing_thread_.join();
	}

}
//}}}

//{{{ OnDisconnected(int reason)
void MyQuoteSpi::OnDisconnected(int reason)
{
	//行情服务器断线后，此函数会被调用
	//TODO:重新login，并在login成功后，再次订阅

	cout << "Disconnect from quote server. " << endl;

	//重新登陆行情服务器
	int ret = -1;
	while (0 != ret)
	{
		ret = m_pQuoteApi->Login(quote_server_ip.c_str(), quote_server_port, quote_username.c_str(), quote_password.c_str(), protocol_type, local_ip.c_str());
		if (1 != ret)
		{
			// 登录失败，获取错误信息
			XTPRI* error_info = m_pQuoteApi->GetApiLastError();
			cout << "login to server error, " << error_info->error_id << " : " << error_info->error_msg << endl;

			//等待10s以后再次连接，可修改此等待时间，建议不要小于3s
#ifdef _WIN32
			Sleep(10000);
#else
			sleep(10);
#endif // _WIN32	
		}
	}

	//重连成功
	cout << "login to server success. " << endl;
	//再次订阅行情快照
	subscribeMarketData();
}
//}}}

//{{{ 1 ---  OnQueryAllTickersFullInfo(XTPQFI* ticker_info, XTPRI *error_info, bool is_last)
void MyQuoteSpi::OnQueryAllTickersFullInfo(XTPQFI* ticker_info, XTPRI *error_info, bool is_last)
{
	query_full_info_callback_count ++;

	//cout << query_full_info_callback_count << endl;

	if (error_info && error_info->error_id != 0){
		//查询失败
		cout << "OnQueryAllTickersFullInfo -----" << "error_id = " << error_info->error_id << ", error_msg = " << error_info->error_msg << endl;
		return;
	}

	//查询成功
	//TODO:将查询结果缓存

	// 只有有效ticker_info才保存
	if (ticker_info) {
		string code(ticker_info->ticker);

		//只保存主板股票
		if (ticker_info->exchange_id == XTP_EXCHANGE_SH && ticker_info->security_type == XTP_SECURITY_MAIN_BOARD && ticker_info->security_status == XTP_SECURITY_STATUS_COMMON){
			ticker_full_infos_SH[code] = *ticker_info;
			//cout << code << endl;
		}

		//只保存主板股票
		else if (ticker_info->exchange_id == XTP_EXCHANGE_SZ && ticker_info->security_type == XTP_SECURITY_MAIN_BOARD && ticker_info->security_status == XTP_SECURITY_STATUS_COMMON){
			ticker_full_infos_SZ[code] = *ticker_info;
			//cout << code << endl;
		}
	}

	// 是否已查询完
	if(is_last && ticker_info->exchange_id == XTP_EXCHANGE_SH){
		SH_QueryAllTickersFullInfo_finished_down = true;
		cout << "[OnQueryAllTickersFullInfo] This was the last response for the query --- SH." << endl;
	}
	if(is_last && ticker_info->exchange_id == XTP_EXCHANGE_SZ){
		SZ_QueryAllTickersFullInfo_finished_down = true;
		cout << "[OnQueryAllTickersFullInfo] This was the last response for the query --- SZ." << endl;
	}


	//当沪市的静态行情查询完毕后，查询深市行情静态信息
	if (is_last && ticker_info->exchange_id == XTP_EXCHANGE_SH){
		m_pQuoteApi->QueryAllTickersFullInfo(XTP_EXCHANGE_SZ);
	}

	if(SH_QueryAllTickersFullInfo_finished_down && SZ_QueryAllTickersFullInfo_finished_down){
		subscribeMarketData();
	}
}
//}}}

//{{{ OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
void MyQuoteSpi::OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
{
	if (error_info && error_info->error_id != 0){
		//订阅失败
		cout << "OnSubMarketData -----" << "error_id = " << error_info->error_id << ", error_msg = " << error_info->error_msg << ",exchange_id = " << ticker->exchange_id  << "   " << ticker->ticker << endl;
		return;
	}
	else{

	}
	//订阅成功
	//TODO:当is_last == true时，触发其他用户逻辑	
}
//}}}

//{{{ OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
void MyQuoteSpi::OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
{
	if (error_info && error_info->error_id != 0)
	{
		//取消订阅失败
		cout << "OnUnSubMarketData -----" << "error_id = " << error_info->error_id << ", error_msg = " << error_info->error_msg << endl;
		return;
	}

	//退阅成功
	//TODO:当is_last == true时，触发其他用户逻辑	
}
//}}}

//{{{ OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count)
void MyQuoteSpi::OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count)
{
	// 1. 创建一个数据包，其中包含接收到数据的 *副本*, 使用 make_unique 来确保异常安全和自动内存管理。
	auto packet_ptr = make_unique<MarketDataPacket>(
			market_data,      // 原始行情数据
			bid1_qty, bid1_count, // 买一队列
			ask1_qty, ask1_count,  // 卖一队列
			0);

	// 2. 将指向数据包的智能指针推入线程安全队列。
	depth_market_data_queue_.push(std::move(packet_ptr));



	// === 使用 g_trace_id_generator 生成唯一的 TraceID ===
    uint64_t current_trace_id = g_trace_id_generator.fetch_add(1);

	// T1: 行情数据到达应用层
    record_latency_log("T1_MD_received", market_data->ticker,current_trace_id);
	//  推入共享的外部队列 行情-策略（注意再构造一个新的 packet）
	
    auto packet_for_g = std::make_unique<MarketDataPacket>(
        market_data,
        bid1_qty, bid1_count,
        ask1_qty, ask1_count,
		current_trace_id
    );

    if (g_market_signal_queue) {
		// T2: 行情数据即将入队
        record_latency_log("T2_market_signal_queue_push", market_data->ticker,packet_for_g->trace_id);
        g_market_signal_queue->push(std::move(packet_for_g));
    }

	// 4. 立即返回。所有繁重的处理都已卸载。
	//收到行情快照数据
	//TODO:用户处理逻辑，注意此处不能仅仅保存数据的指针，指针所指向的内存数据将在此函数return后失效
}
//}}}


//{{{ subscribeMarketData()
void MyQuoteSpi::subscribeMarketData()
{
//	//{{{ 订阅全SZ股票
//	int ticker_cnt_SZ = ticker_full_infos_SZ.size(); // SZ所有股票数量,需要订阅行情的证券代码数量
//	char **ppInstrumentID_SZ = new char*[ticker_cnt_SZ]; //申请指针数组
//	int idx_SZ = 0;
//
//	//将ticker_full_infos_SZ的所有股票代码存入ppInstrumentID_SZ
//	for (const auto& pair : ticker_full_infos_SZ){
//		ppInstrumentID_SZ[idx_SZ] = new char[XTP_TICKER_LEN];
//		strncpy(ppInstrumentID_SZ[idx_SZ], pair.first.c_str(), XTP_TICKER_LEN);
//		ppInstrumentID_SZ[idx_SZ][XTP_TICKER_LEN] = '\0';
//		//cout << ppInstrumentID_SZ[idx_SZ] << endl;
//		idx_SZ++;
//	}
//	m_pQuoteApi->SubscribeMarketData(ppInstrumentID_SZ, ticker_cnt_SZ, XTP_EXCHANGE_SZ);
//	//}}}

//	//{{{ 订阅全SH股票
//	int ticker_cnt_SH = ticker_full_infos_SH.size(); // SH所有股票数量
//	char **ppInstrumentID_SH = new char*[ticker_cnt_SH];
//	int idx_SH = 0;
//	//将ticker_full_infos_SH的所有股票代码存入ppInstrumentID_SH
//	for (const auto& pair : ticker_full_infos_SH){
//		ppInstrumentID_SH[idx_SH] = new char[XTP_TICKER_LEN];
//		strncpy(ppInstrumentID_SH[idx_SH], pair.first.c_str(), XTP_TICKER_LEN);
//		ppInstrumentID_SH[idx_SH][XTP_TICKER_LEN] = '\0';
//		idx_SH++;
//	}
//	m_pQuoteApi->SubscribeMarketData(ppInstrumentID_SH, ticker_cnt_SH, XTP_EXCHANGE_SH);
//	//}}}

	//{{{ 从 tickers_SZ.csv 中订阅SZ股票
	auto tickers_SZ = LoadTickersFromCSV("/root/my_xtp/src/tickers_SZ.csv");
	int ticker_cnt_SZ = tickers_SZ.size(); //股票代码的数量
	char **ppInstrumentID_SZ = new char*[ticker_cnt_SZ]; //申请指针数组
	int idx_SZ = 0;
    for (const auto& ticker : tickers_SZ) {
        std::cout << "Ticker: " << ticker << std::endl;
		ppInstrumentID_SZ[idx_SZ] = new char[XTP_TICKER_LEN];
		strncpy(ppInstrumentID_SZ[idx_SZ], ticker.c_str(), XTP_TICKER_LEN);
		//ppInstrumentID_SZ[idx_SZ][6] = '\0';
		idx_SZ++;
    }
	m_pQuoteApi->SubscribeMarketData(ppInstrumentID_SZ, ticker_cnt_SZ, XTP_EXCHANGE_SZ);
	//}}}

	//{{{ 从 tickers_SH.csv 中订阅SH股票
	auto tickers_SH = LoadTickersFromCSV("/root/my_xtp/src/tickers_SH.csv");
	int ticker_cnt_SH = tickers_SH.size(); //股票代码的数量
	char **ppInstrumentID_SH = new char*[ticker_cnt_SH]; //申请指针数组
	int idx_SH = 0;
    for (const auto& ticker : tickers_SH) {
        std::cout << "Ticker: " << ticker << std::endl;
		ppInstrumentID_SH[idx_SH] = new char[XTP_TICKER_LEN];
		strncpy(ppInstrumentID_SH[idx_SH], ticker.c_str(), XTP_TICKER_LEN);
		//ppInstrumentID_SH[idx_SH][6] = '\0';
		idx_SH++;
    }
	m_pQuoteApi->SubscribeMarketData(ppInstrumentID_SH, ticker_cnt_SH, XTP_EXCHANGE_SH);
	//}}}

	//{{{ 释放内存 ppInstrumentID_SZ SH
	for (int i = 0; i < ticker_cnt_SZ; i++) 
	{ 
		delete[] ppInstrumentID_SZ[i];
		ppInstrumentID_SZ[i] = NULL;
	}
	delete[] ppInstrumentID_SZ;
	ppInstrumentID_SZ = NULL;

	for (int i = 0; i < ticker_cnt_SH; i++) {
		delete[] ppInstrumentID_SH[i];
		ppInstrumentID_SH[i] = NULL;
	}
	delete[] ppInstrumentID_SH;
	ppInstrumentID_SH = NULL;
	//}}}

}
//}}}


//{{{ depth_market_data_worker() 这个函数在工作线程 depth_market_data_processing_thread_ 中运行
void MyQuoteSpi::depth_market_data_worker() {
    cout << "行情数据处理工作线程已启动。" << endl;
    while (true) {
        unique_ptr<MarketDataPacket> packet_ptr;
        if (!depth_market_data_queue_.pop(packet_ptr)) { 
			// 阻塞式地从队列中取出数据包,同时实现将队列中的数据取到packet_ptr中和判断shutdown_flag && queue.empty() 是否同时为真
        	// pop 返回 false 表示队列正在关闭且已空,只有MyQuoteSpi析构让shutdown_flag为true和队列中没有数据时才成立
            if (depth_market_data_queue_.is_shutting_down()) {
                cout << "行情数据处理工作线程正在关闭。" << endl;
                break; // 退出循环，结束线程
            }
        }

        if (packet_ptr) {
            // 现在我们获取到了数据包，可以进行处理 (例如，打印它)
            print_market_data_packet(*packet_ptr); //解引用packet_ptr 传入原始结构体信息,打印股票信息ticker|最新价 last_price|>    昨收盘 pre_close_price|今开盘open_price|最高价high_price|最低价low_price|今收盘close_price;

                                                   // 其他可能的处理逻辑:
                                                   // - 更新内部的策略模型
                                                   // - 写入数据库或日志文件
                                                   // - 更新用户界面 (UI)
        }

    }
}
//}}}

//{{{ print_market_data_packet() //打印最新价、昨收、开盘、最高、最低、收盘
void MyQuoteSpi::print_market_data_packet(const MarketDataPacket& packet)
{
	const XTPMD& md = packet.market_data_copy;
	cout << setw(14) << "Ticker"
		<< setw(14) << "Last_price"
		<< setw(14) << "precls_price"
		<< setw(14) << "Open_price"
		<< setw(14) << "High_Price"
		<< setw(14) << "Low_price"
		<< setw(14) << "Close_price"
		<< setw(14) << "qty"
		<< setw(5) << "turnover"
		<< endl;
	cout << "-----------------------------------------------------------------------------------------------------------------------" << endl;

	cout << setw(14) << md.ticker
		<< setw(14) << fixed << setprecision(2) << md.last_price
		<< setw(14) << fixed << setprecision(2) << md.pre_close_price
		<< setw(14) << fixed << setprecision(2) << md.open_price
		<< setw(14) << fixed << setprecision(2) << md.high_price
		<< setw(14) << fixed << setprecision(2) << md.low_price
		<< setw(12) << fixed << setprecision(2) << md.close_price
		<< setw(12) << fixed << setprecision(2) << md.qty
		<< setw(12) << fixed << setprecision(2) << md.turnover
		<< endl;

	cout << endl;
	cout << "------------------- 盘口行情（十档） ------------------" << endl;

	for (int i = 0; i < 10; ++i) {
	    cout << left
	         << setw(6)  << ("买" + std::to_string(i + 1) + ":")
	         << setw(10) << fixed << setprecision(2) << md.bid[i]
	         << setw(10) << md.bid_qty[i]
	         << setw(6)  << "|"
	         << setw(6)  << ("卖" + std::to_string(i + 1) + ":")
	         << setw(10) << fixed << setprecision(2) << md.ask[i]
	         << setw(10) << md.ask_qty[i]
	         << endl;
	}
	cout << "-------------------------------------------------------" << endl;
	cout << endl;
	cout << endl;

	
}
//}}}












