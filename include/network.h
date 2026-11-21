
#pragma once

#include <string>

bool validateIPv4(const std::string& ip);

struct SubnetInfo {
    std::string ipAddress;
    std::string subnetMask;
    std::string networkAddress;
    std::string broadcastAddress;
    std::string firstHost;
    std::string lastHost;

    unsigned long long usableHosts;
    int prefixLength;
};

SubnetInfo calculateSubnet(const std::string& ip, int prefix);


enum class PortStatus {
    Open,
    Closed,
    Timeout,
    Error
};

PortStatus scanTCPPort(const std::string& ip, int port);
