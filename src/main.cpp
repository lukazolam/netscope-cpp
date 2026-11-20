#include <iostream>
#include <string>

int main() {
    //define variables
    std::string appName = "NetScope";
    std::string appDescription = "Homelab Network Toolkit";
    double appVersion = 0.2.1;
    bool running = true;
    int choice;

    while (running) {
        //display main menu
        std::cout << appName << " v" << appVersion << '\n';
        std::cout << appDescription << '\n';
        std::cout << "\n============================\n";
        std::cout << "        MAIN MENU\n";
        std::cout << "============================\n\n";

        std::cout << "1. Show local network information\n";
        std::cout << "2. Network tools\n";
        std::cout << "3. Settings\n";
        std::cout << "4. About NetScope\n";
        std::cout << "0. Exit\n";
    
        //get user input
        std::cout << "\nSelect an option: ";

        if (!(std::cin >> choice)) {
            std::cout << "\nInvalid input! Please enter a number.\n";

            if (std::cin.eof()) {
                std::cout << "\nInput closed. Exiting NetScope.\n";
                break;
            }
            // Reset the input stream's error state
            std::cin.clear();

            // Discard the remaining input on the current line
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Skip the rest of this loop iteration
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        //define choices
        switch (choice) {
            case 1:
                std::cout << "\nLocal network information is not implemented yet.\n";
                break;

            case 2:
                std::cout << "\nNetwork tools are not implemented yet.\n";
                break;

            case 3:
                std::cout << "\nSettings are not available yet.\n";
                break;

            case 4:
                std::cout << "\nNetScope - A C++ networking toolkit.\n";
                break;

            case 0:
                std::cout << "\nExiting NetScope v0.2. Goodbye!\n";
                running = false;
                break;

            default:
                std::cout << "\nInvalid option! Please try again.\n";
                break;
        }
       
    }
    return 0;
}
