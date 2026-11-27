
#include "network.h"

#include <sstream>
#include <string>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <functional>
#include <mutex>
#include <cstdio>
#include <regex>
#include <cerrno>
#include <cstdint>
#include <iomanip>
#include <cstdlib>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <netdb.h>
#endif


bool initializeNetworking() {
#ifdef _WIN32
    WSADATA data{};

    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return true;
#endif
}

void cleanupNetworking() {
#ifdef _WIN32
    WSACleanup();
#endif
}


#ifdef _WIN32
using NetSocket = SOCKET;
#else
using NetSocket = int;
#endif

void closeNetworkSocket(NetSocket sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

bool setNonBlocking(NetSocket sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);

    return flags != -1 &&
           fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

int getSocketError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

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

PortStatus scanTCPPort(
    const std::string& ip,
    int port,
    const AppSettings& settings
) {
    if (!validateIPv4(ip) || port < 1 || port > 65535) {
        return PortStatus::Error;
    }

    NetSocket sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        return PortStatus::Error;
    }
#else
    if (sock < 0) {
        return PortStatus::Error;
    }
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));

#ifdef _WIN32
    int converted = InetPtonA(
        AF_INET, ip.c_str(), &address.sin_addr
    );
#else
    int converted = inet_pton(
        AF_INET, ip.c_str(), &address.sin_addr
    );
#endif

    if (converted != 1) {
        closeNetworkSocket(sock);
        return PortStatus::Error;
    }

    if (!setNonBlocking(sock)) {
        closeNetworkSocket(sock);
        return PortStatus::Error;
    }

    int result = connect(
        sock,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    );

    if (result == 0) {
        closeNetworkSocket(sock);
        return PortStatus::Open;
    }

    int error = getSocketError();

#ifdef _WIN32
    bool inProgress =
        error == WSAEWOULDBLOCK ||
        error == WSAEINPROGRESS;

    bool refused = error == WSAECONNREFUSED;
#else
    bool inProgress = error == EINPROGRESS;
    bool refused = error == ECONNREFUSED;
#endif

    if (!inProgress) {
        closeNetworkSocket(sock);

        return refused
            ? PortStatus::Closed
            : PortStatus::Error;
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval timeout{};
    timeout.tv_sec = settings.tcpTimeoutMs / 1000;
    timeout.tv_usec = (settings.tcpTimeoutMs % 1000) * 1000;

#ifdef _WIN32
    result = select(
        0, nullptr, &writeSet, nullptr, &timeout
    );
#else
    result = select(
        sock + 1, nullptr, &writeSet, nullptr, &timeout
    );
#endif

    if (result == 0) {
        closeNetworkSocket(sock);
        return PortStatus::Timeout;
    }

    if (result < 0) {
        closeNetworkSocket(sock);
        return PortStatus::Error;
    }

    int socketError = 0;

#ifdef _WIN32
    int errorLength = sizeof(socketError);
    char* errorBuffer = reinterpret_cast<char*>(&socketError);
#else
    socklen_t errorLength = sizeof(socketError);
    void* errorBuffer = &socketError;
#endif

    if (getsockopt(
            sock,
            SOL_SOCKET,
            SO_ERROR,
            errorBuffer,
            &errorLength
        ) != 0) {
        closeNetworkSocket(sock);
        return PortStatus::Error;
    }

    closeNetworkSocket(sock);

    if (socketError == 0) {
        return PortStatus::Open;
    }

#ifdef _WIN32
    if (socketError == WSAECONNREFUSED) {
        return PortStatus::Closed;
    }

    if (socketError == WSAETIMEDOUT) {
        return PortStatus::Timeout;
    }
#else
    if (socketError == ECONNREFUSED) {
        return PortStatus::Closed;
    }

    if (socketError == ETIMEDOUT) {
        return PortStatus::Timeout;
    }
#endif

    return PortStatus::Error;
}

std::vector<PortScanResult> scanTCPPorts(
    const std::string& ip,
    const std::vector<int>& ports,
    const AppSettings& settings,
    const std::function<void(std::size_t, std::size_t)>& onProgress,
    const std::function<bool()>& shouldStop
) {
    if (ports.empty()) {
        return {};
    }

    const unsigned int maxThreads = settings.maxThreads;
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
                // Stop before starting another TCP connection
                if (shouldStop && shouldStop()) {
                    break;
                }
                int port = ports[index];

                PortStatus status = scanTCPPort(ip, port, settings);
                results[index] = {port, status};

                
                // Update completed port count
                std::size_t finished = completed.fetch_add(1) + 1;

                // Calculate an update interval for large scans
                const std::size_t progressStep =
                    std::max<std::size_t>(1, ports.size() / 100);

                // Only report progress at selected intervals
                if (onProgress &&
                    (finished % progressStep == 0 ||
                    finished == ports.size())) {

                    std::lock_guard<std::mutex> lock(progressMutex);
                    onProgress(finished, ports.size());
                }
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    // Remove entries that were never scanned
    results.erase(
        std::remove_if(
            results.begin(),
            results.end(),
            [](const PortScanResult& result) {
                return result.port == 0;
            }
        ),
        results.end()
    );

    return results;
}

bool isHostResponding(
    const std::string& ip,
    const AppSettings& settings
) {
    // Common TCP ports to probe
    const std::vector<int> probePorts = {
        22, 80, 443, 445, 3389, 8080
    };

    for (int port : probePorts) {
        PortStatus status = scanTCPPort(ip, port, settings);

        if (status == PortStatus::Open ||
            status == PortStatus::Closed) {
            return true;
        }
    }

    return false;
}

bool pingHost(
    const std::string& ip,
    const AppSettings& settings
) {
    if (!validateIPv4(ip)) {
        return false;
    }

#ifdef _WIN32
    // Convert the IP address
    in_addr address{};

    if (InetPtonA(AF_INET, ip.c_str(), &address) != 1) {
        return false;
    }

    // Create an ICMP handle
    HANDLE icmpHandle = IcmpCreateFile();

    if (icmpHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    char requestData[] = "NetScope";

    // Buffer for the reply and any additional ICMP data
    alignas(ICMP_ECHO_REPLY)
    unsigned char replyBuffer[
        sizeof(ICMP_ECHO_REPLY) + sizeof(requestData) + 64
    ]{};

    DWORD replies = IcmpSendEcho(
        icmpHandle,
        address.s_addr,
        requestData,
        sizeof(requestData),
        nullptr,
        replyBuffer,
        sizeof(replyBuffer),
        settings.icmpTimeoutMs
    );

    bool responding = false;

    if (replies > 0) {
        auto* reply =
            reinterpret_cast<ICMP_ECHO_REPLY*>(replyBuffer);

        responding = reply->Status == IP_SUCCESS;
    }

    IcmpCloseHandle(icmpHandle);

    return responding;

#elif defined(__APPLE__)
    // One ICMP request, one-second reply timeout
    std::string command =
    "/sbin/ping -n -c 1 -W " +
    std::to_string(settings.icmpTimeoutMs) +
    " " + ip + " >/dev/null 2>&1";

    return std::system(command.c_str()) == 0;

#else
    return false;
#endif
}



DiscoveredHost probeHost(
    const std::string& ip,
    const AppSettings& settings
) {
    DiscoveredHost host{};
    host.ipAddress = ip;

    // ICMP discovery
    if (settings.enableICMP) {
        host.icmpResponded = pingHost(ip, settings);
    }

    // ARP discovery
    if (settings.enableARP) {
        host.macAddress = getMACAddress(ip);
        host.arpFound = !host.macAddress.empty();

#ifdef _WIN32
        host.arpConfirmed = host.arpFound;
#else
        host.arpConfirmed = false;
#endif
    }

    // TCP fallback
    bool confirmed =
        host.icmpResponded || host.arpConfirmed;

    if (settings.enableTCP && !confirmed) {
        host.tcpResponded =
            isHostResponding(ip, settings);
    }

    host.responding =
        host.icmpResponded ||
        host.arpConfirmed ||
        host.tcpResponded;

    return host;
}


std::vector<DiscoveredHost> discoverHosts(
    const std::vector<std::string>& addresses,
    const AppSettings& settings,
    const std::function<void(std::size_t, std::size_t)>& onProgress,
    const std::function<bool()>& shouldStop
) {
    if (addresses.empty()) {
        return {};
    }

    const unsigned int threadCount = std::min(
        settings.maxThreads,
        static_cast<unsigned int>(addresses.size())
    );

    std::vector<DiscoveredHost> results(addresses.size());

    std::atomic<std::size_t> nextIndex{0};
    std::size_t completed = 0;
    std::mutex progressMutex;

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; ++i) {
        workers.emplace_back([&]() {
            while (true) {
                std::size_t index = nextIndex.fetch_add(1);

                if (index >= addresses.size()) {
                    break;
                }

                // Cooperative cancellation
                if (shouldStop && shouldStop()) {
                    break;
                }

                results[index] = probeHost(
                    addresses[index], settings
                );

                {
                    std::lock_guard<std::mutex> lock(
                        progressMutex
                    );

                    ++completed;

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

    std::vector<DiscoveredHost> discovered;

    for (const DiscoveredHost& host : results) {
        if (host.responding || host.arpFound) {
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

#ifdef _WIN32
    in_addr address{};

    if (InetPtonA(AF_INET, ip.c_str(), &address) != 1) {
        return "";
    }

    ULONG macBuffer[2]{};
    ULONG macLength = 6;

    DWORD result = SendARP(
        address.s_addr,
        0,
        macBuffer,
        &macLength
    );

    if (result != NO_ERROR || macLength != 6) {
        return "";
    }

    const unsigned char* bytes =
        reinterpret_cast<const unsigned char*>(macBuffer);

    std::ostringstream mac;

    mac << std::hex << std::setfill('0');

    for (ULONG i = 0; i < macLength; i++) {
        if (i > 0) {
            mac << ':';
        }

        mac << std::setw(2)
            << static_cast<unsigned int>(bytes[i]);
    }

    return mac.str();
#endif

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

std::string resolveHostname(const std::string& ip) {
    // Validate the IPv4 address
    if (!validateIPv4(ip)) {
        return "";
    }

    // Construct an IPv4 socket address
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = 0;

#ifdef _WIN32
    int converted = InetPtonA(
        AF_INET,
        ip.c_str(),
        &address.sin_addr
    );
#else
    int converted = inet_pton(
        AF_INET,
        ip.c_str(),
        &address.sin_addr
    );
#endif

    if (converted != 1) {
        return "";
    }

    // Buffer to store the resolved hostname
    char hostname[NI_MAXHOST]{};

    // Perform reverse DNS lookup
    int result = getnameinfo(
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address),
        hostname,
        sizeof(hostname),
        nullptr,
        0,
        NI_NAMEREQD
    );

    if (result != 0) {
        return "";
    }

    return std::string(hostname);
}
