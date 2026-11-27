
#include "macvendor.h"
#include "macvendor_generated.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <string>

namespace {

int hexDigit(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

template <std::size_t N>
const char* findVendor(
    const netscope_vendor_db::Entry (&entries)[N],
    std::uint64_t prefix
) {
    auto it = std::lower_bound(
        std::begin(entries),
        std::end(entries),
        prefix,
        [](const netscope_vendor_db::Entry& entry,
           std::uint64_t value) {
            return entry.prefix < value;
        }
    );

    if (it != std::end(entries) && it->prefix == prefix) {
        return it->vendor;
    }

    return nullptr;
}

} // namespace

std::string lookupMACVendor(const std::string& mac) {
    std::string digits;
    digits.reserve(12);

    // Normalize common MAC address formats.
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

    std::array<unsigned char, 6> bytes{};

    std::uint64_t address = 0;

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const int high = hexDigit(digits[i * 2]);
        const int low = hexDigit(digits[i * 2 + 1]);

        bytes[i] = static_cast<unsigned char>(
            (high << 4) | low
        );

        address = (address << 8) | bytes[i];
    }

    // Multicast/group MAC addresses aren't device OUIs.
    if ((bytes[0] & 1u) != 0) {
        return "Multicast";
    }

    // Randomized or locally administered address.
    if ((bytes[0] & 2u) != 0) {
        return "Private / locally administered";
    }

    // Search most-specific assignments first:
    // MA-S: first 36 bits
    // MA-M: first 28 bits
    // MA-L: first 24 bits

    if (const char* vendor = findVendor(
        netscope_vendor_db::maS,
        address >> 12
    )) {
        return vendor;
    }

    if (const char* vendor = findVendor(
        netscope_vendor_db::maM,
        address >> 20
    )) {
        return vendor;
    }

    if (const char* vendor = findVendor(
        netscope_vendor_db::maL,
        address >> 24
    )) {
        return vendor;
    }

    return "Unknown";
}
