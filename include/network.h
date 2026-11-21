
#pragma once

#include <string>
#include <vector>
#include <functional>

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

struct PortScanResult {
    int port;
    PortStatus status;
};

std::vector<PortScanResult> scanTCPPorts(
    const std::string& ip,
    const std::vector<int>& ports,
    const std::function<void(std::size_t, std::size_t)>& onProgress = {}
);

std::vector<std::string> generateHostIPs(
    const std::string& ip,
    int prefix
);

struct DiscoveredHost {
    std::string ipAddress;
    bool responding;
    std::string hostname;
    std::string macAddress;
};

std::vector<DiscoveredHost> discoverHosts(
    const std::vector<std::string>& addresses,
    const std::function<void(std::size_t, std::size_t)>& onProgress = {}
);

std::string resolveHostname(const std::string& ip);

std::string getMACAddress(const std::string& ip);

//w11 functions
bool initializeNetworking();
void cleanupNetworking();
