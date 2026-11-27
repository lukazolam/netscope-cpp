
#include <QApplication>
#include <QCoreApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QProgressBar>
#include <QFont>
#include <QPointer>

#include <string>
#include <vector>

#include "network.h"
#include "settings.h"
#include "scanworker.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Initialize platform networking
    if (!initializeNetworking()) {
        QMessageBox::critical(
            nullptr,
            "NetScope",
            "Unable to initialize networking."
        );
        return 1;
    }

    AppSettings settings;

    QMainWindow window;
    window.setWindowTitle("NetScope - TCP Port Scanner");
    window.resize(950, 650);

    QWidget* centralWidget = new QWidget(&window);
    QVBoxLayout* mainLayout =
        new QVBoxLayout(centralWidget);

    // Page title
    QLabel* title = new QLabel("TCP Port Scanner");

    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    mainLayout->addWidget(title);

    // Target IP address
    mainLayout->addWidget(
        new QLabel("Target IPv4 Address:")
    );

    QLineEdit* ipInput = new QLineEdit();
    ipInput->setPlaceholderText(
        "Enter IPv4 address (e.g. 127.0.0.1)"
    );

    mainLayout->addWidget(ipInput);

    // Scan mode
    mainLayout->addWidget(
        new QLabel("Scan Mode:")
    );

    QComboBox* scanMode = new QComboBox();
    scanMode->addItem("Common Ports");
    scanMode->addItem("Single Port");
    scanMode->addItem("Port Range");
    scanMode->addItem("Full Scan (1-65535)");

    mainLayout->addWidget(scanMode);

    // Port selection controls
    QHBoxLayout* portLayout = new QHBoxLayout();

    QLabel* startLabel = new QLabel("Starting Port:");

    QSpinBox* startPort = new QSpinBox();
    startPort->setRange(1, 65535);
    startPort->setValue(1);

    QLabel* endLabel = new QLabel("Ending Port:");

    QSpinBox* endPort = new QSpinBox();
    endPort->setRange(1, 65535);
    endPort->setValue(1024);

    portLayout->addWidget(startLabel);
    portLayout->addWidget(startPort);
    portLayout->addWidget(endLabel);
    portLayout->addWidget(endPort);
    portLayout->addStretch();

    mainLayout->addLayout(portLayout);

    // Update visible fields for the selected mode
    auto updatePortFields = [=](int mode) {
        bool single = mode == 1;
        bool range = mode == 2;

        startLabel->setVisible(single || range);
        startPort->setVisible(single || range);

        endLabel->setVisible(range);
        endPort->setVisible(range);

        startLabel->setText(
            single ? "Port:" : "Starting Port:"
        );
    };

    QObject::connect(
        scanMode,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        &window,
        updatePortFields
    );

    // Initial mode is Common Ports
    updatePortFields(scanMode->currentIndex());

    // Start and Stop buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* scanButton =
        new QPushButton("Start Scan");

    QPushButton* stopButton =
        new QPushButton("Stop Scan");

    stopButton->setEnabled(false);

    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(stopButton);

    mainLayout->addLayout(buttonLayout);

    // Progress bar
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    mainLayout->addWidget(progressBar);

    // Scan status
    QLabel* statusLabel = new QLabel("Ready to scan.");
    mainLayout->addWidget(statusLabel);

    // Scan results
    QTableWidget* results = new QTableWidget();

    results->setColumnCount(3);
    results->setHorizontalHeaderLabels(
        {"Port", "Protocol", "Status"}
    );

    results->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    results->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    results->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    results->setSortingEnabled(true);

    mainLayout->addWidget(results);

    window.setCentralWidget(centralWidget);

    // Track the active worker
    QPointer<ScanWorker> currentWorker;

    // Safely stop scanning when the application exits
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &window,
        [&currentWorker]() {
            if (currentWorker) {
                currentWorker->requestStop();
                currentWorker->wait();
            }
        }
    );

    // Stop Scan button
    QObject::connect(
        stopButton,
        &QPushButton::clicked,
        &window,
        [&]() {
            if (currentWorker) {
                currentWorker->requestStop();

                stopButton->setEnabled(false);
                statusLabel->setText(
                    "Stopping scan. Finishing active connections..."
                );
            }
        }
    );

    // Start Scan button
    QObject::connect(
        scanButton,
        &QPushButton::clicked,
        &window,
        [&]() {
            std::string ip =
                ipInput->text().trimmed().toStdString();

            if (!validateIPv4(ip)) {
                QMessageBox::warning(
                    &window,
                    "Invalid IP Address",
                    "Please enter a valid IPv4 address."
                );
                return;
            }

            // Build the port list for the selected mode
            std::vector<int> ports;

            int mode = scanMode->currentIndex();

            if (mode == 0) {
                // Same common ports as our CLI
                ports = {
                    21, 22, 23, 25, 53,
                    80, 110, 143, 443, 445,
                    993, 995, 1433, 3306, 3389,
                    5432, 5900, 6379, 8080, 8443
                };
            }
            else if (mode == 1) {
                ports.push_back(startPort->value());
            }
            else {
                int first = mode == 3
                    ? 1 : startPort->value();

                int last = mode == 3
                    ? 65535 : endPort->value();

                if (first > last) {
                    QMessageBox::warning(
                        &window,
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

            // Reset the interface
            scanButton->setEnabled(false);
            stopButton->setEnabled(true);

            ipInput->setEnabled(false);
            scanMode->setEnabled(false);
            startPort->setEnabled(false);
            endPort->setEnabled(false);

            progressBar->setValue(0);
            results->setRowCount(0);

            statusLabel->setText(
                QString("Scanning %1 TCP ports on %2...")
                    .arg(static_cast<int>(ports.size()))
                    .arg(QString::fromStdString(ip))
            );

            // Create the background worker
            ScanWorker* worker = new ScanWorker(
                ip,
                ports,
                settings,
                &window
            );

            currentWorker = worker;

            // Update progress on the GUI thread
            QObject::connect(
                worker,
                &ScanWorker::progressUpdated,
                &window,
                [progressBar](int percentage) {
                    if (percentage > progressBar->value()) {
                        progressBar->setValue(percentage);
                    }
                }
            );

            // Process completed or canceled scans
            QObject::connect(
                worker,
                &QThread::finished,
                &window,
                [=]() {
                    int openCount = 0;
                    int closedCount = 0;
                    int timeoutCount = 0;
                    int errorCount = 0;

                    // Avoid reordering rows during insertion
                    results->setSortingEnabled(false);

                    for (const PortScanResult& result :
                         worker->results()) {

                        switch (result.status) {
                            case PortStatus::Open: {
                                ++openCount;

                                int row = results->rowCount();
                                results->insertRow(row);

                                // Store port as a number for proper sorting
                                auto* portItem =
                                    new QTableWidgetItem();

                                portItem->setData(
                                    Qt::EditRole,
                                    result.port
                                );

                                results->setItem(
                                    row, 0, portItem
                                );

                                results->setItem(
                                    row, 1,
                                    new QTableWidgetItem("TCP")
                                );

                                results->setItem(
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

                    results->setSortingEnabled(true);

                    int scanned =
                        static_cast<int>(worker->results().size());

                    if (worker->wasStopped()) {
                        statusLabel->setText(
                            QString(
                                "Scan stopped. Checked: %1/%2 | "
                                "Open: %3 | Closed: %4 | "
                                "Timeouts: %5 | Errors: %6"
                            )
                            .arg(scanned)
                            .arg(static_cast<int>(ports.size()))
                            .arg(openCount)
                            .arg(closedCount)
                            .arg(timeoutCount)
                            .arg(errorCount)
                        );
                    }
                    else {
                        progressBar->setValue(100);

                        statusLabel->setText(
                            QString(
                                "Scan complete. Checked: %1 | "
                                "Open: %2 | Closed: %3 | "
                                "Timeouts: %4 | Errors: %5"
                            )
                            .arg(scanned)
                            .arg(openCount)
                            .arg(closedCount)
                            .arg(timeoutCount)
                            .arg(errorCount)
                        );
                    }

                    // Re-enable controls
                    scanButton->setEnabled(true);
                    stopButton->setEnabled(false);

                    ipInput->setEnabled(true);
                    scanMode->setEnabled(true);
                    startPort->setEnabled(true);
                    endPort->setEnabled(true);
                }
            );

            // Cleanup after result processing
            QObject::connect(
                worker,
                &QThread::finished,
                worker,
                &QObject::deleteLater
            );

            worker->start();
        }
    );

    window.show();

    int exitCode = app.exec();

    cleanupNetworking();

    return exitCode;
}
