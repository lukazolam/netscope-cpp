
#include "network.h"

#include <sstream>
#include <string>
#include <cstdint>

bool validateIPv4(const std::string& ip) {
    std::stringstream ss(ip);
    std::string octet;
    int octetCount = 0;

    while (std::getline(ss, octet, '.')) {
        octetCount++;

        if (octet.empty() || octet.length() > 3) {
            return false;
        }

        if (octet.length() > 1 && octet[0] == '0') {
            return false;
        }

        for (char c : octet) {
            if (c < '0' || c > '9') {
                return false;
            }
        }

        int value = std::stoi(octet);

        if (value > 255) {
            return false;
        }
    }

    if (octetCount != 4 || ip.empty() || ip.back() == '.') {
        return false;
    }

    return true;
}

std::uint32_t ipToInteger(const std::string& ip) {
    std::stringstream ss(ip);
    std::string octet;

    std::uint32_t result = 0;

    while (std::getline(ss, octet, '.')) {
        result = (result << 8) | std::stoul(octet);
    }

    return result;
}

std::string integerToIP(std::uint32_t ip) {
    std::string result;

    for (int i = 3; i >= 0; i--) {
        int octet = (ip >> (i * 8)) & 0xFF;

        result += std::to_string(octet);

        if (i > 0) {
            result += ".";
        }
    }

    return result;
}

SubnetInfo calculateSubnet(const std::string& ip, int prefix) {
    SubnetInfo result{};

    result.ipAddress = ip;
    result.prefixLength = prefix;

    std::uint32_t ipNumber = ipToInteger(ip);
    std::uint32_t mask = (prefix == 0)
        ? 0
        : (0xFFFFFFFFu << (32 - prefix));

    std::uint32_t network = ipNumber & mask;
    std::uint32_t broadcast = network | ~mask;

    result.subnetMask = integerToIP(mask);
    result.networkAddress = integerToIP(network);
    result.broadcastAddress = integerToIP(broadcast);

    if (prefix == 32) {
        result.usableHosts = 1;
        result.firstHost = ip;
        result.lastHost = ip;
    }
    else if (prefix == 31) {
        result.usableHosts = 2;
        result.firstHost = integerToIP(network);
        result.lastHost = integerToIP(broadcast);
    }
    else {
        result.usableHosts = (1ULL << (32 - prefix)) - 2;

        result.firstHost = integerToIP(network + 1);
        result.lastHost = integerToIP(broadcast - 1);
    }


    return result;
}

