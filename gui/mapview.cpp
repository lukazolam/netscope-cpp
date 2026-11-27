
#include "mapview.h"

#include <QWheelEvent>
#include <QPainter>
#include <QGraphicsScene>
#include <QFrame>

MapView::MapView(QWidget* parent)
    : QGraphicsView(parent) {

    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);

    // Drag empty space to pan the map.
    setDragMode(QGraphicsView::ScrollHandDrag);

    // Zoom toward the mouse pointer.
    setTransformationAnchor(
        QGraphicsView::AnchorUnderMouse
    );

    setResizeAnchor(
        QGraphicsView::AnchorViewCenter
    );

    setFrameShape(QFrame::NoFrame);

    setBackgroundBrush(QColor(24, 28, 36));
}

void MapView::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    }
    else if (event->angleDelta().y() < 0) {
        zoomOut();
    }

    event->accept();
}

void MapView::zoomIn() {
    if (transform().m11() < 4.0) {
        scale(1.15, 1.15);
    }
}

void MapView::zoomOut() {
    if (transform().m11() > 0.15) {
        scale(1.0 / 1.15, 1.0 / 1.15);
    }
}

void MapView::fitMap() {
    if (!scene()) {
        return;
    }

    resetTransform();

    fitInView(
        scene()->sceneRect(),
        Qt::KeepAspectRatio
    );
}
