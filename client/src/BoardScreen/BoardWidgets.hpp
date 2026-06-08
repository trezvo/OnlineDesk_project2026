#pragma once

#include "BoardScreenFwd.hpp"
#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPainterPath>
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
    TEXT = 4,
    ARROW = 5,
    DRAWING = 6,
    ERASER = 7
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

    QPointF p1_, p2_;
    QPainterPath drawing_path_;
    QRectF cached_bounding_rect_{-50, -50, 100, 100};

    QRectF DeleteButtonRect() const;

signals:
    void updateSignal(WidgetUpdate upd);
    void deleteSignal(uint64_t widget_id);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private slots:
    void onTextEditingFinished(const QString& new_text);

public:
    explicit Widget(uint64_t widget_id, WidgetType type = WidgetType::STICKER);

    static std::string EncodeContent(WidgetType type, const QString& text);
    static std::pair<WidgetType, QString> DecodeContent(const std::string& content);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    void setPosUnnotify(const QPoint& newPos);
    void setContentSilent(WidgetType type, const QString& text);

    QPointF GetCoords() const;
    uint64_t GetId() const{
        return widget_id_;
    }
    
    WidgetType GetType() const { 
        return type_; 
    }
    
    QString GetText() const { 
        return text_item_ ? text_item_->toPlainText() : QString();
    }
};