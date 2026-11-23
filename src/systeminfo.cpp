
#include "systeminfo.h"

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <windows.h>

#elif defined(__APPLE__)
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <net/if_dl.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <cstring>
    #include <cstdio>
    #include <algorithm>
    #include <unistd.h>
#endif

// Format six or more MAC bytes as hexadecimal text.
static std::string formatMAC(
    const unsigned char* bytes,
    std::size_t length
) {
    if (bytes == nullptr || length == 0) {
        return "Unknown";
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < length; i++) {
        if (i > 0) {
            result << ':';
        }

        result << std::setw(2)
               << static_cast<unsigned int>(bytes[i]);
    }

    return result.str();
}

#ifdef __APPLE__

static unsigned int getInterfaceMTU(
    const std::string& name
) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        return 0;
    }

    ifreq request{};

    std::strncpy(
        request.ifr_name,
        name.c_str(),
        sizeof(request.ifr_name) - 1
    );

    unsigned int mtu = 0;

    if (ioctl(sock, SIOCGIFMTU, &request) == 0) {
        if (request.ifr_mtu > 0) {
            mtu = static_cast<unsigned int>(
                request.ifr_mtu
            );
        }
    }

    close(sock);
    return mtu;
}

static std::pair<std::string, std::string>
getDefaultIPv4Route() {
    FILE* pipe = popen(
        "/sbin/route -n get -inet default 2>/dev/null",
        "r"
    );

    if (!pipe) {
        return {"", ""};
    }

    std::string gateway;
    std::string interfaceName;
    char buffer[512];

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line = buffer;

        std::size_t colon = line.find(':');

        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Remove leading/trailing whitespace
        auto first = value.find_first_not_of(" \t\r\n");

        if (first == std::string::npos) {
            continue;
        }

        auto last = value.find_last_not_of(" \t\r\n");

        value = value.substr(first, last - first + 1);

        if (key.find("gateway") != std::string::npos) {
            gateway = value;
        }

        if (key.find("interface") != std::string::npos) {
            interfaceName = value;
        }
    }

    pclose(pipe);

    return {interfaceName, gateway};
}

#endif

#ifdef __APPLE__

std::vector<NetworkInterface> getNetworkInterfaces() {
    std::map<std::string, NetworkInterface> interfaces;

    ifaddrs* interfaceList = nullptr;

    if (getifaddrs(&interfaceList) != 0) {
        return {};
    }

    for (ifaddrs* current = interfaceList;
         current != nullptr;
         current = current->ifa_next) {

        if (!current->ifa_name || !current->ifa_addr) {
            continue;
        }

        std::string name = current->ifa_name;

        // Create or retrieve this interface
        NetworkInterface& adapter = interfaces[name];

        adapter.name = name;
        adapter.isUp = (current->ifa_flags & IFF_UP) != 0;

        int family = current->ifa_addr->sa_family;

        // MAC address
        if (family == AF_LINK) {
            auto* link = reinterpret_cast<sockaddr_dl*>(
                current->ifa_addr
            );

            if (link->sdl_alen > 0) {
                const auto* bytes =
                    reinterpret_cast<const unsigned char*>(
                        LLADDR(link)
                    );

                adapter.macAddress = formatMAC(
                    bytes,
                    link->sdl_alen
                );
            }
        }

        // IPv4 and IPv6 addresses
        else if (family == AF_INET || family == AF_INET6) {
            char addressBuffer[INET6_ADDRSTRLEN]{};

            const void* addressPtr = nullptr;

            if (family == AF_INET) {
                auto* ipv4 = reinterpret_cast<sockaddr_in*>(
                    current->ifa_addr
                );

                addressPtr = &ipv4->sin_addr;
            } else {
                auto* ipv6 = reinterpret_cast<sockaddr_in6*>(
                    current->ifa_addr
                );

                addressPtr = &ipv6->sin6_addr;
            }

            if (!inet_ntop(
                    family,
                    addressPtr,
                    addressBuffer,
                    sizeof(addressBuffer)
                )) {
                continue;
            }

            InterfaceAddress info{};
            info.address = addressBuffer;
            info.family = family == AF_INET ? 4 : 6;
            info.prefixLength = 0;

            // Read the subnet mask and prefix length
            if (current->ifa_netmask) {
                const unsigned char* maskBytes = nullptr;
                std::size_t byteCount = 0;

                if (family == AF_INET) {
                    auto* mask = reinterpret_cast<sockaddr_in*>(
                        current->ifa_netmask
                    );

                    maskBytes = reinterpret_cast<
                        const unsigned char*
                    >(&mask->sin_addr);

                    byteCount = 4;

                    char maskBuffer[INET_ADDRSTRLEN]{};

                    if (inet_ntop(
                            AF_INET,
                            &mask->sin_addr,
                            maskBuffer,
                            sizeof(maskBuffer)
                        )) {
                        info.subnetMask = maskBuffer;
                    }
                } else {
                    auto* mask = reinterpret_cast<sockaddr_in6*>(
                        current->ifa_netmask
                    );

                    maskBytes = reinterpret_cast<
                        const unsigned char*
                    >(&mask->sin6_addr);

                    byteCount = 16;
                }

                for (std::size_t i = 0; i < byteCount; i++) {
                    unsigned char value = maskBytes[i];

                    while (value != 0) {
                        info.prefixLength += value & 1;
                        value >>= 1;
                    }
                }
            }

            adapter.addresses.push_back(info);
        }
    }

    freeifaddrs(interfaceList);

    // Convert the map to our public vector format
    std::vector<NetworkInterface> results;
    auto defaultRoute = getDefaultIPv4Route();

    for (auto& entry : interfaces) {
        if (entry.second.macAddress.empty()) {
            entry.second.macAddress = "Unknown";
        }
        entry.second.mtu = getInterfaceMTU(entry.second.name);
        if (entry.second.name == defaultRoute.first) {
            entry.second.defaultGateway = defaultRoute.second;
        }
        results.push_back(entry.second);
    }

    return results;
}

std::vector<std::string> getSystemDNSServers() {
    std::vector<std::string> servers;

    FILE* pipe = popen(
        "/usr/sbin/scutil --dns 2>/dev/null",
        "r"
    );

    if (!pipe) {
        return servers;
    }

    char buffer[512];
    bool inFirstResolver = false;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line = buffer;

        // Begin reading the first resolver
        if (line.find("resolver #1") != std::string::npos) {
            inFirstResolver = true;
            continue;
        }

        if (!inFirstResolver) {
            continue;
        }

        // Stop at the next resolver configuration
        if (line.find("resolver #") != std::string::npos) {
            break;
        }

        // Extract nameserver entries
        if (line.find("nameserver[") != std::string::npos) {
            std::size_t colon = line.find(':');

            if (colon == std::string::npos) {
                continue;
            }

            std::string server = line.substr(colon + 1);

            auto first = server.find_first_not_of(" \t\r\n");

            if (first == std::string::npos) {
                continue;
            }

            auto last = server.find_last_not_of(" \t\r\n");

            server = server.substr(
                first,
                last - first + 1
            );

            // Avoid duplicate entries
            if (std::find(
                    servers.begin(),
                    servers.end(),
                    server
                ) == servers.end()) {
                servers.push_back(server);
            }
        }
    }

    pclose(pipe);
    return servers;
}

#endif


#ifdef _WIN32

// Convert Windows UTF-16 adapter names to UTF-8
static std::string wideToUTF8(const wchar_t* text) {
    if (!text) {
        return "";
    }

    int length = WideCharToMultiByte(
        CP_UTF8, 0, text, -1,
        nullptr, 0, nullptr, nullptr
    );

    if (length <= 0) {
        return "";
    }

    std::string result(length, '\0');

    WideCharToMultiByte(
        CP_UTF8, 0, text, -1,
        result.data(), length, nullptr, nullptr
    );

    result.pop_back(); // Remove terminating null
    return result;
}

static std::string socketAddressToString(
    const sockaddr* address
) {
    if (!address) {
        return "";
    }

    char buffer[INET6_ADDRSTRLEN]{};

    if (address->sa_family == AF_INET) {
        auto* ipv4 =
            reinterpret_cast<const sockaddr_in*>(address);

        if (InetNtopA(
                AF_INET,
                const_cast<IN_ADDR*>(&ipv4->sin_addr),
                buffer,
                sizeof(buffer)
            )) {
            return buffer;
        }
    }

    if (address->sa_family == AF_INET6) {
        auto* ipv6 =
            reinterpret_cast<const sockaddr_in6*>(address);

        if (InetNtopA(
                AF_INET6,
                const_cast<IN6_ADDR*>(&ipv6->sin6_addr),
                buffer,
                sizeof(buffer)
            )) {
            return buffer;
        }
    }

    return "";
}

std::vector<NetworkInterface> getNetworkInterfaces() {
    std::vector<NetworkInterface> results;

    ULONG bufferSize = 15000;
    std::unique_ptr<unsigned char[]> buffer;
    ULONG status = ERROR_BUFFER_OVERFLOW;

    // Retry if adapter information changes or buffer grows
    for (int attempt = 0; attempt < 3; attempt++) {
        buffer.reset(new unsigned char[bufferSize]);

        auto* adapters =
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(
                buffer.get()
            );

        status = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS,
            nullptr,
            adapters,
            &bufferSize
        );

        if (status != ERROR_BUFFER_OVERFLOW) {
            break;
        }
    }

    if (status != NO_ERROR) {
        return results;
    }

    auto* adapters =
        reinterpret_cast<IP_ADAPTER_ADDRESSES*>(
            buffer.get()
        );

    for (auto* current = adapters;
         current != nullptr;
         current = current->Next) {

        NetworkInterface adapter{};

        adapter.name = wideToUTF8(current->FriendlyName);
        adapter.isUp = current->OperStatus == IfOperStatusUp;

        adapter.macAddress = formatMAC(
            current->PhysicalAddress,
            current->PhysicalAddressLength
        );
        
        // Read the interface MTU
        adapter.mtu = current->Mtu;

        // Read the first IPv4 gateway
        for (auto* gateway = current->FirstGatewayAddress;
            gateway != nullptr;
            gateway = gateway->Next) {

            const sockaddr* address =
                gateway->Address.lpSockaddr;

            if (address && address->sa_family == AF_INET) {
                adapter.defaultGateway =
                    socketAddressToString(address);

                if (!adapter.defaultGateway.empty()) {
                    break;
                }
            }
        }

        // Read adapter DNS servers
        for (auto* dns = current->FirstDnsServerAddress;
            dns != nullptr;
            dns = dns->Next) {

            std::string server = socketAddressToString(
                dns->Address.lpSockaddr
            );

            if (!server.empty()) {
                adapter.dnsServers.push_back(server);
            }
        }

        // Enumerate all IP addresses on this adapter
        for (auto* address = current->FirstUnicastAddress;
             address != nullptr;
             address = address->Next) {

            if (!address->Address.lpSockaddr) {
                continue;
            }

            int family =
                address->Address.lpSockaddr->sa_family;

            if (family != AF_INET && family != AF_INET6) {
                continue;
            }

            InterfaceAddress info{};

            info.family = family == AF_INET ? 4 : 6;
            info.prefixLength = address->OnLinkPrefixLength;

            char addressBuffer[INET6_ADDRSTRLEN]{};
            const void* addressPtr = nullptr;

            if (family == AF_INET) {
                auto* ipv4 =
                    reinterpret_cast<sockaddr_in*>(
                        address->Address.lpSockaddr
                    );

                addressPtr = &ipv4->sin_addr;

                // Derive IPv4 subnet mask from prefix length
                unsigned int prefix = info.prefixLength;

                if (prefix <= 32) {
                    unsigned int mask = prefix == 0
                        ? 0
                        : 0xFFFFFFFFu << (32 - prefix);

                    in_addr maskAddress{};
                    maskAddress.s_addr = htonl(mask);

                    char maskBuffer[INET_ADDRSTRLEN]{};

                    if (InetNtopA(
                            AF_INET,
                            &maskAddress,
                            maskBuffer,
                            sizeof(maskBuffer)
                        )) {
                        info.subnetMask = maskBuffer;
                    }
                }
            } else {
                auto* ipv6 =
                    reinterpret_cast<sockaddr_in6*>(
                        address->Address.lpSockaddr
                    );

                addressPtr = &ipv6->sin6_addr;
            }

            if (InetNtopA(
                    family,
                    const_cast<void*>(addressPtr),
                    addressBuffer,
                    sizeof(addressBuffer)
                )) {
                info.address = addressBuffer;
                adapter.addresses.push_back(info);
            }
        }

        results.push_back(adapter);
    }

    return results;
}
std::vector<std::string> getSystemDNSServers() {
    return {};
}
#endif
