#pragma once
#include "xtp_quote_api.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <queue>
#include <iomanip>
#include "self_define_DataStruct.h"
#include <chrono>
#include "xtp_api_struct.h" // 假设 XTPMD 定义在这里




// {{{ MarketDataPacket --- 行情数据包 ---
// 用于在行情回调和策略线程之间传递
struct MarketDataPacket {
    XTPMD market_data_copy;
    std::vector<int64_t> bid1_queue;
    std::vector<int64_t> ask1_queue;
    std::chrono::system_clock::time_point timestamp;

	// === 唯一的 TraceID ,用以性能分析
	uint64_t trace_id; 

	//含参构造
    explicit MarketDataPacket(
        const XTPMD* market_data,
        const int64_t* bid1_qty, int32_t bid1_count,
        const int64_t* ask1_qty, int32_t ask1_count,
		const uint64_t trace_id);

	//默认构造
    MarketDataPacket() = default;
};
//}}}


//{{{ OrderRequest 订单请求
struct OrderRequest {
    std::string ticker;
    XTP_MARKET_TYPE market;
    XTP_SIDE_TYPE side;
    XTP_PRICE_TYPE price_type;
    double price;
    int quantity;
    XTP_BUSINESS_TYPE business_type;
	XTP_POSITION_EFFECT_TYPE position_effect;

	// === 唯一的 TraceID ,用以性能分析
	uint64_t trace_id; 



	// 2. 带参数的构造函数
    OrderRequest(
        const std::string& ticker_str_val,
        XTP_MARKET_TYPE market_val,
        XTP_SIDE_TYPE side_val,
        XTP_PRICE_TYPE price_type_val,
        double price_val,
        int64_t quantity_val,
	    XTP_BUSINESS_TYPE business_type_val,
		XTP_POSITION_EFFECT_TYPE position_effect_val,
		const uint64_t trace_id
    );

	//默认构造
    OrderRequest() = default;

};
//}}}
