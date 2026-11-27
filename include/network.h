
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "settings.h"

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

PortStatus scanTCPPort(
    const std::string& ip,
    int port,
    const AppSettings& settings
);

struct PortScanResult {
    int port;
    PortStatus status;
};

std::vector<PortScanResult> scanTCPPorts(
    const std::string& ip,
    const std::vector<int>& ports,
    const AppSettings& settings,
    const std::function<void(
        std::size_t, std::size_t
    )>& onProgress = {},
    const std::function<bool()>& shouldStop = {}
);

std::vector<std::string> generateHostIPs(
    const std::string& ip,
    int prefix
);


struct DiscoveredHost {
    std::string ipAddress;
    bool responding = false;
    std::string hostname;
    std::string macAddress;

    // Discovery method results
    bool icmpResponded = false;
    bool arpFound = false;
    bool tcpResponded = false;

    // macOS cache-only ARP results may be stale
    bool arpConfirmed = false;
};


std::vector<DiscoveredHost> discoverHosts(
    const std::vector<std::string>& addresses,
    const AppSettings& settings,
    const std::function<void(std::size_t, std::size_t)>& onProgress = {}
);

std::string resolveHostname(const std::string& ip);

std::string getMACAddress(const std::string& ip);

bool pingHost(
    const std::string& ip,
    const AppSettings& settings
);

//w11 functions
bool initializeNetworking();
void cleanupNetworking();

