
#pragma once

#include <QWidget>
#include <QPointer>

#include <vector>

#include "settings.h"
#include "scanworker.h"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QProgressBar;
class QTableWidget;

class PortScannerPage : public QWidget {
public:
    explicit PortScannerPage(
        const AppSettings& settings,
        QWidget* parent = nullptr
    );

    ~PortScannerPage() override;

private:
    // Scanner operations
    void startScan();
    void stopScan();
    void finishScan(ScanWorker* worker, int totalPorts);

    // Interface helpers
    void updatePortFields(int mode);
    void setScanning(bool scanning);

    // Scanner configuration
    AppSettings settings_;

    // Input controls
    QLineEdit* ipInput_ = nullptr;
    QComboBox* scanMode_ = nullptr;
    QSpinBox* startPort_ = nullptr;
    QSpinBox* endPort_ = nullptr;

    QLabel* startLabel_ = nullptr;
    QLabel* endLabel_ = nullptr;

    // Scan controls and results
    QPushButton* scanButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTableWidget* results_ = nullptr;

    // Currently running scan
    QPointer<ScanWorker> worker_;
};
