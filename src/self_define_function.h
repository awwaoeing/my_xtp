#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "xtp_trader_api.h"

//自定义函数1 从外部文件中读入数据并存入vector中
std::vector<std::string> LoadTickersFromCSV(const std::string& filename);


//自定义函数2 将 XTP_ORDER_STATUS_TYPE 报单状态类型的枚举类型转化为字符串
const char* OrderStatusToString(XTP_ORDER_STATUS_TYPE status);

//自定义函数3 将 XTP_ORDER_SUBMIT_STATUS_TYPE 类型的枚举类型转化为字符串
const char* OrderSubmitStatusToString(XTP_ORDER_SUBMIT_STATUS_TYPE status);


//自定义函数4 将 XTP_PRICE_TYPE 类型的枚举类型转化为字符串
const char* PriceTypeToString(XTP_PRICE_TYPE price_type);


//自定义函数5 将 XTP_SIDE_TYPE 类型的枚举类型转化为字符串
const char* SideTypeToString(XTP_SIDE_TYPE side);

