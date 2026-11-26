
cmake_minimum_required(VERSION 3.16)

project(NetScope VERSION 0.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(netscope
    src/main.cpp
    src/network.cpp
    src/systeminfo.cpp
    src/macvendor.cpp
)

target_include_directories(netscope PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)

target_link_libraries(netscope PRIVATE Threads::Threads)


if(WIN32)
    target_link_libraries(netscope PRIVATE
        ws2_32
        iphlpapi
    )
endif()
