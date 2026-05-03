cmake_minimum_required(VERSION 3.10)
project(xnet-bmd-capture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(xnet-bmd-capture
    src/config_parser.cpp
    src/socket.cpp
    src/capture.cpp
    src/main.cpp
)

target_link_libraries(xnet-bmd-capture)
