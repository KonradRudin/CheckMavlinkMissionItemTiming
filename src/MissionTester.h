#ifndef MISSIONTESTER
#define MISSIONTESTER

#include <mav/Connection.h>
#include <mav/Message.h>
#include <mav/MessageSet.h>
#include <mav/Network.h>
#include <mav/UDPClient.h>

class MissionTester {
private:
    mav::MessageSet _message_set;
    mav::UDPClient _client;
    mav::NetworkRuntime _runtime;
    std::shared_ptr<mav::Connection> _connection;

    void _handleMessage(const mav::Message& message);

public:
    MissionTester();
    ~MissionTester();
};

#endif //MISSIONTESTER