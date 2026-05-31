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

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

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
        if (type_int < 0 || type_int > 4) type_int = 0;
        return {
            static_cast<WidgetType>(type_int),
            QString::fromStdString(content.substr(sep + 1))
        };
    } catch (...) {
        return {WidgetType::STICKER, ""};
    }
}

QRectF Widget::boundingRect() const {
    return QRectF(-50, -50, 100, 100);
}

void Widget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*unused*/) {

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
    QSignalBlocker blocker(text_item_);
    if (text_item_->toPlainText() != text) {
        text_item_->setPlainText(text);
    }
    update();
}

void Widget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    text_item_->setTextInteractionFlags(Qt::TextEditorInteraction);
    text_item_->setFocus();
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

QVariant Widget::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (!unnotify_ && change == ItemPositionHasChanged) {
        WidgetUpdate upd;
        upd.widget_id = widget_id_;
        upd.new_x = static_cast<int>(pos().x());
        upd.new_y = static_cast<int>(pos().y());
        upd.content = EncodeContent(type_, text_item_->toPlainText());
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
