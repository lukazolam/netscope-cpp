
#pragma once

#include <QWidget>
#include <QHash>
#include <QString>

#include <vector>

#include "network.h"
#include "mapnode.h"

class QGraphicsScene;
class QLabel;
class QPushButton;
class QShowEvent;

class MapView;
class MapNode;

class NetworkMapPage : public QWidget {
public:
    explicit NetworkMapPage(
        QWidget* parent = nullptr
    );
    void showDiscoveredHosts(
        const std::vector<DiscoveredHost>& hosts,
        const QString& subnetCidr
    );

protected:
    void showEvent(QShowEvent* event) override;

private:
    void loadDemoTopology();
    void updateSelectionDetails();
    void loadDiscoveryJson();

    MapNode* addNode(
        const QString& name,
        const QString& address,
        MapNodeType type,
        qreal x,
        qreal y
    );

    void connectNodes(
        MapNode* source,
        MapNode* target
    );

    QGraphicsScene* scene_ = nullptr;
    MapView* view_ = nullptr;

    QLabel* detailsLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QPushButton* fitButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QHash<QString, DiscoveredHost> discoveredHosts_;
};
