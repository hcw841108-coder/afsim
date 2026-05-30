#include "PlatformDataCollector.hpp"
#include "UtEarth.hpp"
#include "UtMath.hpp"
#include "WsfEM_Antenna.hpp"

#include <iostream>
#include <sstream>
#include <regex>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <cmath>


// ================================================================
// 常量与工具函数
// ================================================================

static constexpr double kPi = 3.1415926535897932384626433832795;

template <typename T>
static std::string ToStdStringByStream(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

static std::string ToLowerCopy(std::string s)
{
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        }
    );

    return s;
}

static std::string GuessPlatformTypeFromName(const std::string& name)
{
    std::string n = ToLowerCopy(name);

    if (n.find("ship") != std::string::npos)
    {
        return "ship";
    }

    if (n.find("uav") != std::string::npos ||
        n.find("drone") != std::string::npos)
    {
        return "uav";
    }

    if (n.find("sat") != std::string::npos ||
        n.find("radar") != std::string::npos)
    {
        return "satellite";
    }

    if (n.find("air") != std::string::npos ||
        n.find("fighter") != std::string::npos ||
        n.find("plane") != std::string::npos)
    {
        return "aircraft";
    }

    return "platform";
}
static double DegToRad(double deg)
{
    // 你的原代码已经使用 UtMath::cDEG_PER_RAD，所以这里用它反算，避免依赖 cRAD_PER_DEG 是否存在
    return deg / UtMath::cDEG_PER_RAD;
}

static double RadToDeg(double rad)
{
    return rad * UtMath::cDEG_PER_RAD;
}

static double NormalizeRad0To2Pi(double rad)
{
    while (rad < 0.0)
    {
        rad += 2.0 * kPi;
    }

    while (rad >= 2.0 * kPi)
    {
        rad -= 2.0 * kPi;
    }

    return rad;
}

static std::string ToUpperCopy(std::string s)
{
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c));
        }
    );

    return s;
}

// 由当前经纬度指向目标经纬度，计算初始航向角，单位：rad
static double ComputeBearingRad(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
    double lat1 = DegToRad(lat1Deg);
    double lat2 = DegToRad(lat2Deg);
    double dLon = DegToRad(lon2Deg - lon1Deg);

    double y = std::sin(dLon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2)
        - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);

    double bearing = std::atan2(y, x);
    return NormalizeRad0To2Pi(bearing);
}


// ================================================================
// Python 智能体指令结构
// ================================================================

struct AgentActionRecord
{
    int platformId = -1;
    std::string action;

    std::string reason;

    bool hasHeading = false;
    double headingDeg = 0.0;

    bool hasMoveTarget = false;
    double targetLat = 0.0;
    double targetLon = 0.0;
    double targetAltM = 0.0;
    double speedMps = -1.0;

    int targetId = -1;
    std::string weapon;
};

// 平台 id -> 最新智能体命令
static std::map<int, AgentActionRecord> gLatestAgentCommands;

// 控制日志限频，避免每帧刷屏
static std::map<int, double> gLastCommandLogSimTime;


// ================================================================
// 简易 JSON 字段提取，不依赖第三方库
// ================================================================

static size_t FindMatchingBrace(const std::string& text, size_t startPos)
{
    if (startPos == std::string::npos || startPos >= text.size() || text[startPos] != '{')
    {
        return std::string::npos;
    }

    int depth = 0;
    bool inString = false;
    bool escape = false;

    for (size_t i = startPos; i < text.size(); ++i)
    {
        char c = text[i];

        if (inString)
        {
            if (escape)
            {
                escape = false;
            }
            else if (c == '\\')
            {
                escape = true;
            }
            else if (c == '"')
            {
                inString = false;
            }

            continue;
        }

        if (c == '"')
        {
            inString = true;
            continue;
        }

        if (c == '{')
        {
            depth++;
        }
        else if (c == '}')
        {
            depth--;
            if (depth == 0)
            {
                return i;
            }
        }
    }

    return std::string::npos;
}

static bool ExtractStringValue(
    const std::string& text,
    const std::string& key,
    std::string& value)
{
    try
    {
        std::string pattern =
            "\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"";

        std::regex re(pattern);
        std::smatch match;

        if (std::regex_search(text, match, re))
        {
            value = match[1].str();
            return true;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "[AgentParser] ExtractStringValue regex error: "
            << e.what() << std::endl;
    }

    return false;
}

static bool ExtractNumberValue(
    const std::string& text,
    const std::string& key,
    double& value)
{
    try
    {
        std::string pattern =
            "\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?)";

        std::regex re(pattern);
        std::smatch match;

        if (std::regex_search(text, match, re))
        {
            value = std::stod(match[1].str());
            return true;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "[AgentParser] ExtractNumberValue regex error: "
            << e.what() << std::endl;
    }

    return false;
}

static bool ExtractIntValue(
    const std::string& text,
    const std::string& key,
    int& value)
{
    double temp = 0.0;

    if (ExtractNumberValue(text, key, temp))
    {
        value = static_cast<int>(temp);
        return true;
    }

    return false;
}

static std::vector<AgentActionRecord> ParseAgentActionsByRegex(const std::string& jsonText)
{
    std::vector<AgentActionRecord> actions;

    try
    {
        // 适配 Python 当前格式：
        // {"id": 9, "action": "SET_HEADING", "parameters": {...}}
        std::regex actionRe(
            R"REGEX("id"\s*:\s*(-?\d+)\s*,\s*"action"\s*:\s*"([^"]+)")REGEX"
        );

        auto begin = std::sregex_iterator(jsonText.begin(), jsonText.end(), actionRe);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it)
        {
            AgentActionRecord rec{};

            rec.platformId = std::stoi((*it)[1].str());
            rec.action = (*it)[2].str();

            size_t matchPos = static_cast<size_t>(it->position());

            size_t objStart = jsonText.rfind('{', matchPos);
            size_t objEnd = FindMatchingBrace(jsonText, objStart);

            std::string objText;

            if (objStart != std::string::npos &&
                objEnd != std::string::npos &&
                objEnd > objStart)
            {
                objText = jsonText.substr(objStart, objEnd - objStart + 1);
            }
            else
            {
                objText = jsonText.substr(matchPos, std::min<size_t>(512, jsonText.size() - matchPos));
            }

            ExtractStringValue(objText, "reason", rec.reason);

            if (ExtractNumberValue(objText, "heading_deg", rec.headingDeg) ||
                ExtractNumberValue(objText, "heading", rec.headingDeg))
            {
                rec.hasHeading = true;
            }

            bool hasLat = ExtractNumberValue(objText, "lat", rec.targetLat);
            bool hasLon = ExtractNumberValue(objText, "lon", rec.targetLon);

            if (ExtractNumberValue(objText, "alt_m", rec.targetAltM) ||
                ExtractNumberValue(objText, "alt", rec.targetAltM))
            {
                // 已提取高度
            }

            ExtractNumberValue(objText, "speed_mps", rec.speedMps);

            if (hasLat && hasLon)
            {
                rec.hasMoveTarget = true;
            }

            ExtractIntValue(objText, "target_id", rec.targetId);
            ExtractStringValue(objText, "weapon", rec.weapon);

            actions.push_back(rec);
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "[AgentParser] ParseAgentActionsByRegex failed: "
            << e.what() << std::endl;
    }

    return actions;
}


// ================================================================
// 仿真平台查找与命令缓存
// ================================================================

static WsfPlatform* FindPlatformByIdInSimulation(WsfSimulation& sim, int platformId)
{
    size_t count = sim.GetPlatformCount();

    for (size_t i = 0; i < count; ++i)
    {
        WsfPlatform* platform = sim.GetPlatformEntry(i);

        if (!platform)
        {
            continue;
        }

        int id = static_cast<int>(platform->GetIndex());

        if (id == platformId)
        {
            return platform;
        }
    }

    return nullptr;
}

static void HandleAgentCommandTextOnly(
    const std::string& jsonText,
    double simTime,
    WsfSimulation& sim)
{
    bool isActionCommand =
        jsonText.find("action_command") != std::string::npos;

    bool isAgentAction =
        jsonText.find("agent_action") != std::string::npos;

    if (!isActionCommand && !isAgentAction)
    {
        std::cout << "[AgentCommand] Ignore message, not action_command / agent_action."
            << std::endl;
        return;
    }

    std::vector<AgentActionRecord> actions = ParseAgentActionsByRegex(jsonText);

    std::cout << "[AgentCommand] simTime = "
        << simTime
        << ", parsed actions = "
        << actions.size()
        << std::endl;

    for (const auto& action : actions)
    {
        WsfPlatform* platform = FindPlatformByIdInSimulation(sim, action.platformId);

        if (!platform)
        {
            std::cout << "[AgentCommand] Platform not found, id = "
                << action.platformId
                << ", action = "
                << action.action
                << std::endl;
            continue;
        }

        if (platform->IsBroken())
        {
            std::cout << "[AgentCommand] Platform is broken, ignored. id = "
                << action.platformId
                << ", name = "
                << platform->GetName()
                << std::endl;
            continue;
        }

        gLatestAgentCommands[action.platformId] = action;

        std::string actionName = ToUpperCopy(action.action);

        std::cout << "[AgentCommand] Cached action: platform_id = "
            << action.platformId
            << ", platform_name = "
            << platform->GetName()
            << ", action = "
            << actionName
            << std::endl;
    }
}


// ================================================================
// 将最新 Python 指令作用到当前 mover
// ================================================================

static void ApplyLatestCommandToMover(WsfMover* moverPtr, double simTime)
{
    if (!moverPtr)
    {
        return;
    }

    WsfPlatform* platform = moverPtr->GetPlatform();

    if (!platform)
    {
        return;
    }

    if (platform->IsBroken())
    {
        return;
    }

    int platformId = static_cast<int>(platform->GetIndex());

    auto it = gLatestAgentCommands.find(platformId);

    if (it == gLatestAgentCommands.end())
    {
        return;
    }

    const AgentActionRecord& cmd = it->second;
    std::string actionName = ToUpperCopy(cmd.action);

    bool shouldLog = false;
    auto logIt = gLastCommandLogSimTime.find(platformId);

    if (logIt == gLastCommandLogSimTime.end() || (simTime - logIt->second) >= 1.0)
    {
        shouldLog = true;
        gLastCommandLogSimTime[platformId] = simTime;
    }

    if (actionName == "HOLD")
    {
        if (shouldLog)
        {
            std::cout << "[AgentExecutor] HOLD platform = "
                << platform->GetName()
                << ", reason = "
                << cmd.reason
                << std::endl;
        }

        return;
    }

    if (actionName == "SET_HEADING" || actionName == "SETHEADING")
    {
        if (!cmd.hasHeading)
        {
            if (shouldLog)
            {
                std::cout << "[AgentExecutor] SET_HEADING missing heading_deg, platform = "
                    << platform->GetName()
                    << std::endl;
            }

            return;
        }

        double headingRad = DegToRad(cmd.headingDeg);
        headingRad = NormalizeRad0To2Pi(headingRad);

        moverPtr->SetHeading(headingRad);

        if (shouldLog)
        {
            std::cout << "[AgentExecutor] Apply SET_HEADING to platform = "
                << platform->GetName()
                << ", heading_deg = "
                << cmd.headingDeg
                << ", heading_rad = "
                << headingRad
                << ", simTime = "
                << simTime
                << std::endl;
        }

        return;
    }

    if (actionName == "MOVE")
    {
        if (!cmd.hasMoveTarget)
        {
            if (shouldLog)
            {
                std::cout << "[AgentExecutor] MOVE missing lat/lon, platform = "
                    << platform->GetName()
                    << std::endl;
            }

            return;
        }

        double curLat = 0.0;
        double curLon = 0.0;
        double curAlt = 0.0;
        platform->GetLocationLLA(curLat, curLon, curAlt);

        double bearingRad = ComputeBearingRad(
            curLat,
            curLon,
            cmd.targetLat,
            cmd.targetLon
        );

        moverPtr->SetHeading(bearingRad);

        if (shouldLog)
        {
            std::cout << "[AgentExecutor] Apply MOVE-as-heading to platform = "
                << platform->GetName()
                << ", target lat = "
                << cmd.targetLat
                << ", lon = "
                << cmd.targetLon
                << ", bearing_deg = "
                << RadToDeg(bearingRad)
                << ", simTime = "
                << simTime
                << std::endl;
        }

        return;
    }

    if (actionName == "FIRE" || actionName == "ATTACK")
    {
        if (shouldLog)
        {
            std::cout << "[AgentExecutor] FIRE received for platform = "
                << platform->GetName()
                << ", target_id = "
                << cmd.targetId
                << ", weapon = "
                << cmd.weapon
                << ". Weapon API is not connected yet."
                << std::endl;
        }

        return;
    }

    if (shouldLog)
    {
        std::cout << "[AgentExecutor] Unknown action = "
            << cmd.action
            << ", platform = "
            << platform->GetName()
            << std::endl;
    }
}


// ================================================================
// PlatformDataCollector 原有逻辑
// ================================================================

// ==================== 构造函数 ====================
PlatformDataCollector::PlatformDataCollector()
    : WsfSimulationExtension()
    , mSendInterval(1.0)
    , mNextSendTime(0.0)
    , mHasSentStartFrame(false)
    , mIsShuttingDown(false)
{
}

// ==================== 析构函数 ====================
PlatformDataCollector::~PlatformDataCollector()
{
    Shutdown();
}

// ==================== 加入仿真 ====================
void PlatformDataCollector::AddedToSimulation()
{
    std::cout << "[PlatformDataCollector] AddedToSimulation called." << std::endl;

    mMoverUpdatedCallback =
        WsfObserver::MoverUpdated(&GetSimulation()).Connect(
            &PlatformDataCollector::OnMoverUpdated,
            this
        );

    std::cout << "[PlatformDataCollector] MoverUpdated callback connected." << std::endl;
}

// ==================== 初始化 ====================
bool PlatformDataCollector::Initialize()
{
    std::cout << "[PlatformDataCollector] Initialize called." << std::endl;

    // AFSIM -> Python：发到 Python 监听端口 9000
    if (!mUdpSender.Initialize("127.0.0.1", 9000))
    {
        std::cout << "[PlatformDataCollector] UDP sender init failed." << std::endl;
        return false;
    }

    std::cout << "[PlatformDataCollector] UDP sender init success." << std::endl;

    // Python -> AFSIM：AFSIM 监听 9001
    if (!mUdpSender.InitializeReceiver(9001))
    {
        std::cout << "[PlatformDataCollector] UDP receiver init failed." << std::endl;
        return false;
    }

    std::cout << "[PlatformDataCollector] UDP receiver init success." << std::endl;
    return true;
}

// ==================== 平台初始化完成 ====================
bool PlatformDataCollector::PlatformsInitialized()
{
    std::cout << "[PlatformDataCollector] PlatformsInitialized called." << std::endl;
    std::cout << "[PlatformDataCollector] platform count after init = "
        << GetSimulation().GetPlatformCount()
        << std::endl;
    return true;
}

// ==================== 仿真开始 ====================
void PlatformDataCollector::Start()
{
    std::cout << "[PlatformDataCollector] Start called." << std::endl;

    double simTime = GetSimulation().GetSimTime();
    CollectAndSend(simTime);

    mHasSentStartFrame = true;
    mNextSendTime = simTime + mSendInterval;

    std::cout << "[PlatformDataCollector] next send time = "
        << mNextSendTime
        << std::endl;
}

// ==================== mover 更新回调 ====================
void PlatformDataCollector::OnMoverUpdated(double simTime, WsfMover* moverPtr)
{
    if (mIsShuttingDown)
    {
        return;
    }

    if (!mHasSentStartFrame)
    {
        return;
    }

    // 1. 接收 Python 智能体指令
    std::string agentData = mUdpSender.ReceiveAgentData();

    if (!agentData.empty())
    {
        std::cout << "[PlatformDataCollector] Received agent data at simTime = "
            << simTime
            << ": "
            << agentData
            << std::endl;

        HandleAgentCommandTextOnly(agentData, simTime, GetSimulation());
    }

    // 2. 将最新 Python 指令作用到当前 mover
    ApplyLatestCommandToMover(moverPtr, simTime);

    // 3. 周期性发送平台态势给 Python
    if (simTime < mNextSendTime)
    {
        return;
    }

    std::cout << "[PlatformDataCollector] OnMoverUpdated triggered at simTime = "
        << simTime
        << std::endl;

    CollectAndSend(simTime);

    mNextSendTime = simTime + mSendInterval;

    std::cout << "[PlatformDataCollector] next send time updated to "
        << mNextSendTime
        << std::endl;
}

// ==================== 采集并发送 ====================
void PlatformDataCollector::CollectAndSend(double simTime)
{
    mAllPlatformData.clear();

    WsfSimulation& sim = GetSimulation();
    size_t count = sim.GetPlatformCount();

    std::cout << "[PlatformDataCollector] CollectAndSend called, simTime = "
        << simTime
        << ", platform count = "
        << count
        << std::endl;

    for (size_t i = 0; i < count; ++i)
    {
        WsfPlatform* platform = sim.GetPlatformEntry(i);

        if (!platform)
        {
            continue;
        }

        PlatformData data{};

        if (ExtractSinglePlatformData(platform, data))
        {
            mAllPlatformData.push_back(data);
        }
    }



    if (!mAllPlatformData.empty())
    {
        //PrintCollectedData(mAllPlatformData, simTime);
        std::cout<<"数据已采集"<< std::endl;
        std::cout << "[PlatformDataCollector] collected count = "
            << mAllPlatformData.size()
            << std::endl;
    }
    else
    {
        std::cout << "[PlatformDataCollector] no platform data, send empty platform_state."
            << std::endl;
    }

    bool ok = mUdpSender.SendPlatformData(mAllPlatformData, simTime);

    std::cout << "[PlatformDataCollector] send result = "
        << ok
        << std::endl;
}

// ==================== 传感器最大范围 ====================
double PlatformDataCollector::GetSensorMaximumRange(WsfSensor* sensor)
{
    (void)sensor;
    return 0.0;
}

// ==================== 传感器提取 ====================
void PlatformDataCollector::ExtractSensorData(WsfPlatform* platform, std::vector<SensorData>& outSensors)
{
    outSensors.clear();

    if (!platform)
    {
        return;
    }

    unsigned int sensorCount = platform->GetComponentCount<WsfSensor>();

    for (unsigned int i = 0; i < sensorCount; ++i)
    {
        WsfSensor* sensor = platform->GetComponentEntry<WsfSensor>(i);

        if (!sensor)
        {
            continue;
        }

        SensorData sd{};

        sd.name = sensor->GetName();
        sd.type = sensor->GetType();

        sd.currentModeIndex = 0;
        sd.modeCount = sensor->GetModeCount();

        sd.maximumRange_m = 0.0;
        sd.hasTargetInRange = false;
        sd.targetCountInRange = 0;

        outSensors.push_back(sd);
    }
}

// ==================== 单个平台提取 ====================
bool PlatformDataCollector::ExtractSinglePlatformData(WsfPlatform* platform, PlatformData& outData)
{
    if (!platform)
    {
        return false;
    }

    outData.id = static_cast<int>(platform->GetIndex());

    outData.name = platform->GetName();
    outData.side = ToStdStringByStream(platform->GetSide());

    // 从 WsfPlatform 获取 platform_type / TypeId
    outData.platformType = ToStdStringByStream(platform->GetTypeId());

    if (outData.platformType.empty())
    {
        outData.platformType = GuessPlatformTypeFromName(outData.name);
    }

    // 位置
    platform->GetLocationLLA(outData.lat, outData.lon, outData.alt);

    // 姿态：弧度 -> 角度
    double heading_rad = 0.0;
    double pitch_rad = 0.0;
    double roll_rad = 0.0;

    platform->GetOrientationNED(heading_rad, pitch_rad, roll_rad);

    outData.heading = heading_rad * UtMath::cDEG_PER_RAD;
    outData.yaw = outData.heading;
    outData.pitch = pitch_rad * UtMath::cDEG_PER_RAD;
    outData.roll = roll_rad * UtMath::cDEG_PER_RAD;

    // 速度
    outData.speed_mps = platform->GetSpeed();

    // 加速度 ECS
    double aclECS[3] = { 0.0, 0.0, 0.0 };
    platform->GetAccelerationECS(aclECS);

    outData.accelX_ecs = aclECS[0];
    outData.accelY_ecs = aclECS[1];
    outData.accelZ_ecs = aclECS[2];

    outData.accelX_g = aclECS[0] / UtEarth::cACCEL_OF_GRAVITY;
    outData.accelY_g = aclECS[1] / UtEarth::cACCEL_OF_GRAVITY;
    outData.accelZ_g = aclECS[2] / UtEarth::cACCEL_OF_GRAVITY;

    // 传感器
    ExtractSensorData(platform, outData.sensors);

    outData.isBroken = platform->IsBroken();

    return true;
}

// ==================== 仿真结束 ====================
void PlatformDataCollector::Complete(double aSimTime)
{
    std::cout << "[PlatformDataCollector] Complete called, simTime = "
        << aSimTime
        << std::endl;

    std::cout << "[PlatformDataCollector] platform count at Complete = "
        << GetSimulation().GetPlatformCount()
        << std::endl;
}

// ==================== 清理 ====================
void PlatformDataCollector::Shutdown()
{
    if (mIsShuttingDown)
    {
        return;
    }

    mIsShuttingDown = true;

    std::cout << "[PlatformDataCollector] Shutdown called." << std::endl;

    if (mMoverUpdatedCallback)
    {
        mMoverUpdatedCallback->Disconnect();
        mMoverUpdatedCallback.reset();
    }

    mAllPlatformData.clear();
    mUdpSender.Shutdown();
}

// ==================== 获取全部数据 ====================
const std::vector<PlatformData>& PlatformDataCollector::GetAllPlatformData() const
{
    return mAllPlatformData;
}

// ==================== 打印 ====================
void PlatformDataCollector::PrintCollectedData(const std::vector<PlatformData>& data, double time)
{
    std::cout << "=== sim time: "
        << time
        << " ==="
        << std::endl;

    std::cout << "collected platforms: "
        << data.size()
        << std::endl;

    for (const auto& platform : data)
    {
        std::string status = platform.isBroken ? "[BROKEN]" : "[ALIVE]";

        std::cout << "  ID: "
            << platform.id
            << " | name: "
            << platform.name
            << " | side: "
            << platform.side
            << " | position: ("
            << platform.lat
            << ", "
            << platform.lon
            << ", "
            << platform.alt
            << ")"
            << " | heading: "
            << platform.heading
            << " deg"
            << " | speed: "
            << platform.speed_mps
            << " m/s"
            << " | sensor count: "
            << platform.sensors.size()
            << " "
            << status
            << std::endl;
    }

    std::cout << std::endl;
}