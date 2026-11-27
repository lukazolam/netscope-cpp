
#include "macvendor.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    // MAC formats and validation
    assert(lookupMACVendor("") == "Unknown");
    assert(lookupMACVendor("invalid") == "Unknown");

    assert(
        lookupMACVendor("02:11:22:33:44:55") ==
        "Private / locally administered"
    );

    assert(
        lookupMACVendor("01:00:5E:00:00:01") ==
        "Multicast"
    );

    // A historically registered Apple MA-L prefix.
    const std::string apple =
        lookupMACVendor("00:0A:95:12:34:56");

    std::cout << "Apple test: " << apple << '\n';

    assert(apple != "Unknown");

    std::cout << "MAC vendor tests passed.\n";
}
