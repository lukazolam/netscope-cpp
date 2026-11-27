
#include <QApplication>
#include <QMessageBox>

#include "mainwindow.h"
#include "network.h"

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

    int exitCode = 0;

    {
        MainWindow window;
        window.show();

        exitCode = app.exec();
    }

    // MainWindow and its workers are now destroyed.
    // It is safe to clean up platform networking.
    cleanupNetworking();

    return exitCode;
}
