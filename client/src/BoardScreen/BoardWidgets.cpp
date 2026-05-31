#include "BoardWidgets.hpp"

#include <QLabel>
#include <QFormLayout>
#include <QPushButton>
#include <QPoint>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPen>

Widget::Widget(uint64_t widget_id)
    : widget_id_(widget_id) {

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
}

void Widget::setPosUnnotify(const QPoint& newPos) {
    unnotify_ = true;
    setPos(newPos);
    unnotify_ = false;
}

QRectF Widget::DeleteButtonRect() const {
    return QRectF(30, -46, 16, 16);
}

void Widget::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && DeleteButtonRect().contains(event->pos())) {
        event->accept();
        emit deleteSignal(widget_id_);
        return;
    }

    QGraphicsObject::mousePressEvent(event);
}

void Widget::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (DeleteButtonRect().contains(event->pos())) {
        setCursor(Qt::PointingHandCursor);
    } else {
        unsetCursor();
    }

    QGraphicsObject::hoverMoveEvent(event);
}

void Widget::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    unsetCursor();
    QGraphicsObject::hoverLeaveEvent(event);
}

uint64_t Widget::GetId() const {
    return widget_id_;
}

QPointF Widget::GetCoords() {
    return pos();
}
