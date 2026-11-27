
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
    // All network operations happen off the GUI thread
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

        // Probe progress: first 90%
        [this](std::size_t completed, std::size_t total) {
            checked_.store(completed);

            int percentage = static_cast<int>(
                completed * 90 / total
            );

            emit progressUpdated(percentage);
        },

        // Stop callback
        [this]() {
            return stopRequested_.load();
        }
    );

    if (stopRequested_.load()) {
        emit statusUpdated("Discovery stopped.");
        return;
    }

    // DNS lookups can take time, so do them here,
    // never on the GUI thread.
    emit statusUpdated("Resolving hostnames...");

    const std::size_t count = results_.size();

    if (count == 0) {
        emit progressUpdated(100);
        return;
    }

    for (std::size_t i = 0; i < count; ++i) {
        if (stopRequested_.load()) {
            break;
        }

        results_[i].hostname =
            resolveHostname(results_[i].ipAddress);

        int percentage = 90 + static_cast<int>(
            ((i + 1) * 10) / count
        );

        emit progressUpdated(percentage);
    }
}
