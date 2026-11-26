#include "export.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::string readAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int main() {
    DiscoveredHost host{};
    host.ipAddress = "192.168.0.10";
    host.macAddress = "aa:bb:cc:dd:ee:ff";
    host.hostname = "=HYPERLINK(\"http://example.test\",\"click\")";
    host.responding = true;
    host.icmpResponded = true;
    host.arpFound = true;

    const std::vector<DiscoveredHost> hosts{host};
    const std::string csv = "netscope-export-test.csv";
    const std::string json = "netscope-export-test.json";
    int failures = 0;
    auto check = [&](bool ok) { if (!ok) ++failures; };

    check(exportDiscoveryResults(hosts, csv));
    const auto csvText = readAll(csv);
    check(csvText.find("\"'=HYPERLINK(\"\"http://example.test\"\"") != std::string::npos);
    check(csvText.find("\"ICMP,ARP cached\"") != std::string::npos);
    check(exportDiscoveryResults(hosts, json));
    const auto jsonText = readAll(json);
    check(jsonText.find("\\\"http://example.test\\\"") != std::string::npos);
    check(jsonText.find("\"confirmed\": true") != std::string::npos);
    check(!exportDiscoveryResults(hosts, "netscope-export-test.txt"));
    check(exportDiscoveryResults({}, csv));
    check(readAll(csv) == "IPAddress,MACAddress,Hostname,Status,DiscoveryMethods\n");

    std::remove(csv.c_str());
    std::remove(json.c_str());
    return failures == 0 ? 0 : 1;
}
