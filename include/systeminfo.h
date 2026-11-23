
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

    // Additional interface information
    unsigned int mtu = 0;
    std::string defaultGateway;
    std::vector<std::string> dnsServers;

    std::vector<InterfaceAddress> addresses;
};


std::vector<NetworkInterface> getNetworkInterfaces();
std::vector<std::string> getSystemDNSServers();
