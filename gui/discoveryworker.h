
#pragma once

#include <QThread>

#include <atomic>
#include <string>
#include <vector>
#include <QMetaType>

#include "network.h"
#include "settings.h"
Q_DECLARE_METATYPE(DiscoveredHost)

class DiscoveryWorker : public QThread {
    Q_OBJECT

public:
    DiscoveryWorker(
        std::string ip,
        int prefix,
        AppSettings settings,
        QObject* parent = nullptr
    );

    void requestStop();

    bool wasStopped() const;
    std::size_t addressesChecked() const;
    std::size_t totalAddresses() const;

    const std::vector<DiscoveredHost>& results() const;

signals:
    void progressUpdated(int percentage);
    void statusUpdated(const QString& message);

    // A newly detected device
    void hostFound(DiscoveredHost host);

    // Reverse DNS has supplied a hostname
    void hostnameResolved(
        QString ip,
        QString hostname
    );

protected:
    void run() override;

private:
    std::string ip_;
    int prefix_;
    AppSettings settings_;

    std::atomic<bool> stopRequested_{false};
    std::atomic<std::size_t> checked_{0};

    std::size_t total_ = 0;
    std::vector<DiscoveredHost> results_;
};
