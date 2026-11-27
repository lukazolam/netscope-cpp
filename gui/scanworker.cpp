
#include "scanworker.h"

#include <utility>

ScanWorker::ScanWorker(
    std::string ip,
    std::vector<int> ports,
    AppSettings settings,
    QObject* parent
)
    : QThread(parent),
      ip_(std::move(ip)),
      ports_(std::move(ports)),
      settings_(settings) {
}

const std::vector<PortScanResult>&
ScanWorker::results() const {
    return results_;
}

void ScanWorker::run() {
    results_ = scanTCPPorts(
        ip_,
        ports_,
        settings_,
        [this](std::size_t completed, std::size_t total) {
            if (total == 0) {
                return;
            }

            int percentage = static_cast<int>(
                completed * 100 / total
            );

            emit progressUpdated(percentage);
        }
    );
}
