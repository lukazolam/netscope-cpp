
#include "discoverypage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QFont>
#include <QStringList>
#include <vector>

#include <string>

#include "network.h"
#include "macvendor.h"

namespace {

QString displayText(const std::string& value) {
    return value.empty()
        ? QString("-")
        : QString::fromStdString(value);
}

QString discoveryMethods(const DiscoveredHost& host) {
    QStringList methods;

    if (host.icmpResponded) {
        methods << "ICMP";
    }

    if (host.arpFound) {
        methods << (
            host.arpConfirmed
                ? "ARP confirmed"
                : "ARP cached"
        );
    }

    if (host.tcpResponded) {
        methods << "TCP";
    }

    return methods.isEmpty()
        ? QString("-")
        : methods.join(", ");
}

} // namespace

DiscoveryPage::DiscoveryPage(
    const AppSettings& settings,
    QWidget* parent
)
    : QWidget(parent),
      settings_(settings) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* title = new QLabel("Network Discovery");

    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    layout->addWidget(title);

    // Network input
    auto* inputLayout = new QHBoxLayout();

    ipInput_ = new QLineEdit();
    ipInput_->setPlaceholderText(
        "IPv4 address (e.g. 192.168.0.1)"
    );

    prefixInput_ = new QSpinBox();
    prefixInput_->setRange(22, 32);
    prefixInput_->setValue(24);
    prefixInput_->setPrefix("/");

    inputLayout->addWidget(
        new QLabel("Target:")
    );

    inputLayout->addWidget(ipInput_, 1);

    inputLayout->addWidget(
        new QLabel("CIDR:")
    );

    inputLayout->addWidget(prefixInput_);

    layout->addLayout(inputLayout);

    // Action buttons
    auto* buttons = new QHBoxLayout();

    startButton_ =
        new QPushButton("Start Discovery");

    stopButton_ =
        new QPushButton("Stop");

    stopButton_->setEnabled(false);

    buttons->addWidget(startButton_);
    buttons->addWidget(stopButton_);

    layout->addLayout(buttons);

    // Progress
    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    layout->addWidget(progressBar_);

    statusLabel_ = new QLabel("Ready.");
    countLabel_ = new QLabel("No scan performed.");

    layout->addWidget(statusLabel_);
    layout->addWidget(countLabel_);

    // Results table
    results_ = new QTableWidget();

    results_->setColumnCount(6);
    results_->setHorizontalHeaderLabels({
        "IP Address",
        "Hostname",
        "MAC Address",
        "Vendor",
        "Methods",
        "Responding"
    });

    results_->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );

    results_->horizontalHeader()
        ->setStretchLastSection(true);

    results_->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    results_->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    results_->setSortingEnabled(true);

    layout->addWidget(results_, 1);

    // Start discovery
    connect(
        startButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            startDiscovery();
        }
    );

    // Stop discovery
    connect(
        stopButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            stopDiscovery();
        }
    );
}

DiscoveryPage::~DiscoveryPage() {
    if (worker_) {
        worker_->requestStop();
        worker_->wait();
    }
}

void DiscoveryPage::setScanning(bool scanning) {
    startButton_->setEnabled(!scanning);
    stopButton_->setEnabled(scanning);

    ipInput_->setEnabled(!scanning);
    prefixInput_->setEnabled(!scanning);
}

void DiscoveryPage::startDiscovery() {
    if (worker_) {
        return;
    }

    std::string ip =
        ipInput_->text().trimmed().toStdString();

    if (!validateIPv4(ip)) {
        QMessageBox::warning(
            this,
            "Invalid IPv4",
            "Please enter a valid IPv4 address."
        );
        return;
    }

    int prefix = prefixInput_->value();

    // Reuse the existing subnet address generator
    std::vector<std::string> addresses =
        generateHostIPs(ip, prefix);

    if (addresses.empty()) {
        QMessageBox::warning(
            this,
            "Invalid Network",
            "No addresses could be generated."
        );
        return;
    }

    int total = static_cast<int>(addresses.size());

    // Prepare the interface
    setScanning(true);
    progressBar_->setValue(0);
    results_->setRowCount(0);

    countLabel_->setText(
        QString("Addresses to check: %1").arg(total)
    );

    statusLabel_->setText("Starting discovery...");

    auto* worker = new DiscoveryWorker(
        ip, prefix, settings_, this
    );

    worker_ = worker;

    connect(
        worker,
        &DiscoveryWorker::progressUpdated,
        this,
        [this](int percentage) {
            if (percentage > progressBar_->value()) {
                progressBar_->setValue(percentage);
            }
        }
    );

    connect(
        worker,
        &DiscoveryWorker::statusUpdated,
        this,
        [this](const QString& message) {
            statusLabel_->setText(message);
        }
    );

    connect(
        worker,
        &QThread::finished,
        this,
        [this, worker]() {
            finishDiscovery(worker);

            worker_ = nullptr;
            worker->deleteLater();
        }
    );

    worker->start();
}

void DiscoveryPage::stopDiscovery() {
    if (!worker_) {
        return;
    }

    worker_->requestStop();
    stopButton_->setEnabled(false);

    statusLabel_->setText(
        "Stopping. Waiting for active probes..."
    );
}

void DiscoveryPage::finishDiscovery(
    DiscoveryWorker* worker
) {
    results_->setSortingEnabled(false);
    results_->setRowCount(0);

    const auto& hosts = worker->results();

    for (const DiscoveredHost& host : hosts) {
        int row = results_->rowCount();
        results_->insertRow(row);

        results_->setItem(
            row, 0,
            new QTableWidgetItem(
                displayText(host.ipAddress)
            )
        );

        results_->setItem(
            row, 1,
            new QTableWidgetItem(
                displayText(host.hostname)
            )
        );

        results_->setItem(
            row, 2,
            new QTableWidgetItem(
                displayText(host.macAddress)
            )
        );

        results_->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::fromStdString(
                    lookupMACVendor(host.macAddress)
                )
            )
        );

        results_->setItem(
            row, 4,
            new QTableWidgetItem(
                discoveryMethods(host)
            )
        );

        results_->setItem(
            row, 5,
            new QTableWidgetItem(
                host.responding
                    ? "Yes"
                    : "ARP cache only"
            )
        );
    }

    results_->setSortingEnabled(true);

    std::size_t checked =
        worker->addressesChecked();

    std::size_t total =
        worker->totalAddresses();

    countLabel_->setText(
        QString("Checked: %1/%2 | Discovered: %3")
            .arg(static_cast<int>(checked))
            .arg(static_cast<int>(total))
            .arg(static_cast<int>(hosts.size()))
    );

    if (worker->wasStopped()) {
        statusLabel_->setText("Discovery stopped.");
    } else {
        progressBar_->setValue(100);
        statusLabel_->setText("Discovery complete.");
    }

    setScanning(false);
}
