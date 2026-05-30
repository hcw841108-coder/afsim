#include "UdpJsonSender.hpp"
#include <sstream>
#include <iostream>
#include "PlatformDataCollector.hpp"
#include <iomanip>
#include <cstdint>
#include <cctype>
// ==================== 构造 ====================
UdpJsonSender::UdpJsonSender()
    : mSocket(INVALID_SOCKET)
    , mInitialized(false)
    , mRecvSocket(INVALID_SOCKET)
    , mRecvInitialized(false)
    , mSeq(0)
{
}

// ==================== 析构 ====================
UdpJsonSender::~UdpJsonSender()
{
    Shutdown();
}

static std::string EscapeJsonString(const std::string& s)
{
    std::ostringstream oss;

    for (unsigned char c : s)
    {
        switch (c)
        {
        case '\"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            oss << static_cast<char>(c);
            break;
        }
    }

    return oss.str();
}

// ==================== 初始化 ====================
bool UdpJsonSender::Initialize(const std::string& ip, int port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "[UdpJsonSender] WSAStartup failed." << std::endl;
        return false;
    }

    mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mSocket == INVALID_SOCKET)
    {
        std::cout << "[UdpJsonSender] socket creation failed." << std::endl;
        WSACleanup();
        return false;
    }

    mTargetAddr = {};
    mTargetAddr.sin_family = AF_INET;
    mTargetAddr.sin_port = htons(port);

    int ret = inet_pton(AF_INET, ip.c_str(), &mTargetAddr.sin_addr);
    if (ret != 1)
    {
        std::cout << "[UdpJsonSender] inet_pton failed." << std::endl;
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    mInitialized = true;
    std::cout << "[UdpJsonSender] initialized to " << ip << ":" << port << std::endl;
    return true;
}

// ==================== JSON构造 ====================
std::string UdpJsonSender::BuildJson(const std::vector<PlatformData>& data, double simTime)
{
    static std::uint64_t sSeq = 0;

    std::ostringstream oss;
    oss << std::setprecision(15);

    std::uint64_t seq = sSeq++;

    oss << "{";

    // ==================== 顶层消息头 ====================
    // msg_type 是当前 Python UI 正在读取的字段
    // type 是兼容后续标准协议用的字段
    oss << "\"msg_type\":\"platform_state\",";
    oss << "\"type\":\"platform_state\",";
    oss << "\"seq\":" << seq << ",";
    oss << "\"source\":\"afsim\",";
    oss << "\"time\":" << simTime << ",";
    oss << "\"timestamp\":" << simTime << ",";
    oss << "\"platform_count\":" << data.size() << ",";

    // ==================== 平台数组 ====================
    oss << "\"platforms\":[";

    for (size_t i = 0; i < data.size(); ++i)
    {
        const auto& p = data[i];

        std::string side = p.side;
        for (auto& c : side)
        {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        oss << "{";

        oss << "\"id\":" << p.id << ",";
        oss << "\"name\":\"" << EscapeJsonString(p.name) << "\",";
        oss << "\"side\":\"" << EscapeJsonString(side) << "\",";

        // Python UI 表格有 Type 列
        std::string platformType = p.platformType.empty() ? "platform" : p.platformType;

        oss << "\"type\":\"" << EscapeJsonString(platformType) << "\",";
        oss << "\"platform_type\":\"" << EscapeJsonString(platformType) << "\",";

        // 位置
        oss << "\"lat\":" << p.lat << ",";
        oss << "\"lon\":" << p.lon << ",";

        // 同时发送 alt 和 alt_m
        // 你的 Python UI 当前用 alt_m 计算 Alt(ft)
        oss << "\"alt\":" << p.alt << ",";
        oss << "\"alt_m\":" << p.alt << ",";

        // 姿态，单位：deg
        oss << "\"heading\":" << p.heading << ",";
        oss << "\"yaw\":" << p.yaw << ",";
        oss << "\"pitch\":" << p.pitch << ",";
        oss << "\"roll\":" << p.roll << ",";

        // 速度
        oss << "\"speed_mps\":" << p.speed_mps << ",";

        // 加速度 ECS
        oss << "\"accelX_ecs\":" << p.accelX_ecs << ",";
        oss << "\"accelY_ecs\":" << p.accelY_ecs << ",";
        oss << "\"accelZ_ecs\":" << p.accelZ_ecs << ",";

        // 加速度 g
        oss << "\"accelX_g\":" << p.accelX_g << ",";
        oss << "\"accelY_g\":" << p.accelY_g << ",";
        oss << "\"accelZ_g\":" << p.accelZ_g << ",";

        // UI 表格可能会读取这些字段，没有就给默认值
        oss << "\"damage\":0,";
        oss << "\"weapon_count\":0,";

        // 状态
        oss << "\"alive\":" << (p.isBroken ? "false" : "true") << ",";
        oss << "\"status\":\"" << (p.isBroken ? "DEAD" : "ALIVE") << "\"";

        oss << "}";

        if (i + 1 < data.size())
        {
            oss << ",";
        }
    }

    oss << "]";

    oss << "}";

    return oss.str();
}

// ==================== 发送 ====================
bool UdpJsonSender::SendPlatformData(const std::vector<PlatformData>& data, double simTime)
{
    if (!mInitialized)
    {
        std::cout << "[UdpJsonSender] not initialized" << std::endl;
        return false;
    }

    std::string json = BuildJson(data, simTime);

    int ret = sendto(
        mSocket,
        json.c_str(),
        static_cast<int>(json.size()),
        0,
        reinterpret_cast<sockaddr*>(&mTargetAddr),
        sizeof(mTargetAddr)
    );

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        std::cout << "[UdpJsonSender] sendto failed, err = " << err << std::endl;
        return false;
    }

    std::cout << "[UdpJsonSender] sent bytes = " << ret << std::endl;
    std::cout << "[UdpJsonSender] json = " << json << std::endl;
    return true;
}

// ==================== 初始化接收器 ====================
bool UdpJsonSender::InitializeReceiver(int recvPort)
{
    if (mRecvSocket != INVALID_SOCKET)
    {
        std::cout << "[UdpJsonSender] Receiver already initialized." << std::endl;
        return true;
    }

    mRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mRecvSocket == INVALID_SOCKET)
    {
        std::cout << "[UdpJsonSender] Receiver socket creation failed." << std::endl;
        return false;
    }

    // 设置套接字为非阻塞模式
    unsigned long iMode = 1;
    if (ioctlsocket(mRecvSocket, FIONBIO, &iMode) != NO_ERROR)
    {
        std::cout << "[UdpJsonSender] Setting non-blocking mode failed." << std::endl;
        closesocket(mRecvSocket);
        mRecvSocket = INVALID_SOCKET;
        return false;
    }

    mRecvAddr.sin_family = AF_INET;
    mRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mRecvAddr.sin_port = htons(recvPort);

    if (bind(mRecvSocket, (sockaddr*)&mRecvAddr, sizeof(mRecvAddr)) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        std::cout << "[UdpJsonSender] bind failed on port " << recvPort << ", err = " << err << std::endl;
        closesocket(mRecvSocket);
        mRecvSocket = INVALID_SOCKET;
        return false;
    }

    mRecvInitialized = true;
    std::cout << "[UdpJsonSender] Receiver initialized on port " << recvPort << std::endl;
    return true;
}

// ==================== 接收智能体数据 ====================
static std::string ExtractJsonStringField(const std::string& json, const std::string& key)
{
    std::string pattern = "\"" + key + "\"";
    size_t keyPos = json.find(pattern);
    if (keyPos == std::string::npos)
    {
        return "";
    }

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos)
    {
        return "";
    }

    size_t firstQuote = json.find("\"", colonPos + 1);
    if (firstQuote == std::string::npos)
    {
        return "";
    }

    size_t secondQuote = json.find("\"", firstQuote + 1);
    if (secondQuote == std::string::npos)
    {
        return "";
    }

    return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

static std::string ExtractJsonNumberField(const std::string& json, const std::string& key)
{
    std::string pattern = "\"" + key + "\"";
    size_t keyPos = json.find(pattern);
    if (keyPos == std::string::npos)
    {
        return "";
    }

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos)
    {
        return "";
    }

    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart])))
    {
        valueStart++;
    }

    size_t valueEnd = valueStart;
    while (valueEnd < json.size())
    {
        char c = json[valueEnd];
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.')
        {
            valueEnd++;
        }
        else
        {
            break;
        }
    }

    if (valueEnd <= valueStart)
    {
        return "";
    }

    return json.substr(valueStart, valueEnd - valueStart);
}
std::string UdpJsonSender::ReceiveAgentData()
{
    if (!mRecvInitialized || mRecvSocket == INVALID_SOCKET)
    {
        return "";
    }

    char buffer[4096] = { 0 };
    sockaddr_in senderAddr{};
    int senderAddrLen = sizeof(senderAddr);

    int ret = recvfrom(
        mRecvSocket,
        buffer,
        sizeof(buffer) - 1,
        0,
        reinterpret_cast<sockaddr*>(&senderAddr),
        &senderAddrLen
    );

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();

        // 非阻塞 socket 没收到数据是正常情况
        if (err != WSAEWOULDBLOCK)
        {
            std::cout << "[UdpJsonSender] recvfrom failed, err = " << err << std::endl;
        }

        return "";
    }

    if (ret <= 0)
    {
        return "";
    }

    buffer[ret] = '\0';
    std::string receivedData(buffer);

    char senderIP[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, INET_ADDRSTRLEN);
    int senderPort = ntohs(senderAddr.sin_port);

    std::string msgType = ExtractJsonStringField(receivedData, "type");
    std::string seq = ExtractJsonNumberField(receivedData, "seq");

    if (msgType.empty())
    {
        msgType = "?";
    }

    if (seq.empty())
    {
        seq = "?";
    }

    std::cout << "[UdpJsonSender] Received UDP from "
        << senderIP << ":" << senderPort
        << " | type = " << msgType
        << " | seq = " << seq
        << " | size = " << ret << " bytes"
        << std::endl;

    std::cout << "[UdpJsonSender] Received JSON = " << receivedData << std::endl;

    return receivedData;
}

// ==================== 关闭 ====================
void UdpJsonSender::Shutdown()
{
    if (mSocket != INVALID_SOCKET)
    {
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
    }

    if (mRecvSocket != INVALID_SOCKET)
    {
        closesocket(mRecvSocket);
        mRecvSocket = INVALID_SOCKET;
    }

    if (mInitialized || mRecvInitialized)
    {
        WSACleanup();
        mInitialized = false;
        mRecvInitialized = false;
    }
}