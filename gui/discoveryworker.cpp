
#include "discoveryworker.h"

#include <algorithm>
#include <utility>

DiscoveryWorker::DiscoveryWorker(
    std::string ip,
    int prefix,
    AppSettings settings,
    QObject* parent
)
    : QThread(parent),
      ip_(std::move(ip)),
      prefix_(prefix),
      settings_(settings) {
}

void DiscoveryWorker::requestStop() {
    stopRequested_.store(true);
}

bool DiscoveryWorker::wasStopped() const {
    return stopRequested_.load();
}

std::size_t DiscoveryWorker::addressesChecked() const {
    return checked_.load();
}

std::size_t DiscoveryWorker::totalAddresses() const {
    return total_;
}

const std::vector<DiscoveredHost>&
DiscoveryWorker::results() const {
    return results_;
}


void DiscoveryWorker::run() {
    // Generate the subnet's usable IP addresses
    std::vector<std::string> addresses =
        generateHostIPs(ip_, prefix_);

    total_ = addresses.size();

    if (addresses.empty()) {
        emit statusUpdated("No valid addresses to scan.");
        return;
    }

    emit statusUpdated("Probing network devices...");

    results_ = discoverHosts(
        addresses,
        settings_,

        // Progress callback
        [this](std::size_t completed, std::size_t total) {
            checked_.store(completed);

            int percentage = static_cast<int>(
                completed * 90 / total
            );

            emit progressUpdated(percentage);
        },

        // Cancellation callback
        [this]() {
            return stopRequested_.load();
        },

        // Newly discovered host callback
        [this](const DiscoveredHost& host) {
            emit hostFound(host);
        }
    );

    if (stopRequested_.load()) {
        emit statusUpdated("Discovery stopped.");
        return;
    }

    emit statusUpdated("Resolving hostnames...");

    const std::size_t count = results_.size();

    if (count == 0) {
        emit progressUpdated(100);
        return;
    }

    // Resolve DNS without blocking the GUI thread
    for (std::size_t i = 0; i < count; ++i) {
        if (stopRequested_.load()) {
            break;
        }

        DiscoveredHost& host = results_[i];

        host.hostname = resolveHostname(host.ipAddress);

        // Update the corresponding GUI row
        emit hostnameResolved(
            QString::fromStdString(host.ipAddress),
            QString::fromStdString(host.hostname)
        );

        int percentage = 90 + static_cast<int>(
            ((i + 1) * 10) / count
        );

        emit progressUpdated(percentage);
    }
}

