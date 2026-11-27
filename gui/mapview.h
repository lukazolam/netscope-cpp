
#pragma once

#include <QGraphicsView>

class QWheelEvent;

class MapView : public QGraphicsView {
public:
    explicit MapView(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;

public:
    void zoomIn();
    void zoomOut();
    void fitMap();
};
