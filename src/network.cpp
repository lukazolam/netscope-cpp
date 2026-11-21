
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

SubnetInfo calculateSubnet(const std::string& ip, int prefix) {
    SubnetInfo result{};

    result.ipAddress = ip;
    result.prefixLength = prefix;

    return result;
}

