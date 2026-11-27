
#pragma once

#include <QGraphicsLineItem>

class MapNode;

class MapEdge : public QGraphicsLineItem {
public:
    MapEdge(MapNode* source, MapNode* target);

    void updatePosition();

private:
    MapNode* source_;
    MapNode* target_;
};
