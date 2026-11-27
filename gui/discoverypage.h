
#pragma once

#include <QWidget>
#include <QPointer>

#include "discoveryworker.h"
#include "settings.h"

class QLineEdit;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QLabel;
class QTableWidget;

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

    AppSettings settings_;

    QLineEdit* ipInput_ = nullptr;
    QSpinBox* prefixInput_ = nullptr;

    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;

    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;

    QTableWidget* results_ = nullptr;

    QPointer<DiscoveryWorker> worker_;
};
