#ifndef MISSIONTESTER
#define MISSIONTESTER

#include <chrono>
#include <vector>

#include <mav/Connection.h>
#include <mav/Message.h>
#include <mav/MessageSet.h>
#include <mav/Network.h>
#include <mav/UDPClient.h>

#include "Util.h"

class MissionTester {
private:
    mav::MessageSet _message_set;
    mav::UDPClient _client;
    mav::NetworkRuntime _runtime;
    std::shared_ptr<mav::Connection> _connection;
    Timer _mission_request_timer;
    Timer _parameter_request_timer;
    uint _mission_count{0};
    std::vector<std::chrono::milliseconds> _mission_item_poll_time;
    int64_t _max_poll_time{0};
    std::chrono::system_clock::time_point _request_time;

    void _handleMissionMessage(const mav::Message& message);
    void _identificationMessageHandler(const mav::Message& message);
    void _sendMissionRequest();
    void _sendParameterRequest();
    void _printMetric();
    void _sendMissionRequest(int mission_item);

public:
    MissionTester();
    ~MissionTester();
};

#endif //MISSIONTESTER