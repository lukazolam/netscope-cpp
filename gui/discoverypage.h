
#pragma once

#include <QWidget>
#include <QPointer>
#include <string>
#include <vector>

#include "discoveryworker.h"
#include "settings.h"

class QLineEdit;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QLabel;
class QTableWidget;
class QCheckBox;

class DiscoveryPage : public QWidget {
public:
    explicit DiscoveryPage(
        const AppSettings& settings,
        QWidget* parent = nullptr
    );

    ~DiscoveryPage() override;

private:
    void startDiscovery();
    void stopDiscovery();
    void finishDiscovery(DiscoveryWorker* worker);
    void setScanning(bool scanning);
    void appendHost(const DiscoveredHost& host);
    void updateHostname(
        const QString& ip,
        const QString& hostname
    );

    void rebuildResultsTable();
    void exportResults(bool json);

    AppSettings settings_;

    QLineEdit* ipInput_ = nullptr;
    QSpinBox* prefixInput_ = nullptr;

    QCheckBox* icmpCheck_ = nullptr;
    QCheckBox* arpCheck_ = nullptr;
    QCheckBox* tcpCheck_ = nullptr;

    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;

    QPushButton* exportCsvButton_ = nullptr;
    QPushButton* exportJsonButton_ = nullptr;

    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;

    QTableWidget* results_ = nullptr;

    QPointer<DiscoveryWorker> worker_;

    std::vector<DiscoveredHost> hosts_;
};
