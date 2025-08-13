#include "self_define_DataStruct.h"


//{{{ --- MarketDataPacket 行情数据包
// 这是一个纯粹的数据聚合类型，用于在线程间传递打包好的行情快照。
// 使用 struct 能更好地表达其作为数据容器的意图。
MarketDataPacket::MarketDataPacket(
    const XTPMD* market_data,
    const int64_t* bid1_qty, int32_t bid1_count,
    const int64_t* ask1_qty, int32_t ask1_count,
	const uint64_t trace_id)
    : timestamp(std::chrono::system_clock::now()),
	  trace_id(trace_id)
{
    if (market_data) {
        market_data_copy = *market_data; // 行情数据结构体副本
    } else {
        memset(&market_data_copy, 0, sizeof(XTPMD));
    }

	// 安全地拷贝买一队列数据,使用指针和计数值的迭代器构造函数来填充vector
    if (bid1_qty && bid1_count > 0) {
        bid1_queue.assign(bid1_qty, bid1_qty + bid1_count);
    }

	// 安全地拷贝卖一队列数据,使用指针和计数值的迭代器构造函数来填充vector
    if (ask1_qty && ask1_count > 0) {
        ask1_queue.assign(ask1_qty, ask1_qty + ask1_count);
    }
}
//}}}


//{{{ OrderRequest 带参数构造函数的实现
OrderRequest::OrderRequest(
    const std::string& ticker_str_val,
    XTP_MARKET_TYPE market_val,
    XTP_SIDE_TYPE side_val,
    XTP_PRICE_TYPE price_type_val,
    double price_val,
    int64_t quantity_val,
    XTP_BUSINESS_TYPE business_type_val,
    XTP_POSITION_EFFECT_TYPE position_effect_val,
	const uint64_t trace_id_val

) : // 初始化列表，用于初始化非数组类型的成员
	ticker(ticker_str_val),
    market(market_val),
    side(side_val),
    price_type(price_type_val),
    price(price_val),
    quantity(quantity_val),
	business_type(business_type_val),
	position_effect(position_effect_val),
	trace_id(trace_id_val)
{
}
//}}} 
