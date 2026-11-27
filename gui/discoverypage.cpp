
#include "discoverypage.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMetaType>
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
#include <QByteArray>
#include <QTextStream>
#include <string>
#include <utility>

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

    qRegisterMetaType<DiscoveredHost>("DiscoveredHost");
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

    // Discovery method selection
    auto* methodLayout = new QHBoxLayout();

    auto* methodLabel = new QLabel("Discovery methods:");

    icmpCheck_ = new QCheckBox("ICMP");
    arpCheck_ = new QCheckBox("ARP");
    tcpCheck_ = new QCheckBox("TCP");

    icmpCheck_->setChecked(settings_.enableICMP);
    arpCheck_->setChecked(settings_.enableARP);
    tcpCheck_->setChecked(settings_.enableTCP);

    methodLayout->addWidget(methodLabel);
    methodLayout->addWidget(icmpCheck_);
    methodLayout->addWidget(arpCheck_);
    methodLayout->addWidget(tcpCheck_);
    methodLayout->addStretch();

    layout->addLayout(methodLayout);

    // Action buttons
    auto* buttons = new QHBoxLayout();

    startButton_ =
        new QPushButton("Start Discovery");

    stopButton_ =
        new QPushButton("Stop");

    stopButton_->setEnabled(false);

    exportCsvButton_ = new QPushButton("Export CSV");
    exportJsonButton_ = new QPushButton("Export JSON");
    showMapButton_ = new QPushButton("Show in Network Map");
    
    showMapButton_->setEnabled(false);
    exportCsvButton_->setEnabled(false);
    exportJsonButton_->setEnabled(false);

    buttons->addWidget(startButton_);
    buttons->addWidget(stopButton_);
    buttons->addStretch();
    buttons->addWidget(exportCsvButton_);
    buttons->addWidget(exportJsonButton_);
    buttons->addWidget(showMapButton_);

    layout->addLayout(buttons);

    connect(
        showMapButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (hosts_.empty() || !showMapCallback_) {
                return;
            }

            const std::string ip =
                ipInput_->text().trimmed().toStdString();

            const int prefix = prefixInput_->value();

            const SubnetInfo subnet =
                calculateSubnet(ip, prefix);

            const QString cidr =
                QString::fromStdString(
                    subnet.networkAddress
                )
                + "/" + QString::number(prefix);

            showMapCallback_(hosts_, cidr);
        }
    );

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

    // Export completed discovery results
    connect(
        exportCsvButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            exportResults(false);
        }
    );

    connect(
        exportJsonButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            exportResults(true);
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

    icmpCheck_->setEnabled(!scanning);
    arpCheck_->setEnabled(!scanning);
    tcpCheck_->setEnabled(!scanning);

    exportCsvButton_->setEnabled(
        !scanning && !hosts_.empty()
    );

    exportJsonButton_->setEnabled(
        !scanning && !hosts_.empty()
    );
    showMapButton_->setEnabled(
        !scanning && !hosts_.empty()
    );
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

    if (!icmpCheck_->isChecked() &&
        !arpCheck_->isChecked() &&
        !tcpCheck_->isChecked()) {

        QMessageBox::warning(
            this,
            "No Discovery Methods",
            "Enable at least one discovery method."
        );

        return;
    }

    // Copy settings for this individual scan
    AppSettings scanSettings = settings_;

    scanSettings.enableICMP = icmpCheck_->isChecked();
    scanSettings.enableARP = arpCheck_->isChecked();
    scanSettings.enableTCP = tcpCheck_->isChecked();

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
    hosts_.clear();

    setScanning(true);
    progressBar_->setValue(0);
    results_->setRowCount(0);

    countLabel_->setText(
        QString("Addresses to check: %1").arg(total)
    );

    statusLabel_->setText("Starting discovery...");

    auto* worker = new DiscoveryWorker(
        ip, prefix, scanSettings, this
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

    // Display discovered devices as they are found
    connect(
        worker,
        &DiscoveryWorker::hostFound,
        this,
        [this, worker](DiscoveredHost host) {
        if (worker_ != worker) {
            return;
        }

        hosts_.push_back(host);
        appendHost(host);

        countLabel_->setText(
            QString("Found: %1 device(s)")
                .arg(static_cast<int>(hosts_.size()))
        );
        }
    );

    // Update hostnames as reverse DNS completes
    connect(
        worker,
        &DiscoveryWorker::hostnameResolved,
        this,
        [this](const QString& ip, const QString& hostname) {
            updateHostname(ip, hostname);
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

            if (pendingDefaultsRefresh_) {
                refreshDiscoveryDefaults();
            }
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
    // Use the authoritative final results
    hosts_ = worker->results();

    rebuildResultsTable();

    std::size_t checked =
        worker->addressesChecked();

    std::size_t total =
        worker->totalAddresses();

    countLabel_->setText(
        QString("Checked: %1/%2 | Discovered: %3")
            .arg(static_cast<int>(checked))
            .arg(static_cast<int>(total))
            .arg(static_cast<int>(hosts_.size()))
    );

    if (worker->wasStopped()) {
        statusLabel_->setText("Discovery stopped.");
    } else {
        progressBar_->setValue(100);
        statusLabel_->setText("Discovery complete.");
    }

    setScanning(false);
}

void DiscoveryPage::appendHost(
    const DiscoveredHost& host
) {
    // Temporarily disable sorting during insertion
    bool sorting = results_->isSortingEnabled();
    results_->setSortingEnabled(false);

    int row = results_->rowCount();
    results_->insertRow(row);

    results_->setItem(
        row, 0,
        new QTableWidgetItem(displayText(host.ipAddress))
    );

    results_->setItem(
        row, 1,
        new QTableWidgetItem(displayText(host.hostname))
    );

    results_->setItem(
        row, 2,
        new QTableWidgetItem(displayText(host.macAddress))
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
        new QTableWidgetItem(discoveryMethods(host))
    );

    results_->setItem(
        row, 5,
        new QTableWidgetItem(
            host.responding ? "Yes" : "ARP cache only"
        )
    );

    results_->setSortingEnabled(sorting);
}


void DiscoveryPage::updateHostname(
    const QString& ip,
    const QString& hostname
) {
    // Update the stored result used for export
    for (auto& host : hosts_) {
        if (QString::fromStdString(host.ipAddress) == ip) {
            host.hostname = hostname.toStdString();
            break;
        }
    }

    // Find the matching row even if the table is sorted
    for (int row = 0; row < results_->rowCount(); ++row) {
        auto* ipItem = results_->item(row, 0);

        if (!ipItem || ipItem->text() != ip) {
            continue;
        }

        auto* hostnameItem = results_->item(row, 1);

        if (hostnameItem) {
            hostnameItem->setText(
                hostname.isEmpty() ? "-" : hostname
            );
        }

        break;
    }
}

void DiscoveryPage::rebuildResultsTable() {
    results_->setSortingEnabled(false);
    results_->setRowCount(0);

    for (const DiscoveredHost& host : hosts_) {
        appendHost(host);
    }

    results_->setSortingEnabled(true);
}

namespace {

// Escape a field for CSV, including spreadsheet formula safety
QString csvField(QString value) {
    QString trimmed = value.trimmed();

    if (!trimmed.isEmpty()) {
        const QChar first = trimmed.at(0);

        // Prevent network-derived fields from being
        // interpreted as spreadsheet formulas.
        if (first == '=' || first == '+' ||
            first == '-' || first == '@') {
            value.prepend('\'');
        }
    }

    value.replace("\"", "\"\"");

    return "\"" + value + "\"";
}

} // namespace


void DiscoveryPage::exportResults(bool json) {
    if (hosts_.empty()) {
        QMessageBox::information(
            this,
            "Nothing to Export",
            "There are no discovered devices to export."
        );
        return;
    }

    const QString extension = json ? ".json" : ".csv";

    QString path = QFileDialog::getSaveFileName(
        this,
        json ? "Export Discovery as JSON"
             : "Export Discovery as CSV",
        "netscope-discovery" + extension,
        json ? "JSON Files (*.json)"
             : "CSV Files (*.csv)"
    );

    if (path.isEmpty()) {
        return;
    }

    if (!path.endsWith(extension, Qt::CaseInsensitive)) {
        path += extension;
    }

    QByteArray output;

    if (json) {
        QJsonArray devices;

        for (const auto& host : hosts_) {
            QJsonObject device;

            device["ipAddress"] =
                QString::fromStdString(host.ipAddress);

            device["hostname"] =
                QString::fromStdString(host.hostname);

            device["macAddress"] =
                QString::fromStdString(host.macAddress);

            device["vendor"] =
                QString::fromStdString(
                    lookupMACVendor(host.macAddress)
                );

            device["responding"] = host.responding;
            device["icmpResponded"] = host.icmpResponded;
            device["arpFound"] = host.arpFound;
            device["arpConfirmed"] = host.arpConfirmed;
            device["tcpResponded"] = host.tcpResponded;

            devices.append(device);
        }

        QJsonObject document;
        document["application"] = "NetScope";
        document["formatVersion"] = 1;
        document["deviceCount"] =
            static_cast<int>(hosts_.size());
        document["devices"] = devices;

        output = QJsonDocument(document).toJson(
            QJsonDocument::Indented
        );
    }
    else {
        QString csv;

        csv += "IP Address,Hostname,MAC Address,Vendor,"
               "Methods,Responding,ICMP,ARP Found,"
               "ARP Confirmed,TCP\n";

        for (const auto& host : hosts_) {
            QStringList fields = {
                QString::fromStdString(host.ipAddress),
                QString::fromStdString(host.hostname),
                QString::fromStdString(host.macAddress),
                QString::fromStdString(
                    lookupMACVendor(host.macAddress)
                ),
                discoveryMethods(host),
                host.responding ? "Yes" : "No",
                host.icmpResponded ? "Yes" : "No",
                host.arpFound ? "Yes" : "No",
                host.arpConfirmed ? "Yes" : "No",
                host.tcpResponded ? "Yes" : "No"
            };

            for (QString& field : fields) {
                field = csvField(field);
            }

            csv += fields.join(',') + "\n";
        }

        output = csv.toUtf8();
    }

    // QSaveFile writes to a temporary file first and
    // replaces the destination only on successful commit.
    QSaveFile file(path);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            this,
            "Export Failed",
            file.errorString()
        );
        return;
    }

    if (file.write(output) != output.size()) {
        file.cancelWriting();

        QMessageBox::critical(
            this,
            "Export Failed",
            "Unable to write the complete export file."
        );
        return;
    }

    if (!file.commit()) {
        QMessageBox::critical(
            this,
            "Export Failed",
            file.errorString()
        );
        return;
    }

    QMessageBox::information(
        this,
        "Export Complete",
        QString("Exported %1 devices to:\n%2")
            .arg(static_cast<int>(hosts_.size()))
            .arg(path)
    );
}

void DiscoveryPage::refreshDiscoveryDefaults() {
    if (worker_) {
        pendingDefaultsRefresh_ = true;
        return;
    }

    pendingDefaultsRefresh_ = false;

    icmpCheck_->setChecked(settings_.enableICMP);
    arpCheck_->setChecked(settings_.enableARP);
    tcpCheck_->setChecked(settings_.enableTCP);
}

void DiscoveryPage::setShowMapCallback(
    std::function<void(
        const std::vector<DiscoveredHost>&,
        const QString&
    )> callback
) {
    showMapCallback_ = std::move(callback);
}