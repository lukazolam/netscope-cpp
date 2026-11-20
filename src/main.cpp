
#include <iostream>
#include <string>
#include <sstream>

#include "network.h"


// Network tools function
void showNetworkTools() {
    std::string ip;

    std::cout << "\n========== NETWORK TOOLS ==========\n";
    std::cout << "Enter an IPv4 address (or 'back' to return): " << std::flush;

    // Read input and check if the input stream is still available
    if (!std::getline(std::cin, ip)) {
        return;
    }

    // Handle empty input
    if (ip.empty()) {
        std::cout << "\n[ERROR] No IPv4 address entered.\n";
        return;
    }

    // Return to main menu
    if (ip == "back") {
        return;
    }

    // Validate entered IPv4 address
    if (validateIPv4(ip)) {
        std::cout << "\n[OK] " << ip << " is a valid IPv4 address.\n";
    }
    else {
        std::cout << "\n[ERROR] " << ip << " is not a valid IPv4 address.\n";
    }
}


int main() {
    // Define variables
    std::string appName = "NetScope";
    std::string appDescription = "Homelab Network Toolkit";
    std::string appVersion = "0.3.2";

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
