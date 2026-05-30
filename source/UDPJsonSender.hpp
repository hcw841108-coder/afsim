#ifndef UDP_JSON_SENDER_HPP
#define UDP_JSON_SENDER_HPP

#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 前向声明（避免头文件耦合）
struct PlatformData;

class UdpJsonSender {
public:
    UdpJsonSender();
    ~UdpJsonSender();

    // 初始化发送UDP (9000端口)
    bool Initialize(const std::string& ip, int port);

    // 初始化接收UDP (9001端口)
    bool InitializeReceiver(int recvPort);

    // 发送平台数据（JSON格式）
    bool SendPlatformData(const std::vector<PlatformData>& data, double simTime);

    // 接收来自Python智能体的数据
    std::string ReceiveAgentData();

    // 关闭
    void Shutdown();

private:
    std::string BuildJson(const std::vector<PlatformData>& data, double simTime);

private:
    // 发送套接字
    SOCKET mSocket;
    sockaddr_in mTargetAddr;
    bool mInitialized;
    std::uint64_t mSeq;
    // 接收套接字
    SOCKET mRecvSocket;
    sockaddr_in mRecvAddr;
    bool mRecvInitialized;
};

#endif