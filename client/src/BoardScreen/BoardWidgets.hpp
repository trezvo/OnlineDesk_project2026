#pragma once

#include "BoardScreenFwd.hpp"
#include <QDialog> 
#include <QString>
#include <QPainter>
#include <QGraphicsObject>
#include <QLineEdit>
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

public:

    QRectF boundingRect() const override {
        return QRectF(-50, -50, 100, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override {
        painter->setBrush(QBrush(QColor(R, G, B)));
        painter->drawRect(boundingRect());
    }

    void setPosUnnotify(const QPoint& newPos);

    explicit Widget(uint64_t widget_id);
    QPointF GetCoords();
};
