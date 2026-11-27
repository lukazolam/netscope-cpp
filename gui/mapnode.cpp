
#include "mapnode.h"
#include "mapedge.h"

#include <QGraphicsTextItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QVariant>

MapNode::MapNode(
    const QString& name,
    const QString& address,
    MapNodeType type
)
    : name_(name),
      address_(address),
      type_(type) {

    // Node dimensions
    setRect(0, 0, 170, 64);

    QColor background;
    QColor border;

    switch (type_) {
        case MapNodeType::Gateway:
            background = QColor(80, 90, 110);
            border = QColor(160, 170, 190);
            break;

        case MapNodeType::Subnet:
            background = QColor(33, 75, 135);
            border = QColor(80, 155, 255);
            break;

        case MapNodeType::Device:
            background = QColor(24, 95, 78);
            border = QColor(55, 190, 145);
            break;
    }

    setBrush(QBrush(background));
    setPen(QPen(border, 2));

    setFlag(
        QGraphicsItem::ItemIsMovable,
        true
    );

    setFlag(
        QGraphicsItem::ItemIsSelectable,
        true
    );

    setFlag(
        QGraphicsItem::ItemSendsGeometryChanges,
        true
    );

    setZValue(1);

    // Display name
    auto* nameText = new QGraphicsTextItem(name_, this);

    QFont nameFont;
    nameFont.setPointSize(11);
    nameFont.setBold(true);

    nameText->setFont(nameFont);
    nameText->setDefaultTextColor(Qt::white);
    nameText->setPos(10, 7);

    // Display address
    auto* addressText =
        new QGraphicsTextItem(address_, this);

    QFont addressFont;
    addressFont.setPointSize(9);

    addressText->setFont(addressFont);
    addressText->setDefaultTextColor(
        QColor(210, 225, 235)
    );

    addressText->setPos(10, 34);

    setToolTip(name_ + "\n" + address_);
}

void MapNode::addEdge(MapEdge* edge) {
    edges_.push_back(edge);
}

QString MapNode::name() const {
    return name_;
}

QString MapNode::address() const {
    return address_;
}

MapNodeType MapNode::nodeType() const {
    return type_;
}

QVariant MapNode::itemChange(
    GraphicsItemChange change,
    const QVariant& value
) {
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        for (MapEdge* edge : edges_) {
            if (edge) {
                edge->updatePosition();
            }
        }
    }

    return QGraphicsRectItem::itemChange(change, value);
}
