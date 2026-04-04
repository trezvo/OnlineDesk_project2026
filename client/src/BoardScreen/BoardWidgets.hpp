#pragma once

#include "BoardScreenFwd.hpp"
#include <QDialog> 
#include <QString>
#include <QPainter>
#include <QGraphicsObject>
#include <QLineEdit>
#include <string>
#include <utility>

struct WidgetUpdate{
    double x, y;
    std::string text;
};

class Widget : public QGraphicsObject {

    Q_OBJECT

    int R = std::rand() % 256, G = std::rand() % 256, B = std::rand() % 256;

    uint64_t widget_id_;

signals:

    void updateRequest(uint64_t widget_id, WidgetUpdate upd);
    void deleteRequest(uint64_t widget_id);

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionHasChanged) {
            qDebug() << "Moved to new position:" << pos();
            WidgetUpdate upd;
            upd.x = pos().x();
            upd.y = pos().y();
            emit updateRequest(widget_id_, std::move(upd));
        }
        
        return QGraphicsObject::itemChange(change, value);
    }

public:

    QRectF boundingRect() const override {
        return QRectF(-50, -50, 100, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override {
        painter->setBrush(QBrush(QColor(R, G, B)));
        painter->drawRect(boundingRect());
    }

    explicit Widget(uint64_t widget_id);
    QPointF GetCoords();
};
