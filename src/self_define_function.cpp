#include "self_define_function.h"

//{{{ 自定义函数1 LoadTickersFromCSV (读取外部文件中的代码 LoadTickersFromCSV)
std::vector<std::string> LoadTickersFromCSV(const std::string& file_path) { 
    std::vector<std::string> tickers; 
    std::ifstream file(file_path); 
    std::string line; 
 
    if (!file.is_open()) { 
        std::cerr << "Failed to open file: " << file_path << std::endl; 
        return {}; 
    } 
 
    while (std::getline(file, line)) { 
        // 忽略空行或标题行 
        if (line.empty() || line == "ticker," || line == "ticker")  continue; 
 
		std::cout << line << std::endl; 
        tickers.push_back(line); 
    } 
 
    return tickers; 
} 
//}}} 
 
//{{{ 自定义函数2 OrderStatusToString (将 XTP_ORDER_STATUS_TYPE报单状态类型 的枚举类型转化为字符串)
const char* OrderStatusToString(XTP_ORDER_STATUS_TYPE status) {
    switch (status) {
        case XTP_ORDER_STATUS_INIT:
            return "XTP_ORDER_STATUS_INIT（初始化）";
        case XTP_ORDER_STATUS_ALLTRADED:
            return "XTP_ORDER_STATUS_ALLTRADED（全部成交）";
        case XTP_ORDER_STATUS_PARTTRADEDQUEUEING:
            return "XTP_ORDER_STATUS_PARTTRADEDQUEUEING（部分成交排队中）";
        case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING:
            return "XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING（部分成交不在队列）";
        case XTP_ORDER_STATUS_NOTRADEQUEUEING:
            return "XTP_ORDER_STATUS_NOTRADEQUEUEING（未成交排队中）";
        case XTP_ORDER_STATUS_CANCELED:
            return "XTP_ORDER_STATUS_CANCELED（已撤单）";
        case XTP_ORDER_STATUS_REJECTED:
            return "XTP_ORDER_STATUS_REJECTED（已拒绝）";
        case XTP_ORDER_STATUS_UNKNOWN:
            return "XTP_ORDER_STATUS_UNKNOWN（未知状态）";
        default:
            return "未知XTP_ORDER_STATUS_TYPE";
    }
}
//}}}

//{{{ 自定义函数3 OrderSubmitStatusToString (将 XTP_ORDER_SUBMIT_STATUS_TYPE 报单状态类型 的枚举类型转化为字符串)

const char* OrderSubmitStatusToString(XTP_ORDER_SUBMIT_STATUS_TYPE status) {
    switch (status) {
        case XTP_ORDER_SUBMIT_STATUS_INSERT_SUBMITTED:
            return "XTP_ORDER_SUBMIT_STATUS_INSERT_SUBMITTED（订单已提交）";
        case XTP_ORDER_SUBMIT_STATUS_INSERT_ACCEPTED:
            return "XTP_ORDER_SUBMIT_STATUS_INSERT_ACCEPTED（订单已被接受）";
        case XTP_ORDER_SUBMIT_STATUS_INSERT_REJECTED:
            return "XTP_ORDER_SUBMIT_STATUS_INSERT_REJECTED（订单已被拒绝）";
        case XTP_ORDER_SUBMIT_STATUS_CANCEL_SUBMITTED:
            return "XTP_ORDER_SUBMIT_STATUS_CANCEL_SUBMITTED（撤单已提交）";
        case XTP_ORDER_SUBMIT_STATUS_CANCEL_REJECTED:
            return "XTP_ORDER_SUBMIT_STATUS_CANCEL_REJECTED（撤单被拒绝）";
        case XTP_ORDER_SUBMIT_STATUS_CANCEL_ACCEPTED:
            return "XTP_ORDER_SUBMIT_STATUS_CANCEL_ACCEPTED（撤单已接受）";
        default:
            return "未知提交状态";
    }
}
//}}}

//{{{ 自定义函数4 PriceTypeToString (将 XTP_PRICE_TYPE 的枚举类型转化为字符串)
const char* PriceTypeToString(XTP_PRICE_TYPE price_type) {
    switch (price_type) {
        case XTP_PRICE_LIMIT:
            return "XTP_PRICE_LIMIT（限价单）";
        case XTP_PRICE_BEST_OR_CANCEL:
            return "XTP_PRICE_BEST_OR_CANCEL（即时成交剩余转撤销）";
        case XTP_PRICE_BEST5_OR_LIMIT:
            return "XTP_PRICE_BEST5_OR_LIMIT（最优五档即时成交剩余转限价）";
        case XTP_PRICE_BEST5_OR_CANCEL:
            return "XTP_PRICE_BEST5_OR_CANCEL（最优五档即时成交剩余转撤销）";
        case XTP_PRICE_ALL_OR_CANCEL:
            return "XTP_PRICE_ALL_OR_CANCEL（全部成交或撤销）";
        case XTP_PRICE_FORWARD_BEST:
            return "XTP_PRICE_FORWARD_BEST（本方最优）";
        case XTP_PRICE_REVERSE_BEST_LIMIT:
            return "XTP_PRICE_REVERSE_BEST_LIMIT（对方最优剩余转限价）";
        case XTP_PRICE_LIMIT_OR_CANCEL:
            return "XTP_PRICE_LIMIT_OR_CANCEL（期权限价申报FOK）";
        case XTP_PRICE_TYPE_UNKNOWN:
            return "XTP_PRICE_TYPE_UNKNOWN（未知或无效）";
        default:
            return "未识别的价格类型";
    }
}
//}}}


//{{{ 自定义函数5 SideTypeToString(将 XTP_SIDE_TYPE 的枚举类型转化为字符串)
const char* SideTypeToString(XTP_SIDE_TYPE side) {
    switch (side) {
        case 1:   return "XTP_SIDE_BUY 买入";
        case 2:   return "XTP_SIDE_SELL 卖出";
        default:  return "未识别买卖方向";
    }
}
//}}}
