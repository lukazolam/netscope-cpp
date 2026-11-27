
#pragma once

#include <QThread>

#include <atomic>
#include <string>
#include <vector>

#include "network.h"
#include "settings.h"

class ScanWorker : public QThread {
    Q_OBJECT

public:
    ScanWorker(
        std::string ip,
        std::vector<int> ports,
        AppSettings settings,
        QObject* parent = nullptr
    );

    const std::vector<PortScanResult>& results() const;
    void requestStop();
    bool wasStopped() const;

signals:
    void progressUpdated(int percentage);

protected:
    void run() override;

private:
    std::string ip_;
    std::vector<int> ports_;
    AppSettings settings_;

    std::vector<PortScanResult> results_;
    std::atomic<bool> stopRequested_{false};
};
