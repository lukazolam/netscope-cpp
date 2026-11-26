# netscope-cpp
A modular network diagnostics toolkit written in modern C++

Implemented Functions:
- Systeminfo
    - Show network adapters
- Network Tools
    - Validate IPv4 address
    - Subnet calculator
    - TCP port scanner
        - Single Port
        - Port Range (max. 10000 Ports)
        - Common Ports
    - Network Discovery
        - /24 - /32
        - Mac Address included
        - Reverse DNS Lookup
        - ARP Lookup
        - ICMP Lookup
- Settings
    - Scanner Settings
        - Maximum Threads (for scanning and port probe, Default 32)
        - TCP Probe Timeout (Default 2000ms)
        - ICMP Probe Timeout (Default 1000ms)
        - Progress bars (Default on)
    - Discovery Settings (ICMP, ARP, TCP; Default All On)
    - Show all Adapters (Default No)

