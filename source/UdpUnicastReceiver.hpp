#ifndef UDP_UNICAST_RECEIVER_HPP
#define UDP_UNICAST_RECEIVER_HPP

#include <boost/asio.hpp>
#include <vector>
#include <string>

#define BUFFER_SIZE 1024

// 使用命名空间简化代码（头文件中适度使用，避免污染全局命名空间）
namespace XDU
{
	using boost::asio::ip::udp;
	using ReceivedCallback = std::function<void(const char*, std::size_t, const udp::endpoint&)>;
	// 全局运行标志（声明），用于控制程序退出
	//extern bool g_running;

	// 信号处理函数声明（捕获 Ctrl+C）
	//void handle_signal(int signum);

	// UDP 单播接收类
	class UdpUnicastReceiver
	{
	public:
		UdpUnicastReceiver() = default;
		/**
		 * @brief 构造函数
		 * @param io_context IO 上下文引用
		 * @param port 要绑定的接收端口
		 */
		UdpUnicastReceiver(boost::asio::io_context& io_context, uint16_t port);

		// 禁止拷贝构造和赋值（避免 IO 资源重复管理）
		UdpUnicastReceiver(const UdpUnicastReceiver&) = delete;
		UdpUnicastReceiver& operator=(const UdpUnicastReceiver&) = delete;

		// 允许移动构造和赋值
		UdpUnicastReceiver(UdpUnicastReceiver&&) = default;
		UdpUnicastReceiver& operator=(UdpUnicastReceiver&&) = default;

		/**
		 * @brief 启动异步接收（非阻塞）
		 */
		void StartReceive();
		void SetReceivedCallback(const ReceivedCallback& aRecvCallback) { mReceivedCallback = aRecvCallback; }

	private:
		/**
		 * @brief 接收数据后的回调函数
		 * @param error 错误码（无错误则为空）
		 * @param bytes_transferred 接收到的字节数
		 */
		void HandleReceive(const boost::system::error_code& error,
			std::size_t bytes_transferred);

		// 成员变量
		boost::asio::io_context& io_context_;   // IO 上下文引用（核心调度）
		udp::socket mSocket;                    // UDP 套接字
		udp::endpoint mRemoteEndpoint;          // 发送方端点（单播源）
		std::vector<char> mRecvbuffer;          // 接收缓冲区（动态大小更安全）

		ReceivedCallback mReceivedCallback;
	};

} // namespace udp_communication

#endif // UDP_UNICAST_RECEIVER_HPP