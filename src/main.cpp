
#include <iostream>
#include <string>
#include <sstream>

#include "network.h"


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


void showTCPScanner() {
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

    PortStatus result = scanTCPPort(ip, port);

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


void showNetworkTools() {
    bool running = true;

    while (running) {
        std::cout << "\n========== NETWORK TOOLS ==========\n";
        std::cout << "1. Validate IPv4 Address\n";
        std::cout << "2. Calculate Subnet\n";
        std::cout << "3. TCP Port Scanner\n";
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
            showTCPScanner();
        }
        else if (input == "0") {
            running = false;
        }
        else {
            std::cout << "\n[ERROR] Invalid option.\n";
        }
    }
}

int main() {
    // Define variables
    std::string appName = "NetScope";
    std::string appDescription = "Homelab Network Toolkit";
    std::string appVersion = "0.5.1";

    bool running = true;
    int choice;

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
                std::cout << "\nLocal network information is not implemented yet.\n";
                break;

            case 2:
                showNetworkTools();
                break;

            case 3:
                std::cout << "\nSettings are not available yet.\n";
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

    return 0;
}
