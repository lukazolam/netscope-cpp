
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include "network.h"
#include "systeminfo.h"
#include "settings.h"

void displayProgress(std::size_t completed, std::size_t total) {
    const int barWidth = 30;

    double progress = total == 0
        ? 1.0
        : static_cast<double>(completed) / total;

    int filled = static_cast<int>(barWidth * progress);

    std::cout << "\r[";

    for (int i = 0; i < barWidth; i++) {
        if (i < filled) {
            std::cout << "#";
        } else {
            std::cout << "-";
        }
    }

    std::cout << "] "
              << std::setw(3) << static_cast<int>(progress * 100)
              << "% (" << completed << "/" << total << ")"
              << std::flush;
}

void displayPortScan(const std::string& ip, const std::vector<int>& ports, const AppSettings& settings) {
    std::cout << "\nScanning " << ip << "...\n";

    auto startTime = std::chrono::steady_clock::now();

        
    int lastPercentage = -1;

    std::function<void(std::size_t, std::size_t)> progressCallback;

    if (settings.showProgress) {
        progressCallback =
            [&](std::size_t completed, std::size_t total) {
                int percentage = static_cast<int>(
                    (completed * 100) / total
                );

                if (percentage <= lastPercentage) {
                    return;
                }

                lastPercentage = percentage;
                displayProgress(completed, total);
            };
    }

    std::vector<PortScanResult> results = scanTCPPorts(
        ip,
        ports,
        settings,
        progressCallback
    );

    std::cout << '\n';

    auto endTime = std::chrono::steady_clock::now();

    int openCount = 0;
    int closedCount = 0;
    int timeoutCount = 0;
    int errorCount = 0;

    // Process returned results
    std::cout << "\n";
    std::cout << "\n========== OPEN PORTS ==========\n";
    for (const PortScanResult& result : results) {
        switch (result.status) {
            case PortStatus::Open:
                std::cout << result.port << "/tcp\tOPEN\n";
                openCount++;
                break;

            case PortStatus::Closed:
                closedCount++;
                break;

            case PortStatus::Timeout:
                timeoutCount++;
                break;

            case PortStatus::Error:
                errorCount++;
                break;
        }
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    );

    std::cout << "\n========== SCAN RESULTS ==========\n";
    std::cout << "Ports scanned: " << results.size() << '\n';
    std::cout << "Open:          " << openCount << '\n';
    std::cout << "Closed:        " << closedCount << '\n';
    std::cout << "Timed out:     " << timeoutCount << '\n';
    std::cout << "Errors:        " << errorCount << '\n';
    std::cout << "Duration:      " << duration.count() << " ms\n";
}

void showSubnetCalculator() {
    std::string ip;
    std::string prefixInput;

    std::cout << "\n========== SUBNET CALCULATOR ==========\n";

    // Get IPv4 address
    std::cout << "IPv4 Address: ";

    if (!std::getline(std::cin, ip)) {
        return;
    }

    if (!validateIPv4(ip)) {
        std::cout << "\n[ERROR] Invalid IPv4 address.\n";
        return;
    }

    // Get CIDR prefix
    std::cout << "CIDR Prefix (0-32): ";

    if (!std::getline(std::cin, prefixInput)) {
        return;
    }

    int prefix;

    // Convert prefix string into integer
    try {
        std::size_t processed = 0;
        prefix = std::stoi(prefixInput, &processed);

        if (processed != prefixInput.length() || prefix < 0 || prefix > 32) {
            std::cout << "\n[ERROR] CIDR prefix must be between 0 and 32.\n";
            return;
        }
    }
    catch (const std::exception&) {
        std::cout << "\n[ERROR] Invalid CIDR prefix.\n";
        return;
    }

    // Calculate subnet information
    SubnetInfo subnet = calculateSubnet(ip, prefix);

    // Display results
    std::cout << "\n========== RESULTS ==========\n";
    std::cout << "Network Address:    " << subnet.networkAddress << '\n';
    std::cout << "Subnet Mask:        " << subnet.subnetMask << '\n';
    std::cout << "Broadcast Address:  " << subnet.broadcastAddress << '\n';
    std::cout << "First Usable IP:    " << subnet.firstHost << '\n';
    std::cout << "Last Usable IP:     " << subnet.lastHost << '\n';
    std::cout << "Usable Hosts:       " << subnet.usableHosts << '\n';
}

void showSinglePortScanner(const AppSettings& settings) {
    std::string ip;
    std::string portInput;

    std::cout << "\n========== TCP PORT SCANNER ==========\n";

    // Get the target IPv4 address
    std::cout << "Target IPv4: ";

    if (!std::getline(std::cin, ip)) {
        return;
    }

    if (!validateIPv4(ip)) {
        std::cout << "\n[ERROR] Invalid IPv4 address.\n";
        return;
    }

    // Get the target TCP port
    std::cout << "TCP Port (1-65535): ";

    if (!std::getline(std::cin, portInput)) {
        return;
    }

    // Reject empty or non-numeric port input
    if (portInput.empty() ||
        portInput.find_first_not_of("0123456789") != std::string::npos) {
        std::cout << "\n[ERROR] Invalid port number.\n";
        return;
    }

    int port;

    try {
        port = std::stoi(portInput);
    }
    catch (const std::exception&) {
        std::cout << "\n[ERROR] Invalid port number.\n";
        return;
    }

    if (port < 1 || port > 65535) {
        std::cout << "\n[ERROR] Port must be between 1 and 65535.\n";
        return;
    }

    // Start the scan
    std::cout << "\nScanning " << ip << ":" << port << "...\n";

    PortStatus result = scanTCPPort(ip, port, settings);

    // Display the result
    switch (result) {
        case PortStatus::Open:
            std::cout << "[OPEN] TCP port " << port
                      << " is accepting connections.\n";
            break;

        case PortStatus::Closed:
            std::cout << "[CLOSED] TCP port " << port
                      << " refused the connection.\n";
            break;

        case PortStatus::Timeout:
            std::cout << "[TIMEOUT] No connection response received.\n";
            break;

        case PortStatus::Error:
            std::cout << "[ERROR] Unable to complete the scan.\n";
            break;
    }
}

void showCommonPortScanner(const AppSettings& settings) {
    std::string ip;

    std::cout << "\n========== COMMON PORT SCANNER ==========\n";
    std::cout << "Target IPv4: ";

    if (!std::getline(std::cin, ip)) {
        return;
    }

    if (!validateIPv4(ip)) {
        std::cout << "\n[ERROR] Invalid IPv4 address.\n";
        return;
    }

    std::vector<int> commonPorts = {
        21, 22, 23, 25, 53,
        80, 110, 143, 443, 445,
        993, 995, 1433, 3306, 3389,
        5432, 5900, 6379, 8080, 8443
    };

    displayPortScan(ip, commonPorts, settings);
}

void showPortRangeScanner(const AppSettings& settings) {
    std::string ip;
    std::string startInput;
    std::string endInput;

    std::cout << "\n========== PORT RANGE SCANNER ==========\n";

    // Get target IP
    std::cout << "Target IPv4: ";
    if (!std::getline(std::cin, ip)) {
        return;
    }

    if (!validateIPv4(ip)) {
        std::cout << "\n[ERROR] Invalid IPv4 address.\n";
        return;
    }

    // Get starting port
    std::cout << "Starting Port: ";
    if (!std::getline(std::cin, startInput)) {
        return;
    }

    // Get ending port
    std::cout << "Ending Port: ";
    if (!std::getline(std::cin, endInput)) {
        return;
    }

    int startPort;
    int endPort;

    try {
        std::size_t startProcessed = 0;
        std::size_t endProcessed = 0;

        startPort = std::stoi(startInput, &startProcessed);
        endPort = std::stoi(endInput, &endProcessed);

        // Reject invalid characters
        if (startProcessed != startInput.length() ||
            endProcessed != endInput.length()) {
            std::cout << "\n[ERROR] Invalid port numbers.\n";
            return;
        }
    }
    catch (const std::exception&) {
        std::cout << "\n[ERROR] Invalid port numbers.\n";
        return;
    }

    // Validate the range
    if (startPort < 1 || endPort > 65535 || startPort > endPort) {
        std::cout << "\n[ERROR] Invalid port range.\n";
        return;
    }

    if (endPort - startPort + 1 > 10000) {
        std::cout << "\n[ERROR] Maximum range is 1000 ports.\n";
        return;
    }

    // Generate port list
    std::vector<int> ports;

    for (int port = startPort; port <= endPort; port++) {
        ports.push_back(port);
    }

    displayPortScan(ip, ports, settings);
}

void showTCPScanner(const AppSettings& settings) {
    bool running = true;

    while (running) {
        std::cout << "\n========== TCP PORT SCANNER ==========\n";
        std::cout << "1. Scan Single Port\n";
        std::cout << "2. Scan Port Range\n";
        std::cout << "3. Scan Common Ports\n";
        std::cout << "0. Back to Network Tools\n";

        std::string input;
        std::cout << "\nSelect an option: ";

        if (!std::getline(std::cin, input)) {
            return;
        }

        if (input == "1") {
            showSinglePortScanner(settings);
        }
        else if (input == "2") {
            showPortRangeScanner(settings);
        }
        else if (input == "3") {
            showCommonPortScanner(settings);
        }
        else if (input == "0") {
            running = false;
        }
        else if (input.empty()) {
            std::cout << "\n[ERROR] Please select an option.\n";
        }
        else {
            std::cout << "\n[ERROR] Invalid option.\n";
        }
    }
}

void showNetworkDiscovery(const AppSettings& settings) {
    std::string ip;
    std::string prefixInput;

    std::cout << "\n========== NETWORK DISCOVERY ==========\n";

    // Read network address
    std::cout << "IPv4 Address: ";

    if (!std::getline(std::cin, ip)) {
        return;
    }

    if (!validateIPv4(ip)) {
        std::cout << "\n[ERROR] Invalid IPv4 address.\n";
        return;
    }

    // Read CIDR prefix
    std::cout << "CIDR Prefix (24-32): ";

    if (!std::getline(std::cin, prefixInput)) {
        return;
    }

    int prefix;

    try {
        std::size_t processed = 0;
        prefix = std::stoi(prefixInput, &processed);

        if (processed != prefixInput.length() ||
            prefix < 24 || prefix > 32) {
            std::cout << "\n[ERROR] Prefix must be between 24 and 32.\n";
            return;
        }
    }
    catch (const std::exception&) {
        std::cout << "\n[ERROR] Invalid CIDR prefix.\n";
        return;
    }

   
    // Generate host addresses
    std::vector<std::string> hosts = generateHostIPs(ip, prefix);

    // Get subnet information
    SubnetInfo subnet = calculateSubnet(ip, prefix);

    std::cout << "\n========== NETWORK DISCOVERY ==========\n";
    std::cout << "Network: " << subnet.networkAddress
            << "/" << prefix << '\n';
    std::cout << "Subnet Mask: " << subnet.subnetMask << '\n';
    std::cout << "Hosts to scan: " << hosts.size() << "\n\n";

        
    std::cout << "Discovering devices...\n";

    // Keep track of the last displayed percentage

    int lastPercentage = -1;

    std::function<void(std::size_t, std::size_t)> progressCallback;

    if (settings.showProgress) {
        progressCallback =
            [&](std::size_t completed, std::size_t total) {
                int percentage = static_cast<int>(
                    (completed * 100) / total
                );

                if (percentage <= lastPercentage) {
                    return;
                }

                lastPercentage = percentage;
                displayProgress(completed, total);
            };
    }

    // Run discovery with the current settings
    std::vector<DiscoveredHost> discovered = discoverHosts(
        hosts,
        settings,
        progressCallback
    );

// Count MAC addresses found during discovery
int macCount = 0;

for (DiscoveredHost& host : discovered) {
    if (!host.macAddress.empty()) {
        macCount++;
    } else {
        host.macAddress = "Unknown";
    }
}

// Resolve hostnames of discovered devices
std::cout << "\nResolving DNS hostnames...\n";

int hostnameCount = 0;

for (DiscoveredHost& host : discovered) {
    host.hostname = resolveHostname(host.ipAddress);

    if (!host.hostname.empty()) {
        hostnameCount++;
    } else {
        host.hostname = "Unknown";
    }
}

// Move to the next line after the progress bar
std::cout << '\n';


    // Display discovered devices


    std::cout << "\n========== DISCOVERED DEVICES ==========\n";

    std::cout << std::left
            << std::setw(18) << "IP ADDRESS"
            << std::setw(20) << "MAC ADDRESS"
            << std::setw(32) << "HOSTNAME"
            << std::setw(7)  << "ICMP"
            << std::setw(9)  << "ARP"
            << std::setw(7)  << "TCP"
            << "STATUS\n";

    std::cout << std::string(108, '-') << '\n';

    for (const DiscoveredHost& host : discovered) {
        std::string arpStatus =
            host.arpConfirmed ? "Yes" :
            host.arpFound ? "Cached" : "No";

        std::string tcpStatus =
            host.tcpResponded ? "Yes" :
            (host.icmpResponded || host.arpConfirmed) ? "-" : "No";

        std::cout << std::left
                << std::setw(18) << host.ipAddress
                << std::setw(20) << host.macAddress
                << std::setw(32) << host.hostname
                << std::setw(7)
                << (host.icmpResponded ? "Yes" : "No")
                << std::setw(9) << arpStatus
                << std::setw(7) << tcpStatus
                << (host.responding ? "Confirmed" : "ARP cached")
                << '\n';
    }



std::cout << "\n========== RESULTS ==========\n";
std::cout << "Hosts scanned:     " << hosts.size() << '\n';
std::cout << "Hosts discovered:  " << discovered.size() << '\n';
std::cout << "MAC addresses found: " << macCount << '\n';
std::cout << "Hostnames resolved:  " << hostnameCount << '\n';


int icmpCount = 0;
int arpCount = 0;
int tcpCount = 0;
int confirmedCount = 0;
int cachedOnlyCount = 0;

for (const DiscoveredHost& host : discovered) {
    if (host.icmpResponded) {
        icmpCount++;
    }

    if (host.arpFound) {
        arpCount++;
    }

    if (host.tcpResponded) {
        tcpCount++;
    }

    if (host.responding) {
        confirmedCount++;
    } else {
        cachedOnlyCount++;
    }
}

std::cout << "\n========== DISCOVERY METHODS ==========\n";
std::cout << "ICMP replies:       " << icmpCount << '\n';
std::cout << "ARP mappings:       " << arpCount << '\n';
std::cout << "TCP fallback hits:  " << tcpCount << '\n';
std::cout << "Confirmed hosts:    " << confirmedCount << '\n';
std::cout << "ARP-only candidates:" << cachedOnlyCount << '\n';
}

void showNetworkTools(const AppSettings& settings) {
    bool running = true;

    while (running) {
        std::cout << "\n========== NETWORK TOOLS ==========\n";
        std::cout << "1. Validate IPv4 Address\n";
        std::cout << "2. Calculate Subnet\n";
        std::cout << "3. TCP Port Scanner\n";
        std::cout << "4. Network Discovery\n";
        std::cout << "0. Back to Main Menu\n";

        std::string input;
        std::cout << "\nSelect an option: ";

        if (!std::getline(std::cin, input)) {
            return;
        }

        if (input == "1") {
            std::string ip;

            std::cout << "\nEnter IPv4 Address: ";

            if (!std::getline(std::cin, ip)) {
                return;
            }

            if (ip.empty()) {
                std::cout << "\n[ERROR] No IPv4 address entered.\n";
                continue;
            }

            if (validateIPv4(ip)) {
                std::cout << "\n[OK] Valid IPv4 address.\n";
            } else {
                std::cout << "\n[ERROR] Invalid IPv4 address.\n";
            }
        }
        else if (input == "2") {
            showSubnetCalculator();
        }
        else if (input == "3") {
            showTCPScanner(settings);
        }
        else if (input == "4") {
            showNetworkDiscovery(settings);
        }
        else if (input == "0") {
            running = false;
        }
        else {
            std::cout << "\n[ERROR] Invalid option.\n";
        }
    }
}

bool isRelevantAddress(const InterfaceAddress& address) {
    if (address.family == 4) {
        return address.address.rfind("127.", 0) != 0 &&
               address.address.rfind("169.254.", 0) != 0 &&
               address.address != "0.0.0.0";
    }

    if (address.family == 6) {
        if (address.address == "::1" ||
            address.address == "::") {
            return false;
        }

        std::string prefix = address.address.substr(0, 3);

        return prefix != "fe8" &&
               prefix != "fe9" &&
               prefix != "fea" &&
               prefix != "feb";
    }

    return false;
}

bool isRelevantInterface(const NetworkInterface& adapter) {
    if (!adapter.isUp) {
        return false;
    }

    for (const InterfaceAddress& address : adapter.addresses) {
        if (isRelevantAddress(address)) {
            return true;
        }
    }

    return false;
}

void showLocalNetworkInformation(const AppSettings& settings) {
    std::cout << "\n========== LOCAL NETWORK INFORMATION ==========\n";

    // Request local adapters from the operating system
    std::vector<NetworkInterface> interfaces =
        getNetworkInterfaces();

    // Keep only interfaces with relevant IP connectivity
    std::vector<NetworkInterface> activeInterfaces;

    for (const NetworkInterface& adapter : interfaces) {
    if (settings.showAllAdapters ||
        isRelevantInterface(adapter)) {
        activeInterfaces.push_back(adapter);
        }
    }

    if (activeInterfaces.empty()) {
        std::cout << "\nNo active network interfaces with usable IP addresses found.\n";
        return;
    }

    std::cout << "\nNetwork Interfaces Shown: "
          << activeInterfaces.size() << '\n';

    for (const NetworkInterface& adapter : activeInterfaces) {
        std::cout << "\n========== INTERFACE: "
                  << adapter.name
                  << " ==========\n";

        std::cout << "Status:       "
                  << (adapter.isUp ? "Up" : "Down") << '\n';

        std::cout << "MAC Address:  "
                  << adapter.macAddress << '\n';
        
        std::cout << "MTU:          ";
        if (adapter.mtu > 0) {
            std::cout << adapter.mtu << '\n';
        } else {
            std::cout << "Unknown\n";
        }

        bool hasIPv4 = false;
        bool hasIPv6 = false;

        for (const InterfaceAddress& address : adapter.addresses) {
            if (!settings.showAllAdapters &&
                !isRelevantAddress(address)) {
                continue;
            }
            if (address.family == 4) {
                if (!hasIPv4) {
                    std::cout << "\nIPv4:\n";
                    hasIPv4 = true;
                }

                std::cout << "  Address:     "
                          << address.address << '\n';

                std::cout << "  Subnet Mask: "
                          << (address.subnetMask.empty()
                              ? "Unknown" : address.subnetMask)
                          << '\n';
                
                if (!adapter.defaultGateway.empty()) {
                    std::cout << "  Gateway:     "
                            << adapter.defaultGateway << '\n';
                }
            }
        }

        for (const InterfaceAddress& address : adapter.addresses) {
            if (!settings.showAllAdapters &&
                !isRelevantAddress(address)) {
                continue;
            }
            if (address.family == 6) {
                if (!hasIPv6) {
                    std::cout << "\nIPv6:\n";
                    hasIPv6 = true;
                }

                std::cout << "  Address:     "
                          << address.address << '\n';

                std::cout << "  Prefix:      /"
                          << address.prefixLength << '\n';
            }
        }

        if (!adapter.dnsServers.empty()) {
            std::cout << "\nDNS Servers:\n";

            for (const std::string& dns : adapter.dnsServers) {
                std::cout << "  " << dns << '\n';
            }
        }

        if (adapter.addresses.empty()) {
            std::cout << "\nNo IP addresses assigned.\n";
        }
    }
    #ifdef __APPLE__
    std::vector<std::string> dnsServers =
        getSystemDNSServers();

    std::cout << "\n========== DNS CONFIGURATION ==========\n";
    std::cout << "Default Resolver:\n";

    if (dnsServers.empty()) {
        std::cout << "  No DNS servers found.\n";
    } else {
        for (const std::string& dns : dnsServers) {
            std::cout << "  " << dns << '\n';
        }
    }
    #endif
}

bool readSettingNumber(
    const std::string& prompt,
    unsigned int minValue,
    unsigned int maxValue,
    unsigned int& result
) {
    std::cout << prompt;

    std::string input;
    if (!std::getline(std::cin, input)) {
        return false;
    }

    if (input.empty() ||
        input.find_first_not_of("0123456789")
            != std::string::npos) {
        std::cout << "[ERROR] Enter a valid number.\n";
        return false;
    }

    try {
        std::size_t processed = 0;
        unsigned long value = std::stoul(input, &processed);

        if (processed != input.size() ||
            value < minValue || value > maxValue) {
            std::cout << "[ERROR] Value outside allowed range.\n";
            return false;
        }

        result = static_cast<unsigned int>(value);
        return true;
    }
    catch (const std::exception&) {
        std::cout << "[ERROR] Invalid number.\n";
        return false;
    }
}

void showScannerSettings(AppSettings& settings) {
    while (true) {
        std::cout << "\n========== SCANNER SETTINGS ==========\n";
        std::cout << "1. Maximum Threads: " << settings.maxThreads << '\n';
        std::cout << "2. TCP Timeout: " << settings.tcpTimeoutMs << " ms\n";
        std::cout << "3. ICMP Timeout: " << settings.icmpTimeoutMs << " ms\n";
        std::cout << "4. Progress Bars: "
                  << (settings.showProgress ? "On" : "Off") << '\n';
        std::cout << "0. Back\n";
        std::cout << "Select: ";

        std::string input;
        if (!std::getline(std::cin, input)) return;

        unsigned int value = 0;

        if (input == "1") {
            if (readSettingNumber("Threads (1-64): ", 1, 64, value))
                settings.maxThreads = value;
        }
        else if (input == "2") {
            if (readSettingNumber("TCP Timeout (100-10000 ms): ",
                                  100, 10000, value))
                settings.tcpTimeoutMs = value;
        }
        else if (input == "3") {
            if (readSettingNumber("ICMP Timeout (100-10000 ms): ",
                                  100, 10000, value))
                settings.icmpTimeoutMs = value;
        }
        else if (input == "4") {
            settings.showProgress = !settings.showProgress;
        }
        else if (input == "0") {
            return;
        }
        else {
            std::cout << "[ERROR] Invalid selection.\n";
        }
    }
}

void showDiscoverySettings(AppSettings& settings) {
    while (true) {
        std::cout << "\n========== DISCOVERY SETTINGS ==========\n";
        std::cout << "1. ICMP: "
                  << (settings.enableICMP ? "On" : "Off") << '\n';
        std::cout << "2. ARP: "
                  << (settings.enableARP ? "On" : "Off") << '\n';
        std::cout << "3. TCP: "
                  << (settings.enableTCP ? "On" : "Off") << '\n';
        std::cout << "0. Back\n";
        std::cout << "Select: ";

        std::string input;
        if (!std::getline(std::cin, input)) return;

        if (input == "0") return;

        bool* selected = nullptr;

        if (input == "1") selected = &settings.enableICMP;
        else if (input == "2") selected = &settings.enableARP;
        else if (input == "3") selected = &settings.enableTCP;
        else {
            std::cout << "[ERROR] Invalid selection.\n";
            continue;
        }

        int enabledCount =
            static_cast<int>(settings.enableICMP) +
            static_cast<int>(settings.enableARP) +
            static_cast<int>(settings.enableTCP);

        if (*selected && enabledCount == 1) {
            std::cout << "[ERROR] At least one method must remain enabled.\n";
            continue;
        }

        *selected = !*selected;
    }
}

void showSettings(AppSettings& settings) {
    while (true) {
        std::cout << "\n========== SETTINGS ==========\n";
        std::cout << "1. Scanner Settings\n";
        std::cout << "2. Discovery Settings\n";
        std::cout << "3. Show All Adapters: "
                  << (settings.showAllAdapters ? "Yes" : "No") << '\n';
        std::cout << "4. Reset to Defaults\n";
        std::cout << "0. Back\n";
        std::cout << "Select: ";

        std::string input;
        if (!std::getline(std::cin, input)) return;

        if (input == "1") {
            showScannerSettings(settings);
        }
        else if (input == "2") {
            showDiscoverySettings(settings);
        }
        else if (input == "3") {
            settings.showAllAdapters = !settings.showAllAdapters;
        }
        else if (input == "4") {
            settings = AppSettings{};
            std::cout << "[OK] Defaults restored.\n";
        }
        else if (input == "0") {
            return;
        }
        else {
            std::cout << "[ERROR] Invalid selection.\n";
        }
    }
}

int main() {
    // Define variables
    std::string appName = "NetScope";
    std::string appDescription = "Network Toolkit";
    std::string appVersion = "0.8";
    bool running = true;
    int choice;
    AppSettings settings;

    if (!initializeNetworking()) {
    std::cerr << "[ERROR] Unable to initialize networking.\n";
    return 1;
}
    
    while (running) {
        // Display main menu
        std::cout << "\n" << appName << " v" << appVersion << '\n';
        std::cout << appDescription << '\n';

        std::cout << "\n============================\n";
        std::cout << "        MAIN MENU\n";
        std::cout << "============================\n\n";

        std::cout << "1. Show local network information\n";
        std::cout << "2. Network tools\n";
        std::cout << "3. Settings\n";
        std::cout << "4. About NetScope\n";
        std::cout << "0. Exit\n";

        // Get user input
        std::string input;

        std::cout << "\nSelect an option: " << std::flush;

        // Check if input stream is available
        if (!std::getline(std::cin, input)) {
            std::cout << "\nInput closed. Exiting NetScope.\n";
            break;
        }

        // Handle empty input
        if (input.empty()) {
            std::cout << "\n[ERROR] Please select a menu option.\n";
            continue;
        }

        // Convert input into integer
        std::stringstream ss(input);

        if (!(ss >> choice)) {
            std::cout << "\n[ERROR] Please enter a valid number.\n";
            continue;
        }

        // Reject extra characters after the menu selection
        std::string extra;

        if (ss >> extra) {
            std::cout << "\n[ERROR] Invalid menu selection.\n";
            continue;
        }

        // Define choices
        switch (choice) {
            case 1:
                showLocalNetworkInformation(settings);
                break;

            case 2:
                showNetworkTools(settings);
                break;

            case 3:
                showSettings(settings);
                break;

            case 4:
                std::cout << "\nNetScope - A C++ networking toolkit.\n";
                break;

            case 0:
                std::cout << "\nExiting " << appName << " v" << appVersion << "\nGoodbye!\n";
                running = false;
                break;

            default:
                std::cout << "\nInvalid option! Please try again.\n";
                break;
        }
    }

    cleanupNetworking();
    return 0;
}
