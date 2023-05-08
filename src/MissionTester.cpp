#include "MissionTester.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "Util.h"

static const std::string UUID{"123456789abcdef1234567890abcdef"};
static const std::string IP_ADDR{"127.0.0.1"};

MissionTester::MissionTester():
_message_set("mavlink/development.xml"),
_client(IP_ADDR, 1369),
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
    _connection->addMessageCallback([this](const mav::Message& message){_handleMissionMessage(message);});
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

            _parameter_request_timer.setTimeout([this](){_sendParameterRequest();},2500);
            _mission_request_timer.setTimeout([this](){_sendMissionRequest();}, 3000);
        }
    }
}

void MissionTester::_sendMissionRequest() {
    auto message = _message_set.create("MISSION_REQUEST_LIST");
    message["target_system"] = 1;
    message["target_component"] = _message_set.enum_for("MAV_COMP_ID_AUTOPILOT1");

    std::cout << "Start requesting mission" << std::endl;
    _connection->send(message);
}

void MissionTester::_sendParameterRequest() {
    auto message = _message_set.create("PARAM_REQUEST_LIST");
    message["target_system"] = 1;
    message["target_component"] = _message_set.enum_for("MAV_COMP_ID_AUTOPILOT1");

    _connection->send(message);
}

void MissionTester::_handleMissionMessage(const mav::Message& message) {
    auto message_id = message.id();
    if(message_id == _message_set.idForMessage("MISSION_COUNT")) {
        std::cout << "got mission count"<< std::endl;
        // Reply by requesting first mission
        _mission_count = message.get<int>("count");
        if(_mission_count>0) {
            _sendMissionRequest(0);
        }
    } else if (message_id == _message_set.idForMessage("MISSION_ITEM_INT")) {
        auto seq = message.get<uint>("seq");
        auto arrival_time = std::chrono::system_clock::now();
        _mission_item_poll_time.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(arrival_time - _request_time));
        if (seq + 1 <_mission_count) {
            // ask for next one
            _sendMissionRequest(seq + 1);
        } else {
            auto out_message = _message_set.create("MISSION_ACK");
            out_message["target_system"] = message.header().systemId();
            out_message["target_component"] = _message_set.enum_for("MAV_COMP_ID_AUTOPILOT1");

            _connection->send(out_message);

            _printMetric();
            _mission_item_poll_time.clear();

            _parameter_request_timer.setTimeout([this](){_sendParameterRequest();},9500);
            _mission_request_timer.setTimeout([this](){_sendMissionRequest();}, 10000);
        }
    // } else if (message_id == _message_set.idForMessage("PARAM_VALUE")) {
    //     static size_t counter{0U};
    //     if (counter % 80 ==0) {
    //         std::cout << "Got parameter value" << std::endl;
    //     }
    //     counter++;
    }
}

void MissionTester::_sendMissionRequest(int mission_item) {
    auto out_message = _message_set.create("MISSION_REQUEST_INT");
    out_message["target_system"] = 1;
    out_message["target_component"] = _message_set.enum_for("MAV_COMP_ID_AUTOPILOT1");
    out_message["seq"] = mission_item;

    _request_time = std::chrono::system_clock::now();
    for (size_t i=0;i<1;i++) {
        _connection->send(out_message);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

}

void MissionTester::_printMetric() {
    if(_mission_item_poll_time.size() > 1u) {
        int64_t min{INT64_MAX};
        int64_t max{0U};
        int64_t sum{0};
        for(const auto& e : _mission_item_poll_time) {
            int64_t passed_time{e.count()};
            if(passed_time < min) {
                min = passed_time;
            }
            
            if(passed_time > max) {
                max = passed_time;
            }

            sum += passed_time;
        }

        if (max > _max_poll_time) {
            _max_poll_time = max;
        }

        std::cout << "Mission item statistics for " <<  _mission_item_poll_time.size() << " items:"
                  << "\n\t min: " << static_cast<int>(min) 
                  << "\n\t max: " <<  static_cast<int>(max)
                  << "\n\t mean: " <<  static_cast<double>(sum)/_mission_item_poll_time.size() << std::endl;

        std::cout << "Overall max: " << static_cast<int>(_max_poll_time) << std::endl;

    } else {
        std::cout << "Not enough elements, could not create statistics\n";
    }
}