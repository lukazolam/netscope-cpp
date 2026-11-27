
#include <QApplication>
#include <QCoreApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

    // Initialize platform networking (Winsock on Windows)
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
    window.resize(900, 600);

    QWidget* centralWidget = new QWidget(&window);
    QVBoxLayout* mainLayout =
        new QVBoxLayout(centralWidget);

    // Title
    QLabel* title = new QLabel("TCP Port Scanner");

    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    mainLayout->addWidget(title);

    // Target IP and scan button
    QHBoxLayout* inputLayout = new QHBoxLayout();

    QLineEdit* ipInput = new QLineEdit();
    ipInput->setPlaceholderText(
        "Target IPv4 address (e.g. 127.0.0.1)"
    );

    QPushButton* scanButton =
        new QPushButton("Start Scan");

    inputLayout->addWidget(ipInput);
    inputLayout->addWidget(scanButton);
    mainLayout->addLayout(inputLayout);

    // Progress indicator
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    mainLayout->addWidget(progressBar);

    // Status label
    QLabel* statusLabel = new QLabel("Ready to scan.");
    mainLayout->addWidget(statusLabel);

    // Results table
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

    mainLayout->addWidget(results);

    window.setCentralWidget(centralWidget);

    // Keeps track of the running worker safely
    QPointer<ScanWorker> currentWorker;

    // Don't destroy an active thread when the app closes.
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &window,
        [&currentWorker]() {
            if (currentWorker) {
                currentWorker->wait();
            }
        }
    );

    // Handle Start Scan
    QObject::connect(
        scanButton,
        &QPushButton::clicked,
        &window,
        [&]() {
            std::string ip = ipInput->text().toStdString();

            if (!validateIPv4(ip)) {
                QMessageBox::warning(
                    &window,
                    "Invalid IP Address",
                    "Please enter a valid IPv4 address."
                );
                return;
            }

            // Our existing CLI's common TCP ports
            std::vector<int> commonPorts = {
                21, 22, 23, 25, 53,
                80, 110, 143, 443, 445,
                993, 995, 1433, 3306, 3389,
                5432, 5900, 6379, 8080, 8443
            };

            // Prepare the interface
            scanButton->setEnabled(false);
            ipInput->setEnabled(false);
            progressBar->setValue(0);
            results->setRowCount(0);

            statusLabel->setText(
                "Scanning " + QString::fromStdString(ip) + "..."
            );

            // Create the background worker
            ScanWorker* worker = new ScanWorker(
                ip,
                commonPorts,
                settings,
                &window
            );

            currentWorker = worker;

            // Progress signals are delivered to the GUI thread
            QObject::connect(
                worker,
                &ScanWorker::progressUpdated,
                &window,
                [progressBar](int percentage) {
                    progressBar->setValue(percentage);
                }
            );

            // Process the results after the worker finishes
            QObject::connect(
                worker,
                &QThread::finished,
                &window,
                [=]() {
                    int openCount = 0;
                    int closedCount = 0;
                    int timeoutCount = 0;
                    int errorCount = 0;

                    for (const PortScanResult& result :
                         worker->results()) {

                        switch (result.status) {
                            case PortStatus::Open: {
                                ++openCount;

                                int row = results->rowCount();
                                results->insertRow(row);

                                results->setItem(
                                    row, 0,
                                    new QTableWidgetItem(
                                        QString::number(result.port)
                                    )
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

                    progressBar->setValue(100);

                    statusLabel->setText(
                        QString(
                            "Scan complete. Open: %1 | "
                            "Closed: %2 | Timed out: %3 | "
                            "Errors: %4"
                        )
                        .arg(openCount)
                        .arg(closedCount)
                        .arg(timeoutCount)
                        .arg(errorCount)
                    );

                    scanButton->setEnabled(true);
                    ipInput->setEnabled(true);
                }
            );

            // Schedule worker cleanup after processing results
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
