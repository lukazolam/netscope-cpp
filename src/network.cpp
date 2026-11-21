
#include "network.h"

#include <sstream>
#include <string>
#include <cstdint>
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <functional>
#include <mutex>
#include <cstdio>
#include <regex>

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

std::vector<std::string> generateHostIPs(
    const std::string& ip,
    int prefix
) {
    std::vector<std::string> hosts;

    // Validate input
    if (!validateIPv4(ip) || prefix < 0 || prefix > 32) {
        return hosts;
    }

    // Get existing subnet information
    SubnetInfo subnet = calculateSubnet(ip, prefix);

    // Convert first and last usable addresses to integers
    std::uint32_t first = ipToInteger(subnet.firstHost);
    std::uint32_t last = ipToInteger(subnet.lastHost);

    // Generate each address in the usable range
    for (std::uint64_t address = first; address <= last; address++) {
        hosts.push_back(
            integerToIP(static_cast<std::uint32_t>(address))
        );
    }

    return hosts;
}


PortStatus scanTCPPort(const std::string& ip, int port) {
    // Reject invalid port numbers and IP addresses
    if (!validateIPv4(ip) || port < 1 || port > 65535) {
        return PortStatus::Error;
    }

    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        return PortStatus::Error;
    }

    // Configure the destination
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
        close(sock);
        return PortStatus::Error;
    }

    // Configure the socket as nonblocking
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(sock);
        return PortStatus::Error;
    }

    // Attempt the connection
    int result = connect(
        sock,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    );

    // Connection succeeded immediately
    if (result == 0) {
        close(sock);
        return PortStatus::Open;
    }

    // EINPROGRESS means the connection is still being established
    if (errno != EINPROGRESS) {
        int error = errno;
        close(sock);

        return error == ECONNREFUSED
            ? PortStatus::Closed
            : PortStatus::Error;
    }

    // Wait for the connection, with a two-second timeout
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval timeout{};
    timeout.tv_sec = 2;

    result = select(sock + 1, nullptr, &writeSet, nullptr, &timeout);

    if (result == 0) {
        close(sock);
        return PortStatus::Timeout;
    }

    if (result < 0) {
        close(sock);
        return PortStatus::Error;
    }

    // Check whether the connection succeeded
    int socketError = 0;
    socklen_t errorLength = sizeof(socketError);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR,
                   &socketError, &errorLength) == -1) {
        close(sock);
        return PortStatus::Error;
    }

    close(sock);

    if (socketError == 0) {
        return PortStatus::Open;
    }

    if (socketError == ECONNREFUSED) {
        return PortStatus::Closed;
    }

    if (socketError == ETIMEDOUT) {
        return PortStatus::Timeout;
    }

    return PortStatus::Error;
}


std::vector<PortScanResult> scanTCPPorts(
    const std::string& ip,
    const std::vector<int>& ports,
    const std::function<void(std::size_t, std::size_t)>& onProgress
) {
    if (ports.empty()) {
        return {};
    }

    const unsigned int maxThreads = 32;

    const unsigned int threadCount = std::min(
        maxThreads,
        static_cast<unsigned int>(ports.size())
    );

    std::vector<PortScanResult> results(
        ports.size(),
        {0, PortStatus::Error}
    );

    std::atomic<std::size_t> nextIndex{0};
    std::atomic<std::size_t> completed{0};

    std::mutex progressMutex;

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; i++) {
        workers.emplace_back([&]() {
            while (true) {
                std::size_t index = nextIndex.fetch_add(1);

                if (index >= ports.size()) {
                    break;
                }

                int port = ports[index];

                PortStatus status = scanTCPPort(ip, port);
                results[index] = {port, status};

                // Update completed port count
                std::size_t finished = completed.fetch_add(1) + 1;

                // Report progress safely
                if (onProgress) {
                    std::lock_guard<std::mutex> lock(progressMutex);
                    onProgress(finished, ports.size());
                }
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    return results;
}

bool isHostResponding(const std::string& ip) {
    // Common TCP ports to probe
    const std::vector<int> probePorts = {
        22, 80, 443, 445, 3389, 8080
    };

    for (int port : probePorts) {
        PortStatus status = scanTCPPort(ip, port);

        if (status == PortStatus::Open ||
            status == PortStatus::Closed) {
            return true;
        }
    }

    return false;
}

std::vector<DiscoveredHost> discoverHosts(
    const std::vector<std::string>& addresses,
    const std::function<void(std::size_t, std::size_t)>& onProgress
) {
    if (addresses.empty()) {
        return {};
    }

    const unsigned int maxThreads = 32;

    const unsigned int threadCount = std::min(
        maxThreads,
        static_cast<unsigned int>(addresses.size())
    );

    std::vector<DiscoveredHost> results(addresses.size());

    std::atomic<std::size_t> nextIndex{0};
    std::size_t completed = 0;
    std::mutex progressMutex;

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; i++) {
        workers.emplace_back([&]() {
            while (true) {
                std::size_t index = nextIndex.fetch_add(1);

                if (index >= addresses.size()) {
                    break;
                }

                const std::string& ip = addresses[index];

                bool responding = isHostResponding(ip);

                results[index] = {
                    ip,
                    responding,
                    "",
                    ""
                };
                
                // Update discovery progress
                {
                    std::lock_guard<std::mutex> lock(progressMutex);

                    completed++;

                    if (onProgress) {
                        onProgress(completed, addresses.size());
                    }
                }

            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    // Keep only responding devices
    std::vector<DiscoveredHost> discovered;

    for (const DiscoveredHost& host : results) {
        if (host.responding) {
            discovered.push_back(host);
        }
    }

    return discovered;
}

std::string getMACAddress(const std::string& ip) {
    // Only accept validated IPv4 addresses
    if (!validateIPv4(ip)) {
        return "";
    }

#ifdef __APPLE__
    // Query the macOS ARP table
    std::string command = "/usr/sbin/arp -n " + ip + " 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");

    if (pipe == nullptr) {
        return "";
    }

    char buffer[512];
    std::string output;

    // Read the command output
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    pclose(pipe);

    // Extract a six-byte hexadecimal MAC address
    std::regex macPattern(
        R"(\bat ((?:[0-9a-fA-F]{1,2}:){5}[0-9a-fA-F]{1,2})(?:\s|$))"
    );

    std::smatch match;

    if (std::regex_search(output, match, macPattern)) {
        return match[1].str();
    }
#endif

    return "";
}

