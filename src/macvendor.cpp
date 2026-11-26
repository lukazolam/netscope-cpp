
#include "macvendor.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace {

int hexDigit(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

std::string lookupMACVendor(const std::string& mac) {
    std::array<unsigned char, 6> bytes{};

    std::string digits;
    digits.reserve(12);

    // Remove common MAC address separators
    for (unsigned char ch : mac) {
        if (ch == ':' || ch == '-') {
            continue;
        }

        if (hexDigit(ch) < 0) {
            return "Unknown";
        }

        digits.push_back(static_cast<char>(ch));
    }

    if (digits.size() != 12) {
        return "Unknown";
    }

    // Convert pairs of hex digits into six bytes
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        int upper = hexDigit(digits[i * 2]);
        int lower = hexDigit(digits[i * 2 + 1]);

        bytes[i] = static_cast<unsigned char>(
            (upper << 4) | lower
        );
    }

    // Group or multicast address
    if ((bytes[0] & 1u) != 0) {
        return "Multicast";
    }

    // Locally administered address
    if ((bytes[0] & 2u) != 0) {
        return "Private / locally administered";
    }

    // Construct the 24-bit OUI
    std::uint32_t oui =
        (static_cast<std::uint32_t>(bytes[0]) << 16) |
        (static_cast<std::uint32_t>(bytes[1]) << 8) |
        static_cast<std::uint32_t>(bytes[2]);

    // Starter manufacturer database
    static const std::unordered_map<
        std::uint32_t, const char*
    > known = {
        {0xB827EBu, "Raspberry Pi"},
        {0xDCA632u, "Raspberry Pi"},
        {0xE45F01u, "Raspberry Pi"},
        {0x28CDC1u, "Raspberry Pi"},
        {0x080027u, "VirtualBox"},
        {0x000C29u, "VMware"},
        {0x005056u, "VMware"},
        {0x00155Du, "Microsoft Hyper-V"},
        {0x00163Eu, "Xen"},
        {0x00156Du, "Ubiquiti"},
        {0x246511u, "AVM"},
        {0x00040Eu, "AVM"},
        {0x000C42u, "MikroTik"},
        {0x50C7BFu, "TP-Link"},
        {0x00095Bu, "Netgear"},
        {0x001132u, "Synology"},
        {0x00089Bu, "QNAP"},
        {0x000A95u, "Apple"},
        {0x001B63u, "Apple"},
        {0x001D25u, "Samsung"},
        {0x001422u, "Dell"},
        {0x001F29u, "HP"},
        {0x001B21u, "Intel"},
        {0x00000Cu, "Cisco"},
        {0x000E58u, "Sonos"}
    };

    auto it = known.find(oui);

    if (it == known.end()) {
        return "Unknown";
    }

    return it->second;
}
