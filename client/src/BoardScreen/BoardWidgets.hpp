#pragma once

#include "BoardScreenFwd.hpp"
#include <QDialog> 
#include <QString>
#include <QPainter>
#include <QGraphicsObject>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QLineEdit>
#include <QPen>
#include <string>
#include <utility>
#include <iostream>

struct WidgetUpdate{
    uint64_t widget_id;
    int new_x, new_y;
    std::string text;
};

class Widget : public QGraphicsObject {

    Q_OBJECT

    int R = std::rand() % 256, G = std::rand() % 256, B = std::rand() % 256;

    uint64_t widget_id_;
    bool unnotify_{false};

    QRectF DeleteButtonRect() const;

signals:

    void updateSignal(WidgetUpdate upd);
    void deleteSignal(uint64_t widget_id);

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (unnotify_) {
            return value;
        }

        if (change == ItemPositionHasChanged) {
            qDebug() << "Moved to new position:" << pos();
            WidgetUpdate upd;
            upd.widget_id = widget_id_;
            upd.new_x = pos().x();
            upd.new_y = pos().y();
            emit updateSignal(std::move(upd));
        }

        return QGraphicsObject::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

public:

    QRectF boundingRect() const override {
        return QRectF(-50, -50, 100, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override {
        painter->save();

        painter->setBrush(QBrush(QColor(R, G, B)));
        painter->drawRect(boundingRect());

        const QRectF delete_button = DeleteButtonRect();
        painter->setBrush(QBrush(QColor("#d9534f")));
        painter->setPen(Qt::NoPen);
        painter->drawRect(delete_button);

        painter->setPen(QPen(Qt::white, 2));
        painter->drawLine(
            delete_button.topLeft() + QPointF(4, 4),
            delete_button.bottomRight() - QPointF(4, 4));
        painter->drawLine(
            QPointF(delete_button.right() - 4, delete_button.top() + 4),
            QPointF(delete_button.left() + 4, delete_button.bottom() - 4));

        painter->restore();
    }

    void setPosUnnotify(const QPoint& newPos);

    explicit Widget(uint64_t widget_id);
    uint64_t GetId() const;
    QPointF GetCoords();
};
