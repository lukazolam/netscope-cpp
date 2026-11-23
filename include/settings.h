
#pragma once

struct AppSettings {
    unsigned int maxThreads = 32;
    unsigned int tcpTimeoutMs = 2000;
    unsigned int icmpTimeoutMs = 1000;

    bool enableICMP = true;
    bool enableARP = true;
    bool enableTCP = true;

    bool showAllAdapters = false;
    bool showProgress = true;
};
