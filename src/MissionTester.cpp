#include "MissionTester.h"

MissionTester::MissionTester():
_message_set("mavlink/development.xml"),
_client("10.41.1.1", 1369),
_runtime(_message_set,_client) {
    _connection = _runtime.awaitConnection();
}

MissionTester::~MissionTester() {
    _connection->removeAllCallbacks();

    _runtime.stop();
    _client.stop();
}

void MissionTester::_handleMessage(const mav::Message& message) {

}