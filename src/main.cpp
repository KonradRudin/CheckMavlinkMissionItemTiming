#include <chrono>
#include <thread>

#include "MissionTester.h"


int main(int argc, char* argv[])
{

    MissionTester tester;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}