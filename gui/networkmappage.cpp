
#include "networkmappage.h"

#include "mapview.h"
#include "mapnode.h"
#include "mapedge.h"
#include "macvendor.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QShowEvent>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QStringList>
#include <QByteArray>

#include <utility>

NetworkMapPage::NetworkMapPage(QWidget* parent)
    : QWidget(parent) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* title = new QLabel("Network Map");

    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    layout->addWidget(title);

    // Toolbar
    auto* toolbar = new QHBoxLayout();

    resetButton_ = new QPushButton("Load Demo");
    fitButton_ = new QPushButton("Fit to View");

    auto* importButton = new QPushButton("Load Discovery JSON");

    auto* zoomInButton = new QPushButton("Zoom +");
    auto* zoomOutButton = new QPushButton("Zoom -");

    toolbar->addWidget(resetButton_);
    toolbar->addWidget(importButton);
    toolbar->addWidget(fitButton_);
    toolbar->addStretch();
    toolbar->addWidget(zoomOutButton);
    toolbar->addWidget(zoomInButton);

    layout->addLayout(toolbar);

    // The scene owns all graphics items.
    scene_ = new QGraphicsScene(this);

    view_ = new MapView();
    view_->setScene(scene_);

    layout->addWidget(view_, 1);

    // Selection details
    detailsLabel_ = new QLabel(
        "Select a node to view its details."
    );

    detailsLabel_->setMinimumHeight(60);
    detailsLabel_->setWordWrap(true);
    detailsLabel_->setTextInteractionFlags(
        Qt::TextSelectableByMouse
    );
    layout->addWidget(detailsLabel_);

    statusLabel_ = new QLabel(
        "Demo topology. Connections show logical grouping."
    );

    layout->addWidget(statusLabel_);

    connect(
        fitButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            view_->fitMap();
        }
    );

    connect(
        resetButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            loadDemoTopology();
            view_->fitMap();
        }
    );

    connect(
        zoomInButton,
        &QPushButton::clicked,
        this,
        [this]() {
            view_->zoomIn();
        }
    );

    connect(
        zoomOutButton,
        &QPushButton::clicked,
        this,
        [this]() {
            view_->zoomOut();
        }
    );

    connect(
        scene_,
        &QGraphicsScene::selectionChanged,
        this,
        [this]() {
            updateSelectionDetails();
        }
    );

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        [this]() {
            loadDiscoveryJson();
        }
    );

    // Initial sample data
    loadDemoTopology();
}

MapNode* NetworkMapPage::addNode(
    const QString& name,
    const QString& address,
    MapNodeType type,
    qreal x,
    qreal y
) {
    auto* node = new MapNode(name, address, type);

    scene_->addItem(node);
    node->setPos(x, y);

    return node;
}

void NetworkMapPage::connectNodes(
    MapNode* source,
    MapNode* target
) {
    auto* edge = new MapEdge(source, target);

    scene_->addItem(edge);
    edge->updatePosition();
}

void NetworkMapPage::loadDemoTopology() {
    scene_->clear();
    discoveredHosts_.clear();
    
    // Top-level gateway
    MapNode* gateway = addNode(
        "Gateway",
        "10.10.0.1",
        MapNodeType::Gateway,
        440, 0
    );

    // Two logical networks
    MapNode* subnetA = addNode(
        "Lab Network A",
        "10.10.10.0/24",
        MapNodeType::Subnet,
        190, 145
    );

    MapNode* subnetB = addNode(
        "Lab Network B",
        "10.10.20.0/24",
        MapNodeType::Subnet,
        690, 145
    );

    connectNodes(gateway, subnetA);
    connectNodes(gateway, subnetB);

    // Devices in subnet A
    MapNode* vm1 = addNode(
        "Windows VM",
        "10.10.10.10",
        MapNodeType::Device,
        20, 290
    );

    MapNode* vm2 = addNode(
        "Linux VM",
        "10.10.10.20",
        MapNodeType::Device,
        210, 290
    );

    MapNode* vm3 = addNode(
        "File Server",
        "10.10.10.30",
        MapNodeType::Device,
        400, 290
    );

    connectNodes(subnetA, vm1);
    connectNodes(subnetA, vm2);
    connectNodes(subnetA, vm3);

    // Devices in subnet B
    MapNode* vm4 = addNode(
        "Web Server",
        "10.10.20.10",
        MapNodeType::Device,
        540, 430
    );

    MapNode* vm5 = addNode(
        "Database",
        "10.10.20.20",
        MapNodeType::Device,
        730, 430
    );

    MapNode* vm6 = addNode(
        "Monitoring",
        "10.10.20.30",
        MapNodeType::Device,
        920, 430
    );

    connectNodes(subnetB, vm4);
    connectNodes(subnetB, vm5);
    connectNodes(subnetB, vm6);

    // Leave room around the entire topology.
    scene_->setSceneRect(
        scene_->itemsBoundingRect().adjusted(
            -80, -80, 80, 80
        )
    );

    detailsLabel_->setText(
        "Select a node to view its details."
    );

    statusLabel_->setText(
        "Demo topology: 6 devices, 2 subnets, 1 gateway."
    );
}

void NetworkMapPage::updateSelectionDetails() {
    const auto selected = scene_->selectedItems();

    if (selected.isEmpty()) {
        detailsLabel_->setText(
            "Select a node to view its details."
        );
        return;
    }

    // Child text items are not selectable; the
    // selected item should be one of our MapNodes.
    auto* node = dynamic_cast<MapNode*>(
        selected.first()
    );

    if (!node) {
        detailsLabel_->setText(
            "No device selected."
        );
        return;
    }

    QString typeText;

    switch (node->nodeType()) {
        case MapNodeType::Device:
            typeText = "Device";
            break;

        case MapNodeType::Subnet:
            typeText = "Subnet";
            break;

        case MapNodeType::Gateway:
            typeText = "Gateway";
            break;
    }

    
    // Look for actual discovery data associated with this IP.
    const auto it = discoveredHosts_.constFind(node->address());

    if (it != discoveredHosts_.cend()) {
        const DiscoveredHost& host = it.value();

        QStringList methods;

        if (host.icmpResponded) {
            methods << "ICMP";
        }

        if (host.arpFound) {
            methods << (
                host.arpConfirmed ? "ARP confirmed" : "ARP cached"
            );
        }

        if (host.tcpResponded) {
            methods << "TCP";
        }

        const QString hostname =
            QString::fromStdString(host.hostname);

        const QString mac =
            QString::fromStdString(host.macAddress);

        const QString vendor =
            QString::fromStdString(
                lookupMACVendor(host.macAddress)
            );

        detailsLabel_->setText(
            QString(
                "IP: %1 | Hostname: %2 | MAC: %3\n"
                "Vendor: %4 | Methods: %5 | Responding: %6"
            )
                .arg(node->address())
                .arg(hostname.isEmpty() ? "-" : hostname)
                .arg(mac.isEmpty() ? "-" : mac)
                .arg(vendor)
                .arg(methods.isEmpty() ? "-" : methods.join(", "))
                .arg(host.responding ? "Yes" : "No")
        );
    }
    else {
        detailsLabel_->setText(
            QString("%1 | %2 | %3")
                .arg(node->name())
                .arg(node->address())
                .arg(typeText)
        );
    }

}

void NetworkMapPage::showEvent(
    QShowEvent* event
) {
    QWidget::showEvent(event);

    // Fit after the page becomes visible and has
    // a meaningful viewport size.
    QTimer::singleShot(0, this, [this]() {
        view_->fitMap();
    });
}

void NetworkMapPage::showDiscoveredHosts(
    const std::vector<DiscoveredHost>& hosts,
    const QString& subnetCidr
) {
    // Replace the previous map completely.
    scene_->clear();
    discoveredHosts_.clear();

    // Create a logical subnet node.
    MapNode* subnet = addNode(
        "Discovered Network",
        subnetCidr,
        MapNodeType::Subnet,
        400,
        0
    );

    // Arrange devices in a grid.
    constexpr int columns = 4;
    constexpr qreal horizontalSpacing = 210.0;
    constexpr qreal verticalSpacing = 140.0;

    constexpr qreal startX = 0.0;
    constexpr qreal startY = 180.0;

    int index = 0;

    for (const DiscoveredHost& host : hosts) {
        const QString ip =
            QString::fromStdString(host.ipAddress);

        if (ip.isEmpty() || discoveredHosts_.contains(ip)) {
            continue;
        }

        discoveredHosts_.insert(ip, host);

        QString name =
            QString::fromStdString(host.hostname);

        if (name.isEmpty()) {
            name = "Unknown Device";
        }

        const int row = index / columns;
        const int column = index % columns;

        const qreal x =
            startX + column * horizontalSpacing;

        const qreal y =
            startY + row * verticalSpacing;

        MapNode* device = addNode(
            name,
            ip,
            MapNodeType::Device,
            x,
            y
        );

        connectNodes(subnet, device);

        ++index;
    }

    // Center subnet node above the device columns.
    subnet->setPos(
        ((columns - 1) * horizontalSpacing) / 2.0,
        0
    );

    // Calculate map bounds with a margin.
    scene_->setSceneRect(
        scene_->itemsBoundingRect().adjusted(
            -100, -100, 100, 100
        )
    );

    detailsLabel_->setText(
        "Select a device to view its discovery details."
    );

    statusLabel_->setText(
        QString("Discovered network: %1 | Devices: %2")
            .arg(subnetCidr)
            .arg(index)
    );

    view_->fitMap();
}

void NetworkMapPage::loadDiscoveryJson() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Load NetScope Discovery Results",
        QString(),
        "JSON Files (*.json)"
    );

    if (path.isEmpty()) {
        return;
    }

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(
            this,
            "Import Failed",
            file.errorString()
        );
        return;
    }

    // Keep imported files bounded for the GUI.
    constexpr qint64 maxFileSize = 10 * 1024 * 1024;

    if (file.size() > maxFileSize) {
        QMessageBox::warning(
            this,
            "Import Failed",
            "The JSON file is too large."
        );
        return;
    }

    const QByteArray data = file.readAll();

    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError ||
        !document.isObject()) {

        QMessageBox::warning(
            this,
            "Invalid JSON",
            "The file does not contain a valid JSON object."
        );
        return;
    }

    const QJsonObject root = document.object();

    if (root.value("application").toString() != "NetScope" ||
        root.value("formatVersion").toInt() != 1 ||
        !root.value("devices").isArray()) {

        QMessageBox::warning(
            this,
            "Invalid Discovery File",
            "This is not a supported NetScope discovery export."
        );
        return;
    }

    const QJsonArray devices =
        root.value("devices").toArray();

    if (devices.size() > 2000) {
        QMessageBox::warning(
            this,
            "Import Failed",
            "A maximum of 2000 devices is supported per import."
        );
        return;
    }

    std::vector<DiscoveredHost> hosts;
    hosts.reserve(devices.size());

    for (const QJsonValue& value : devices) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();

        DiscoveredHost host;

        host.ipAddress =
            object.value("ipAddress").toString().toStdString();

        if (!validateIPv4(host.ipAddress)) {
            continue;
        }

        host.hostname =
            object.value("hostname").toString().toStdString();

        host.macAddress =
            object.value("macAddress").toString().toStdString();

        host.responding =
            object.value("responding").toBool();

        host.icmpResponded =
            object.value("icmpResponded").toBool();

        host.arpFound =
            object.value("arpFound").toBool();

        host.arpConfirmed =
            object.value("arpConfirmed").toBool();

        host.tcpResponded =
            object.value("tcpResponded").toBool();

        hosts.push_back(std::move(host));
    }

    if (hosts.empty()) {
        QMessageBox::information(
            this,
            "No Devices",
            "No valid IPv4 devices were found in this file."
        );
        return;
    }

    // The existing JSON export does not contain CIDR
    // information, so don't invent a subnet mask.
    showDiscoveredHosts(hosts, "Imported inventory");
}
