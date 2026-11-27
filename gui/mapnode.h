
#pragma once

#include <QGraphicsRectItem>
#include <QString>
#include <QColor>

#include <vector>

class MapEdge;

enum class MapNodeType {
    Device,
    Subnet,
    Gateway
};

class MapNode : public QGraphicsRectItem {
public:
    MapNode(
        const QString& name,
        const QString& address,
        MapNodeType type
    );

    void addEdge(MapEdge* edge);

    QString name() const;
    QString address() const;
    MapNodeType nodeType() const;

protected:
    QVariant itemChange(
        GraphicsItemChange change,
        const QVariant& value
    ) override;

private:
    QString name_;
    QString address_;
    MapNodeType type_;

    std::vector<MapEdge*> edges_;
};
