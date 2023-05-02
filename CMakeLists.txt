cmake_minimum_required(VERSION 3.20)
project(mission_tester)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

set(APP_SOURCES
        src/MissionTester.cpp)

add_executable(mission_tester src/main.cpp ${APP_SOURCES})
target_include_directories(mission_tester PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libmav/include)
target_link_libraries(mission_tester PRIVATE Threads::Threads)

# copy mavlink message definitions to the build directory
file(GLOB MAVLINK_XML ${CMAKE_CURRENT_SOURCE_DIR}/mavlink/message_definitions/v1.0/*.xml)
file(COPY ${MAVLINK_XML} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/mavlink)