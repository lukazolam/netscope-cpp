#include "macvendor.h"
#include <iostream>

int main() {
    int failures = 0;
    auto expect = [&](const std::string& value, const std::string& answer) {
        if (value != answer) {
            std::cerr << "Expected '" << answer << "', got '" << value << "'\n";
            ++failures;
        }
    };
    expect(lookupMACVendor("b8:27:eb:12:34:56"), "Raspberry Pi");
    expect(lookupMACVendor("B8-27-EB-12-34-56"), "Raspberry Pi");
    expect(lookupMACVendor("B827EB123456"), "Raspberry Pi");
    expect(lookupMACVendor("02:11:22:33:44:55"), "Private / randomized");
    expect(lookupMACVendor("01:00:5e:00:00:01"), "Multicast");
    expect(lookupMACVendor("Unknown"), "Unknown");
    expect(lookupMACVendor("b8:27:eb:12"), "Unknown");
    expect(lookupMACVendor("00:11:22:33:44:55"), "Unknown");
    return failures ? 1 : 0;
}
