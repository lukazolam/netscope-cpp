
cmake_minimum_required(VERSION 3.16)

project(NetScope VERSION 0.10.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Shared networking engine
add_library(netscope-core STATIC
    src/network.cpp
    src/systeminfo.cpp
    src/macvendor.cpp
)

target_include_directories(netscope-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)

target_link_libraries(netscope-core PUBLIC
    Threads::Threads
)

if(WIN32)
    target_link_libraries(netscope-core PUBLIC
        ws2_32
        iphlpapi
    )
endif()

# Original command-line application
add_executable(netscope
    src/main.cpp
)

target_link_libraries(netscope PRIVATE
    netscope-core
)

# Optional Qt graphical application
option(NETSCOPE_BUILD_GUI "Build NetScope GUI" OFF)


if(NETSCOPE_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

    add_executable(netscope-gui
        gui/main.cpp

        gui/mainwindow.cpp
        gui/mainwindow.h

        gui/portscannerpage.cpp
        gui/portscannerpage.h

        gui/scanworker.cpp
        gui/scanworker.h
    )

    set_target_properties(netscope-gui PROPERTIES
        AUTOMOC ON
    )

    target_include_directories(netscope-gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
    )

    target_link_libraries(netscope-gui PRIVATE
        Qt6::Widgets
        Qt6::Core
        netscope-core
    )
endif()

