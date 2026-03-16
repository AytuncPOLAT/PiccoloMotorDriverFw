cmake_minimum_required(VERSION 3.10)
project(SerialTester)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)
include_directories(../AppLayer/Common)

add_executable(SerialTester
    main.cpp
    Crc16.cpp
)