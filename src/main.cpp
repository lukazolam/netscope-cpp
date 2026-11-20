#include <iostream>
#include <string>

int main() {
    std::string appName = "NetScope";
    std::string appDescription = "Homelab Network Toolkit";
    double appVersion = 0.1;
    bool running = true;
    int choice;

    while (running) {
        std::cout << appName << " v" << appVersion << '\n';
        std::cout << appDescription << '\n';
        std::cout << "\n============================\n";
        std::cout << "        MAIN MENU\n";
        std::cout << "============================\n\n";

        std::cout << "1. Show local network information\n";
        std::cout << "2. Network tools\n";
        std::cout << "3. About NetScope\n";
        std::cout << "0. Exit\n";
    
        std::cout << "\nSelect an option: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                std::cout << "\nLocal network information is not implemented yet.\n";
                break;

            case 2:
                std::cout << "\nNetwork tools are not implemented yet.\n";
                break;

            case 3:
                std::cout << "\nNetScope - A C++ networking toolkit.\n";
                break;

            case 0:
                std::cout << "\nGoodbye!\n";
                running = false;
                break;

            default:
                std::cout << "\nInvalid option! Please try again.\n";
                break;
        }


    }
    return 0;
}
