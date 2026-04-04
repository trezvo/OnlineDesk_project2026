#include "BoardWidgets.hpp"

#include <QLabel>
#include <QFormLayout>
#include <QPushButton>
#include <QPoint>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QHBoxLayout>

Widget::Widget(uint64_t widget_id)
    : widget_id_(widget_id) {
    
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
}

QPointF Widget::GetCoords() {
    return pos();
}

