
#pragma once

#include <string>
#include <vector>

struct InterfaceAddress {
    std::string address;
    int family;                 // 4 = IPv4, 6 = IPv6
    int prefixLength;
    std::string subnetMask;    // Used for IPv4
};

struct NetworkInterface {
    std::string name;
    std::string macAddress;
    bool isUp = false;

    std::vector<InterfaceAddress> addresses;
};

std::vector<NetworkInterface> getNetworkInterfaces();
