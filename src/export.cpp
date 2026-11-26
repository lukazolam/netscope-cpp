#include "export.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

std::string statusOf(const DiscoveredHost& host) {
    return host.responding ? "Confirmed" : "ARP cached";
}

std::string methodsOf(const DiscoveredHost& host) {
    std::string methods;
    if (host.icmpResponded) methods += "ICMP";
    if (host.arpFound) {
        if (!methods.empty()) methods += ",";
        methods += host.arpConfirmed ? "ARP" : "ARP cached";
    }
    if (host.tcpResponded) {
        if (!methods.empty()) methods += ",";
        methods += "TCP";
    }
    return methods;
}

std::string csvCell(std::string value) {
    // Prevent spreadsheet formula evaluation in cells derived from network data.
    auto first = std::find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    if (first != value.end() &&
        (*first == '=' || *first == '+' || *first == '-' || *first == '@')) {
        value.insert(0, "'");
    }

    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') out += "\"\"";
        else out += ch;
    }
    return out + '"';
}

std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0') << static_cast<int>(ch)
                        << std::dec;
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    out << '"';
    return out.str();
}

} // namespace

bool exportDiscoveryResults(
    const std::vector<DiscoveredHost>& hosts,
    const std::string& filename
) {
    if (filename.empty()) return false;

    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const bool csv = lower.size() >= 4 && lower.substr(lower.size() - 4) == ".csv";
    const bool json = lower.size() >= 5 && lower.substr(lower.size() - 5) == ".json";
    if (!csv && !json) return false;

    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;

    if (csv) {
        out << "IPAddress,MACAddress,Hostname,Status,DiscoveryMethods\n";
        for (const auto& host : hosts) {
            out << csvCell(host.ipAddress) << ','
                << csvCell(host.macAddress) << ','
                << csvCell(host.hostname) << ','
                << csvCell(statusOf(host)) << ','
                << csvCell(methodsOf(host)) << '\n';
        }
    } else {
        out << "{\n  \"hosts\": [\n";
        for (std::size_t i = 0; i < hosts.size(); ++i) {
            const auto& host = hosts[i];
            if (i > 0) out << ",\n";
            out << "    {\"ip\": " << jsonString(host.ipAddress)
                << ", \"mac\": " << jsonString(host.macAddress)
                << ", \"hostname\": " << jsonString(host.hostname)
                << ", \"status\": " << jsonString(statusOf(host))
                << ", \"methods\": " << jsonString(methodsOf(host))
                << ", \"confirmed\": " << (host.responding ? "true" : "false")
                << "}";
        }
        out << "\n  ]\n}\n";
    }
    out.close();
    return !out.fail();
}
