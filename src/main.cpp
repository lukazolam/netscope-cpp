#include <iostream>
#include <string>

int main() {
    std::string appName = "NetScope";
    std::string appDescription = "Homelab Network Toolkit";
    double appVersion = 0.1;

    std::cout << appName << " v" << appVersion << '\n';
    std::cout << appDescription << '\n';
    std::cout << "\n============================\n";
    std::cout << "        MAIN MENU\n";
    std::cout << "============================\n\n";

    std::cout << "1. Show local network information\n";
    std::cout << "2. Network tools\n";
    std::cout << "0. Exit\n";

    std::cout << "\nSelect an option: ";

    return 0;
}
