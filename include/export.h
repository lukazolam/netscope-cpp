#pragma once

#include "network.h"
#include <string>
#include <vector>

// Export the completed discovery list to .csv or .json.
// Returns false for unsupported extensions or file I/O failures.
bool exportDiscoveryResults(
    const std::vector<DiscoveredHost>& hosts,
    const std::string& filename
);
