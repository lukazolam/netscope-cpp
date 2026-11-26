#pragma once
#include <string>

// Human-readable vendor from the first 24 bits (OUI) of a MAC address.
// A missing vendor means the prefix is unknown to the built-in subset.
std::string lookupMACVendor(const std::string& mac);
