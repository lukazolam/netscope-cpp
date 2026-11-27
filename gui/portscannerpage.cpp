
#include "portscannerpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QFont>

#include <string>
#include <vector>

#include "network.h"

PortScannerPage::PortScannerPage(
    const AppSettings& settings,
    QWidget* parent
)
    : QWidget(parent),
      settings_(settings) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* title = new QLabel("TCP Port Scanner");

    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    layout->addWidget(title);

    // Target IP
    layout->addWidget(new QLabel("Target IPv4 Address:"));

    ipInput_ = new QLineEdit();
    ipInput_->setPlaceholderText(
        "Enter IPv4 address (e.g. 127.0.0.1)"
    );

    layout->addWidget(ipInput_);

    // Scan mode
    layout->addWidget(new QLabel("Scan Mode:"));

    scanMode_ = new QComboBox();
    scanMode_->addItems({
        "Common Ports",
        "Single Port",
        "Port Range",
        "Full Scan (1-65535)"
    });

    layout->addWidget(scanMode_);

    // Port selection
    auto* portLayout = new QHBoxLayout();

    startLabel_ = new QLabel("Starting Port:");
    startPort_ = new QSpinBox();
    startPort_->setRange(1, 65535);
    startPort_->setValue(1);

    endLabel_ = new QLabel("Ending Port:");
    endPort_ = new QSpinBox();
    endPort_->setRange(1, 65535);
    endPort_->setValue(1024);

    portLayout->addWidget(startLabel_);
    portLayout->addWidget(startPort_);
    portLayout->addWidget(endLabel_);
    portLayout->addWidget(endPort_);
    portLayout->addStretch();

    layout->addLayout(portLayout);

    // Scan buttons
    auto* buttonLayout = new QHBoxLayout();

    scanButton_ = new QPushButton("Start Scan");
    stopButton_ = new QPushButton("Stop Scan");

    stopButton_->setEnabled(false);

    buttonLayout->addWidget(scanButton_);
    buttonLayout->addWidget(stopButton_);

    layout->addLayout(buttonLayout);

    // Progress indicator
    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    layout->addWidget(progressBar_);

    // Status label
    statusLabel_ = new QLabel("Ready to scan.");
    layout->addWidget(statusLabel_);

    // Results
    results_ = new QTableWidget();
    results_->setColumnCount(3);

    results_->setHorizontalHeaderLabels({
        "Port", "Protocol", "Status"
    });

    results_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    results_->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    results_->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    results_->setSortingEnabled(true);

    layout->addWidget(results_);

    // Connect interface events
    connect(
        scanMode_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int mode) {
            updatePortFields(mode);
        }
    );

    connect(
        scanButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            startScan();
        }
    );

    connect(
        stopButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            stopScan();
        }
    );

    updatePortFields(scanMode_->currentIndex());
}

PortScannerPage::~PortScannerPage() {
    // Ensure the worker has finished before destroying the page
    if (worker_) {
        worker_->requestStop();
        worker_->wait();
    }
}

void PortScannerPage::updatePortFields(int mode) {
    bool single = mode == 1;
    bool range = mode == 2;

    startLabel_->setVisible(single || range);
    startPort_->setVisible(single || range);

    endLabel_->setVisible(range);
    endPort_->setVisible(range);

    startLabel_->setText(
        single ? "Port:" : "Starting Port:"
    );
}

void PortScannerPage::setScanning(bool scanning) {
    scanButton_->setEnabled(!scanning);
    stopButton_->setEnabled(scanning);

    ipInput_->setEnabled(!scanning);
    scanMode_->setEnabled(!scanning);

    startPort_->setEnabled(!scanning);
    endPort_->setEnabled(!scanning);
}

void PortScannerPage::startScan() {
    if (worker_) {
        return;
    }

    std::string ip =
        ipInput_->text().trimmed().toStdString();

    if (!validateIPv4(ip)) {
        QMessageBox::warning(
            this,
            "Invalid IP Address",
            "Please enter a valid IPv4 address."
        );
        return;
    }

    // Build the port list
    std::vector<int> ports;
    int mode = scanMode_->currentIndex();

    if (mode == 0) {
        ports = {
            21, 22, 23, 25, 53,
            80, 110, 143, 443, 445,
            993, 995, 1433, 3306, 3389,
            5432, 5900, 6379, 8080, 8443
        };
    }
    else if (mode == 1) {
        ports.push_back(startPort_->value());
    }
    else {
        int first = mode == 3
            ? 1 : startPort_->value();

        int last = mode == 3
            ? 65535 : endPort_->value();

        if (first > last) {
            QMessageBox::warning(
                this,
                "Invalid Port Range",
                "Starting port cannot exceed ending port."
            );
            return;
        }

        ports.reserve(last - first + 1);

        for (int port = first; port <= last; ++port) {
            ports.push_back(port);
        }
    }

    const int totalPorts = static_cast<int>(ports.size());

    // Prepare page for scanning
    setScanning(true);
    progressBar_->setValue(0);
    results_->setRowCount(0);

    statusLabel_->setText(
        QString("Scanning %1 TCP ports on %2...")
            .arg(totalPorts)
            .arg(QString::fromStdString(ip))
    );

    ScanWorker* worker = new ScanWorker(
        ip,
        std::move(ports),
        settings_,
        this
    );

    worker_ = worker;

    // Progress is delivered to the GUI thread
    connect(
        worker,
        &ScanWorker::progressUpdated,
        this,
        [this](int percentage) {
            if (percentage > progressBar_->value()) {
                progressBar_->setValue(percentage);
            }
        }
    );

    // Handle completed scans
    connect(
        worker,
        &QThread::finished,
        this,
        [this, worker, totalPorts]() {
            finishScan(worker, totalPorts);

            worker_ = nullptr;
            worker->deleteLater();
        }
    );

    worker->start();
}

void PortScannerPage::stopScan() {
    if (!worker_) {
        return;
    }

    worker_->requestStop();

    stopButton_->setEnabled(false);

    statusLabel_->setText(
        "Stopping scan. Finishing active connections..."
    );
}

void PortScannerPage::finishScan(
    ScanWorker* worker,
    int totalPorts
) {
    int openCount = 0;
    int closedCount = 0;
    int timeoutCount = 0;
    int errorCount = 0;

    results_->setSortingEnabled(false);

    for (const PortScanResult& result : worker->results()) {
        switch (result.status) {
            case PortStatus::Open: {
                ++openCount;

                int row = results_->rowCount();
                results_->insertRow(row);

                auto* portItem = new QTableWidgetItem();
                portItem->setData(Qt::EditRole, result.port);

                results_->setItem(row, 0, portItem);

                results_->setItem(
                    row, 1,
                    new QTableWidgetItem("TCP")
                );

                results_->setItem(
                    row, 2,
                    new QTableWidgetItem("Open")
                );

                break;
            }

            case PortStatus::Closed:
                ++closedCount;
                break;

            case PortStatus::Timeout:
                ++timeoutCount;
                break;

            case PortStatus::Error:
                ++errorCount;
                break;
        }
    }

    results_->setSortingEnabled(true);

    int scanned =
        static_cast<int>(worker->results().size());

    if (worker->wasStopped()) {
        statusLabel_->setText(
            QString(
                "Stopped. Checked: %1/%2 | Open: %3 | "
                "Closed: %4 | Timeouts: %5 | Errors: %6"
            )
            .arg(scanned)
            .arg(totalPorts)
            .arg(openCount)
            .arg(closedCount)
            .arg(timeoutCount)
            .arg(errorCount)
        );
    }
    else {
        progressBar_->setValue(100);

        statusLabel_->setText(
            QString(
                "Complete. Checked: %1 | Open: %2 | "
                "Closed: %3 | Timeouts: %4 | Errors: %5"
            )
            .arg(scanned)
            .arg(openCount)
            .arg(closedCount)
            .arg(timeoutCount)
            .arg(errorCount)
        );
    }

    setScanning(false);
}
