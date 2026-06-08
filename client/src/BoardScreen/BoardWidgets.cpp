#include "BoardWidgets.hpp"
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPolygonF>
#include <QTextOption>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QTextDocument>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>

EditableTextItem::EditableTextItem(QGraphicsItem* parent)
    : QGraphicsTextItem(parent) {
    setTextInteractionFlags(Qt::NoTextInteraction);
}

void EditableTextItem::focusOutEvent(QFocusEvent* event) {
    setTextInteractionFlags(Qt::NoTextInteraction);
    emit textEditingFinished(toPlainText());
    QGraphicsTextItem::focusOutEvent(event);
}

void EditableTextItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        clearFocus(); 
        return;
    }
    QGraphicsTextItem::keyPressEvent(event);
}

Widget::Widget(uint64_t widget_id, WidgetType type)
    : widget_id_(widget_id)
    , type_(type)
    , R_(std::rand() % 160 + 60)
    , G_(std::rand() % 160 + 60)
    , B_(std::rand() % 160 + 60) {

    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    const bool needs_text = (type != WidgetType::ARROW && type != WidgetType::DRAWING);

    if (needs_text){
        setFlag(QGraphicsItem::ItemIsMovable);
        text_item_ = new EditableTextItem(this);
        text_item_->setTextWidth(84);
        text_item_->setPos(-42, -32);
        text_item_->setDefaultTextColor(Qt::black);
        text_item_->setZValue(1);

        QTextOption opt;
        opt.setAlignment(Qt::AlignCenter);
        text_item_->document()->setDefaultTextOption(opt);

        connect(text_item_, &EditableTextItem::textEditingFinished, this, &Widget::onTextEditingFinished);
    }
    else{
        text_item_ = nullptr;
    }
}

std::string Widget::EncodeContent(WidgetType type, const QString& text) {
    return std::to_string(static_cast<int>(type)) + "|" + text.toStdString();
}   

std::pair<WidgetType, QString> Widget::DecodeContent(const std::string& content) {
    if (content.empty()) {
        return {WidgetType::STICKER, ""};
    }
    const auto sep = content.find('|');
    if (sep == std::string::npos) {
        return {
            WidgetType::STICKER, 
            QString::fromStdString(content)
        };
    }
    try {
        int type_int = std::stoi(content.substr(0, sep));
        if (type_int < 0 || type_int > 7) type_int = 0;
        return {
            static_cast<WidgetType>(type_int),
            QString::fromStdString(content.substr(sep + 1))
        };
    } catch (...) {
        return {WidgetType::STICKER, ""};
    }
}

QRectF Widget::boundingRect() const {
     if (type_ == WidgetType::ARROW || type_ == WidgetType::DRAWING) {
        return cached_bounding_rect_;
    }
    return QRectF(-50, -50, 100, 100);
}

void Widget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*unused*/) {
    if (type_ == WidgetType::ARROW) {
        QPen pen(Qt::black, 2);
        if (option->state & QStyle::State_Selected)
            pen = QPen(Qt::blue, 2, Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::black);
        painter->drawLine(p1_, p2_);
        if (p1_ != p2_) {
            const double dx = p2_.x() - p1_.x();
            const double dy = p2_.y() - p1_.y();
            const double angle = std::atan2(dy, dx);
            const double sz = 12.0;
            const QPointF ap1 = p2_ - QPointF(sz * std::cos(angle - M_PI / 6), sz * std::sin(angle - M_PI / 6));
            const QPointF ap2 = p2_ - QPointF(sz * std::cos(angle + M_PI / 6), sz * std::sin(angle + M_PI / 6));
            QPolygonF head;
            head << p2_ << ap1 << ap2;
            painter->drawPolygon(head);
        }
        return;
    }

    if (type_ == WidgetType::DRAWING) {
        QPen pen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        if (option->state & QStyle::State_Selected)
            pen.setColor(Qt::blue);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(drawing_path_);
        return;
    }

    QColor fill(R_, G_, B_, 220);
    if (type_ == WidgetType::TEXT) {
        fill = QColor(255, 255, 255, 210);
    }
    painter->setBrush(fill);

    QPen pen(Qt::black, 1.5);
    if (option->state & QStyle::State_Selected) {
        pen = QPen(Qt::blue, 2, Qt::DashLine);
    }
    painter->setPen(pen);

    switch (type_) {
        case WidgetType::STICKER:
        case WidgetType::RECTANGLE: {
            painter->drawRect(QRectF(-50, -50, 100, 100));
            break;
        }
        case WidgetType::CIRCLE: {
            painter->drawEllipse(QRectF(-50, -50, 100, 100));
            break;
        }
        case WidgetType::TRIANGLE: {
            QPolygonF tri;
            tri << QPointF(0, -50) << QPointF(50, 50) << QPointF(-50, 50);
            painter->drawPolygon(tri);
            break;
        }
        case WidgetType::TEXT: {
            painter->drawRoundedRect(QRectF(-50, -50, 100, 100), 6, 6);
            break;
        }
    }
}

void Widget::setPosUnnotify(const QPoint& newPos) {
    unnotify_ = true;
    setPos(newPos);
    unnotify_ = false;
}

void Widget::setContentSilent(WidgetType type, const QString& text) {
    type_ = type;
 
    if (type == WidgetType::ARROW) {
        const auto parts = text.split(',');
        if (parts.size() == 4) {
            p1_ = QPointF(parts[0].toInt(), parts[1].toInt());
            p2_ = QPointF(parts[2].toInt(), parts[3].toInt());
            const double xmin = std::min(p1_.x(), p2_.x()) - 15;
            const double ymin = std::min(p1_.y(), p2_.y()) - 15;
            const double xmax = std::max(p1_.x(), p2_.x()) + 15;
            const double ymax = std::max(p1_.y(), p2_.y()) + 15;
            prepareGeometryChange();
            cached_bounding_rect_ = QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
        }
        update();
        return;
    }
 
    if (type == WidgetType::DRAWING) {
        const auto parts = text.split(',');
        drawing_path_ = QPainterPath();
        const int n = parts.size();
        if (n >= 2) {
            drawing_path_.moveTo(parts[0].toInt(), parts[1].toInt());
            for (int i = 2; i + 3 < n; i += 2) {
                const QPointF ctrl(parts[i].toInt(),     parts[i + 1].toInt());
                const QPointF next(parts[i + 2].toInt(), parts[i + 3].toInt());
                const QPointF mid((ctrl.x() + next.x()) / 2.0,
                                  (ctrl.y() + next.y()) / 2.0);
                drawing_path_.quadTo(ctrl, mid);
            }

            if (n >= 4) {
                drawing_path_.lineTo(parts[n - 2].toInt(), parts[n - 1].toInt());
            }
 
            prepareGeometryChange();
            cached_bounding_rect_ = drawing_path_.boundingRect().adjusted(-5, -5, 5, 5);
        }
        update();
        return;
    }

    if (text_item_) {
        QSignalBlocker blocker(text_item_);
        if (text_item_->toPlainText() != text) {
            text_item_->setPlainText(text);
        }
    }
    update();
}

void Widget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (text_item_) {
        text_item_->setTextInteractionFlags(Qt::TextEditorInteraction);
        text_item_->setFocus();
    }
    QGraphicsObject::mouseDoubleClickEvent(event);
}

void Widget::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* deleteAct = menu.addAction("Удалить");
    if (menu.exec(event->screenPos()) == deleteAct) {
        emit deleteSignal(widget_id_);
    }
}

void Widget::onTextEditingFinished(const QString& new_text) {
    WidgetUpdate upd;
    upd.widget_id = widget_id_;
    upd.new_x = static_cast<int>(pos().x());
    upd.new_y = static_cast<int>(pos().y());
    upd.content = EncodeContent(type_, new_text);
    emit updateSignal(std::move(upd));
}

void Widget::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (type_ != WidgetType::ARROW && type_ != WidgetType::DRAWING && event->button() == Qt::LeftButton && DeleteButtonRect().contains(event->pos())) {
        event->accept();
        emit deleteSignal(widget_id_);
        return;
    }
    QGraphicsObject::mousePressEvent(event);
}

void Widget::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (type_ != WidgetType::ARROW && type_ != WidgetType::DRAWING && DeleteButtonRect().contains(event->pos())) {
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


QVariant Widget::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (!unnotify_ && change == ItemPositionHasChanged && type_ != WidgetType::ARROW && type_ != WidgetType::DRAWING) {
        WidgetUpdate upd;
        upd.widget_id = widget_id_;
        upd.new_x = static_cast<int>(pos().x());
        upd.new_y = static_cast<int>(pos().y());
        upd.content = EncodeContent(type_, text_item_ ? text_item_->toPlainText() : QString());
        emit updateSignal(std::move(upd));
    }
    return QGraphicsObject::itemChange(change, value);
}

QPointF Widget::GetCoords() const {
    return pos();
}

QRectF Widget::DeleteButtonRect() const {
    return QRectF(30, -46, 16, 16);
}