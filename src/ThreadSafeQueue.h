#pragma once
#include <iomanip> 
#include <string> 
#include <iostream> 
#include <unistd.h> 
#include <cstring> 
#include <map> 
#include <vector> 
#include <queue> 
#include <mutex>              // 互斥锁 
#include <condition_variable> // 条件变量

using namespace std;


//{{{ --- ThreadSafeQueue (线程安全队列) 模版 ---
template <typename T>
class ThreadSafeQueue {
	public:
		ThreadSafeQueue() = default;
		~ThreadSafeQueue() {
			shutdown(); // 析构时确保通知等待的线程
		}

		// 向队列中添加元素
		void push(T value) {
			lock_guard<mutex> lock(mtx_); // 加锁以保护队列
			queue_.push(move(value));         // 使用 move 提高效率
			cv_.notify_one();                      // 通知一个等待的线程有新数据了
		}

		// 从队列中取出元素 (阻塞式)
		bool pop(T& value) {
			unique_lock<mutex> lock(mtx_); // 加锁
										   // 等待直到队列不为空或收到关闭信号
			cv_.wait(lock, [this]() { return !queue_.empty() || shutdown_flag_; });

			if (shutdown_flag_ && queue_.empty()) {
				return false; // 已发出关闭信号且队列为空，返回 false
							  // 即使shutdown_flag_是true，只要队列还有数据bool pop()就不会返回false,能继续取数据只到队列为空
			}
			
			value = move(queue_.front()); // 取出队首元素
			queue_.pop();
			return true;
		}

		// MyQuoteSpi析构时发出关闭信号,共享队列.shutdown()
		void shutdown() {
			lock_guard<mutex> lock(mtx_);
			shutdown_flag_ = true;
			cv_.notify_all(); // 通知所有等待的线程
		}

		// 检查是否正在关闭
		bool is_shutting_down() {
			lock_guard<mutex> lock(mtx_);
			return shutdown_flag_;
		}

	private:
		queue<T> queue_;                // 底层队列
		mutex mtx_;                     // 互斥锁
		condition_variable cv_;         // 条件变量
		bool shutdown_flag_ = false;         // 关闭标志
};
//}}}
