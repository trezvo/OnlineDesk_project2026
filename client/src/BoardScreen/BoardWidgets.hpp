#pragma once

#include "BoardScreenFwd.hpp"
#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOption>
#include <QMenu>
#include <QString>
#include <QGraphicsSceneHoverEvent>
#include <QLineEdit>
#include <QPen>
#include <string>
#include <utility>
#include <iostream>


enum class WidgetType : int {
    STICKER = 0,
    CIRCLE = 1,   
    RECTANGLE = 2,   
    TRIANGLE = 3,  
    TEXT = 4    
};

struct WidgetUpdate {
    uint64_t widget_id;
    int new_x, new_y;
    std::string content;
};

class EditableTextItem : public QGraphicsTextItem {
    Q_OBJECT

public:
    explicit EditableTextItem(QGraphicsItem* parent = nullptr);

signals:
    void textEditingFinished(const QString& text);

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

class Widget : public QGraphicsObject {
    Q_OBJECT

    int R_, G_, B_;
    uint64_t widget_id_;
    bool unnotify_{false};
    WidgetType type_;
    EditableTextItem* text_item_;  

    QRectF DeleteButtonRect() const;

signals:
    void updateSignal(WidgetUpdate upd);
    void deleteSignal(uint64_t widget_id);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private slots:
    void onTextEditingFinished(const QString& new_text);
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
    static std::string EncodeContent(WidgetType type, const QString& text);
    static std::pair<WidgetType, QString> DecodeContent(const std::string& content);
    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void setPosUnnotify(const QPoint& newPos);
    void setContentSilent(WidgetType type, const QString& text);

    explicit Widget(uint64_t widget_id, WidgetType type = WidgetType::STICKER);

    QPointF GetCoords() const;
    
    WidgetType GetType() const { 
        return type_; 
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

    QString GetText() const { 
        return text_item_->toPlainText(); 
    }

};
    explicit Widget(uint64_t widget_id);
    uint64_t GetId() const;
    QPointF GetCoords();
};
