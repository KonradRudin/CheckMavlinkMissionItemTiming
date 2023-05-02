#include "MissionTester.h"

#include <string>

static const std::string UUID{"123456789abcdef1234567890abcdef"};

MissionTester::MissionTester():
_message_set("mavlink/development.xml"),
_client("10.41.1.1", 1369),
_runtime(_message_set,_client) {

    auto heartbeat = _message_set.create("HEARTBEAT")({{"type", _message_set.e("MAV_TYPE_GCS")},
                                           {"autopilot", _message_set.e("MAV_AUTOPILOT_GENERIC")},
                                           {"base_mode", 0},
                                           {"custom_mode", 0},
                                           {"system_status", _message_set.e("MAV_STATE_ACTIVE")},
                                           {"mavlink_version", 2}});

    _runtime.setHeartbeatMessage(heartbeat);

    _connection = _runtime.awaitConnection();
    _connection->addMessageCallback([this](const mav::Message& message) {_identificationMessageHandler(message);});
}

MissionTester::~MissionTester() {
    _connection->removeAllCallbacks();

    _runtime.stop();
    _client.stop();
}

void MissionTester::_identificationMessageHandler(const mav::Message& message) {
    auto message_id = message.id();
    if (message_id == _message_set.idForMessage("COMMAND_INT")) {
        auto cmd = message.get<int>("command");
        auto param1 = message.get<int>("param1");
        if (cmd == _message_set.enum_for("MAV_CMD_REQUEST_MESSAGE") &&
            param1 == _message_set.idForMessage("UNIQUE_IDENTIFIER")) {
            auto out_message = _message_set.create("UNIQUE_IDENTIFIER");
            out_message["uuid"] = UUID;

            _connection->send(out_message);
        }
    }
}

void MissionTester::_handleMessage(const mav::Message& message) {

}