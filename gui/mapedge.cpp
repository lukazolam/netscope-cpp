
#include "mapedge.h"
#include "mapnode.h"

#include <QPen>
#include <QColor>

MapEdge::MapEdge(
    MapNode* source,
    MapNode* target
)
    : source_(source),
      target_(target) {

    QPen pen(QColor(110, 125, 145));
    pen.setWidthF(2.0);
    pen.setStyle(Qt::DashLine);

    setPen(pen);

    // Draw edges behind nodes.
    setZValue(-1);

    source_->addEdge(this);
    target_->addEdge(this);

    updatePosition();
}

void MapEdge::updatePosition() {
    if (!source_ || !target_) {
        return;
    }

    const QPointF sourceCenter =
        source_->sceneBoundingRect().center();

    const QPointF targetCenter =
        target_->sceneBoundingRect().center();

    setLine(QLineF(sourceCenter, targetCenter));
}
