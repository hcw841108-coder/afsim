#include "UdpUnicastReceiver.hpp"
#include <iostream>
#include <signal.h>
#include <boost/bind/bind.hpp>

// 全局运行标志（定义）
//bool g_running = true;

// 信号处理函数实现（捕获 Ctrl+C 优雅退出）
//void XDU::handle_signal(int signum)
//{
//	if (signum == SIGINT) {
//		std::cout << "\n[INFO] 接收到退出信号 (Ctrl+C)，正在停止接收..." << std::endl;
//		g_running = false;
//	}
//}

// 构造函数实现
XDU::UdpUnicastReceiver::UdpUnicastReceiver(boost::asio::io_context& io_context, uint16_t port)
	: io_context_(io_context)
	, mSocket(io_context, udp::endpoint(udp::v4(), port))
	, mRecvbuffer(BUFFER_SIZE) // 初始化缓冲区大小为 1024 字节
{
	// 注册信号处理
	//signal(SIGINT, handle_signal);

	// 打印启动信息
	std::cout << "[INFO] UDP 单播接收器已启动，绑定端口: " << port << std::endl;
	//std::cout << "[INFO] 按 Ctrl+C 退出程序..." << std::endl;

	// 启动首次异步接收
	StartReceive();
}

// 启动异步接收实现
void XDU::UdpUnicastReceiver::StartReceive()
{
	//mRecvbuffer.resize(BUFFER_SIZE);
	// 清空缓冲区（可选，防止旧数据残留）
	std::fill(mRecvbuffer.begin(), mRecvbuffer.end(), 0);

	//size_t datagramSize = mSocket.receive_from(
	//	boost::asio::buffer(mRecvbuffer, 1),  // 仅需1字节缓冲区
	//	mRemoteEndpoint,
	//	boost::asio::socket_base::message_peek); // peek标志：只看数据，不读取

	//// 第二步：动态调整缓冲区大小
	//if (datagramSize > mRecvbuffer.size()) 
	//{
	//	mRecvbuffer.resize(datagramSize);
	//	std::cout << "[DEBUG] 动态调整缓冲区到: " << datagramSize << " 字节" << std::endl;
	//}

	// 异步接收数据：非阻塞，数据到达后调用 HandleReceive 回调
	mSocket.async_receive_from(
		boost::asio::buffer(mRecvbuffer),
		mRemoteEndpoint,
		boost::bind(&UdpUnicastReceiver::HandleReceive,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

// 接收回调函数实现
void XDU::UdpUnicastReceiver::HandleReceive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	//// 程序已标记退出，关闭套接字并返回
	//if (!g_running) {
	//	if (mSocket.is_open()) {
	//		mSocket.close();
	//	}
	//	return;
	//}

	// 处理接收结果
	if (!error)
	{
		// 解析接收到的单播数据
		std::string received_data(mRecvbuffer.data(), bytes_transferred);
		std::cout << "[RECV] 来自 " << mRemoteEndpoint.address() << ":"
			<< mRemoteEndpoint.port() << " 的数据: " << received_data << std::endl;
		char* tempBuffer = new char[bytes_transferred];
		memcpy(tempBuffer, mRecvbuffer.data(), bytes_transferred);
		mReceivedCallback(tempBuffer, bytes_transferred, mRemoteEndpoint);

		std::size_t sent_bytes = mSocket.send_to(boost::asio::buffer(mRecvbuffer, bytes_transferred), mRemoteEndpoint);

		// 重启异步接收（异步接收是一次性的，需手动重启）
		StartReceive();
	}
	else
	{
		// 处理接收错误（非退出信号则重启接收）
		std::cerr << "[ERROR] 接收失败: " << error.message() << std::endl;
		//if (g_running) {
		//	StartReceive();
		//}
	}
}