
#include "mainwindow.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QFrame>
#include <QSettings>
#include <QtGlobal>

#include "portscannerpage.h"
#include "overviewpage.h"
#include "discoverypage.h"
#include "subnetcalculatorpage.h"
#include "settingspage.h"
#include "networkmappage.h"

namespace {

AppSettings loadSavedSettings() {
    AppSettings settings{};

    QSettings storage("NetScope", "NetScope");

    settings.maxThreads = static_cast<unsigned int>(
        qBound(
            1,
            storage.value(
                "scanner/maxThreads",
                static_cast<int>(settings.maxThreads)
            ).toInt(),
            256
        )
    );

    settings.tcpTimeoutMs = static_cast<unsigned int>(
        qBound(
            100,
            storage.value(
                "scanner/tcpTimeoutMs",
                static_cast<int>(settings.tcpTimeoutMs)
            ).toInt(),
            10000
        )
    );

    settings.icmpTimeoutMs = static_cast<unsigned int>(
        qBound(
            100,
            storage.value(
                "scanner/icmpTimeoutMs",
                static_cast<int>(settings.icmpTimeoutMs)
            ).toInt(),
            10000
        )
    );

    settings.enableICMP = storage.value(
        "discovery/enableICMP",
        settings.enableICMP
    ).toBool();

    settings.enableARP = storage.value(
        "discovery/enableARP",
        settings.enableARP
    ).toBool();

    settings.enableTCP = storage.value(
        "discovery/enableTCP",
        settings.enableTCP
    ).toBool();

    return settings;
}

} // namespace

// Temporary page for features not yet implemented
static QWidget* createPlaceholderPage(
    const QString& heading,
    const QString& description
) {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* title = new QLabel(heading);

    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    auto* message = new QLabel(description);
    message->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(message);
    layout->addStretch();

    return page;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      settings_(loadSavedSettings()) {

    setWindowTitle("NetScope");
    resize(1100, 720);

    setupInterface();
}

void MainWindow::setupInterface() {
    // Main application container
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    auto* sidebar = new QFrame();
    sidebar->setFixedWidth(210);
    sidebar->setObjectName("sidebar");

    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(12, 20, 12, 12);
    sidebarLayout->setSpacing(8);

    auto* logo = new QLabel("NetScope");

    QFont logoFont = logo->font();
    logoFont.setPointSize(18);
    logoFont.setBold(true);
    logo->setFont(logoFont);

    sidebarLayout->addWidget(logo);
    sidebarLayout->addSpacing(20);

    // Page container
    pages_ = new QStackedWidget();

    pages_->addWidget(new OverviewPage());

    pages_->addWidget(new PortScannerPage(settings_));

    auto* discoveryPage = new DiscoveryPage(settings_);
    pages_->addWidget(discoveryPage);

    pages_->addWidget(new SubnetCalculatorPage());

    auto* mapPage = new NetworkMapPage();
    pages_->addWidget(mapPage);

    auto* settingsPage = new SettingsPage(
        settings_,
        [discoveryPage]() {
            discoveryPage->refreshDiscoveryDefaults();
        }
    );

    pages_->addWidget(settingsPage);

    // Navigation buttons
    auto* navigationGroup = new QButtonGroup(this);
    navigationGroup->setExclusive(true);

    const QStringList pageNames = {
        "Overview",
        "Port Scanner",
        "Network Discovery",
        "Subnet Calculator",
        "Network Map",
        "Settings"
    };

    for (int i = 0; i < pageNames.size(); ++i) {
        auto* button = new QPushButton(pageNames[i]);

        button->setCheckable(true);
        button->setMinimumHeight(38);

        navigationGroup->addButton(button, i);
        sidebarLayout->addWidget(button);

        connect(
            button,
            &QPushButton::clicked,
            this,
            [this, i]() {
                pages_->setCurrentIndex(i);
            }
        );
    }

    discoveryPage->setShowMapCallback(
        [this, mapPage, navigationGroup](
            const std::vector<DiscoveredHost>& hosts,
            const QString& cidr
        ) {
            // Replace the current map with discovery results.
            mapPage->showDiscoveredHosts(hosts, cidr);

            // Navigate to Network Map.
            pages_->setCurrentWidget(mapPage);

            // Update the sidebar selection.
            if (auto* button = navigationGroup->button(4)) {
                button->setChecked(true);
            }
        }
    );

    sidebarLayout->addStretch();

    // Select Port Scanner initially
    navigationGroup->button(1)->setChecked(true);
    pages_->setCurrentIndex(1);

    // Main content area
    auto* content = new QWidget();
    auto* contentLayout = new QVBoxLayout(content);

    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->addWidget(pages_);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(content, 1);

    setCentralWidget(centralWidget);

    // Styling, respecting both light and dark Qt palettes
    sidebar->setStyleSheet(
        "QFrame#sidebar {"
        "  background-color: palette(alternate-base);"
        "  border-right: 1px solid palette(mid);"
        "}"
    );

    for (auto* button : navigationGroup->buttons()) {
        button->setStyleSheet(
            "QPushButton {"
            "  text-align: left;"
            "  padding: 8px 12px;"
            "  border: none;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:checked {"
            "  background-color: palette(highlight);"
            "  color: palette(highlighted-text);"
            "}"
        );
    }
}
