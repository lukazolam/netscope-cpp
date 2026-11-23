
#include "systeminfo.h"

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>

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

    for (auto& entry : interfaces) {
        if (entry.second.macAddress.empty()) {
            entry.second.macAddress = "Unknown";
        }

        results.push_back(entry.second);
    }

    return results;
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
            GAA_FLAG_INCLUDE_PREFIX,
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

#endif
