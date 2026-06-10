#include "BoardScreen.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QToolBar>
#include <QThread>
#include <QMessageBox>
#include <QWheelEvent>
#include <memory>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr double kZoomFactor = 1.2;
constexpr double kMinZoom = 0.2;
constexpr double kMaxZoom = 5.0;
}

BoardScreen::BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id, QWidget* parent)
    : QMainWindow(parent)
      , scene_(new QGraphicsScene(this))
      , scene_view_(new QGraphicsView(scene_, this))
      , grpc_client_(grpc_client)
      , board_id_(board_id)
      , gen64_(std::chrono::system_clock::now().time_since_epoch().count()) {
    qRegisterMetaType<online_desk::board::BoardUpdate>("online_desk::board::BoardUpdate");
    SetupUI();

    scene_->setSceneRect(0, 0, 800, 500);
    scene_->setBackgroundBrush(QBrush(QColor("#97d2f7")));
}

BoardScreen::~BoardScreen() = default;
void BoardScreen::SetupUI() {

    setWindowTitle(QString::fromStdString("Board № " + std::to_string(board_id_)));
    resize(1000, 600);

    setCentralWidget(scene_view_);

    scene_view_->setRenderHint(QPainter::Antialiasing);
    scene_view_->setDragMode(QGraphicsView::RubberBandDrag);
    scene_view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scene_view_->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    scene_view_->viewport()->installEventFilter(this);

    QToolBar *tool_bar = addToolBar("Actions");
    tool_bar->addSeparator();

    widget_type_selector_ = new QComboBox(this);

    widget_type_selector_->addItem("Стикер", QVariant(static_cast<int>(WidgetType::STICKER)));

    widget_type_selector_->addItem("Круг", QVariant(static_cast<int>(WidgetType::CIRCLE)));

    widget_type_selector_->addItem("Прямоугольник", QVariant(static_cast<int>(WidgetType::RECTANGLE)));

    widget_type_selector_->addItem("Треугольник", QVariant(static_cast<int>(WidgetType::TRIANGLE)));

    widget_type_selector_->addItem("Текст", QVariant(static_cast<int>(WidgetType::TEXT)));

    widget_type_selector_->addItem("Стрелка", QVariant(static_cast<int>(WidgetType::ARROW)));

    widget_type_selector_->addItem("Рисование", QVariant(static_cast<int>(WidgetType::DRAWING)));

    widget_type_selector_->addItem("Ластик", QVariant(static_cast<int>(WidgetType::ERASER)));

    tool_bar->addWidget(widget_type_selector_);
 
    connect(widget_type_selector_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        current_widget_type_ = static_cast<WidgetType>(widget_type_selector_->currentData().toInt());
        updateToolMode();
    });

    create_widget_button_ = new QPushButton("Создать виджет", this);
    tool_bar->addWidget(create_widget_button_);

    QPushButton* delete_widget_button = new QPushButton("Удалить выбранное", this);
    tool_bar->addWidget(delete_widget_button);

    QPushButton* export_png_button = new QPushButton("Экспорт PNG", this);
    tool_bar->addWidget(export_png_button);

    QPushButton* zoom_in_button = new QPushButton("+", this);
    tool_bar->addWidget(zoom_in_button);

    QPushButton* zoom_out_button = new QPushButton("-", this);
    tool_bar->addWidget(zoom_out_button);

    QPushButton* reset_zoom_button = new QPushButton("100%", this);
    tool_bar->addWidget(reset_zoom_button);

    QPushButton* create_snapshot_button = new QPushButton("Создать снапшот", this);
    tool_bar->addWidget(create_snapshot_button);

    QPushButton* back_button = new QPushButton("← Главное меню", this);
    tool_bar->addWidget(back_button);

    QThread* worker_thread = new QThread(this);
    worker_ = new BoardWorker(grpc_client_, board_id_);
    worker_->moveToThread(worker_thread);

    connect(create_widget_button_, &QPushButton::clicked, this, &BoardScreen::createWidget);
    connect(delete_widget_button, &QPushButton::clicked, this, &BoardScreen::deleteSelectedWidgets);
    connect(export_png_button, &QPushButton::clicked, this, &BoardScreen::exportBoardToPng);
    connect(zoom_in_button, &QPushButton::clicked, this, &BoardScreen::zoomIn);
    connect(zoom_out_button, &QPushButton::clicked, this, &BoardScreen::zoomOut);
    connect(reset_zoom_button, &QPushButton::clicked, this, &BoardScreen::resetZoom);
    connect(create_snapshot_button, &QPushButton::clicked, this, &BoardScreen::createSnapshot);
    connect(back_button, &QPushButton::clicked, this, &BoardScreen::onBackToMenuClicked);
    connect(this, &BoardScreen::sendSessionUpdate, worker_, &BoardWorker::sendSessionUpdate);
    connect(worker_thread, &QThread::started, worker_, &BoardWorker::runWorking);
    connect(worker_, &BoardWorker::printUpdate, this, &BoardScreen::acceptBoardUpdate);

    std::cout << this->thread() << ' ' << worker_->thread() <<  std::endl;

    scene_view_->show();
    worker_thread->start();

    if (!worker_thread->isRunning()) {
        // std::cout << "does not running" << std::endl;
    }
}

void BoardScreen::updateToolMode() {
    const bool interactive = (current_widget_type_ == WidgetType::ARROW ||current_widget_type_ == WidgetType::DRAWING || current_widget_type_ == WidgetType::ERASER);
 
    if (create_widget_button_) {
        create_widget_button_->setEnabled(!interactive);
    }
 
    if (interactive) {
        scene_view_->setDragMode(QGraphicsView::NoDrag);
        scene_view_->viewport()->setCursor(
            current_widget_type_ == WidgetType::ERASER ? Qt::CrossCursor : Qt::CrossCursor);
    } else {

        placing_arrow_ = false;
        is_drawing_ = false;
        is_erasing_ = false;
        if (drawing_preview_) {
            scene_->removeItem(drawing_preview_);
            delete drawing_preview_;
            drawing_preview_ = nullptr;
        }
        current_drawing_points_.clear();
        scene_view_->setDragMode(QGraphicsView::RubberBandDrag);
        scene_view_->viewport()->unsetCursor();
    }
}

Widget* BoardScreen::ProduceWidget(uint64_t widget_id, WidgetType type) {
    Widget* new_widget = new Widget(widget_id, type);
    connect(new_widget, &Widget::updateSignal, this, &BoardScreen::requestUpdate);
    connect(new_widget, &Widget::deleteSignal, this, &BoardScreen::requestDelete);
    return new_widget;
}

void BoardScreen::createSnapshot() {
    grpc_client_->createBoardSnapshot(board_id_);
}

void BoardScreen::createWidget() {
    if (current_widget_type_ == WidgetType::ARROW || current_widget_type_ == WidgetType::DRAWING || current_widget_type_ == WidgetType::ERASER) {
        return;
    }

    const WidgetType type = current_widget_type_;
    const uint64_t widget_id = gen64_();
 
    Widget* new_widget = ProduceWidget(widget_id, type);
    new_widget->setPosUnnotify({200, 150});
    scene_->addItem(new_widget);
 
    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);
        board_widgets_[widget_id] = new_widget;
    }

    online_desk::board::BoardUpdate request;
    request.set_action_type(online_desk::board::CREATE);
    request.set_widget_id(widget_id);
 
    auto [x, y] = new_widget->GetCoords();
 
    online_desk::board::WidgetInfo* difference = request.mutable_update_data();
    difference->set_coord_x(x);
    difference->set_coord_y(y);
    difference->set_content(Widget::EncodeContent(type, ""));
 
    worker_->sendSessionUpdate(std::move(request));
}

void BoardScreen::createArrow(QPointF p1, QPointF p2) {
    const uint64_t widget_id = gen64_();
    const QString  data = QString("%1,%2,%3,%4").arg(static_cast<int>(p1.x())).arg(static_cast<int>(p1.y())).arg(static_cast<int>(p2.x())).arg(static_cast<int>(p2.y()));

    Widget* w = ProduceWidget(widget_id, WidgetType::ARROW);
    w->setContentSilent(WidgetType::ARROW, data);
    w->setPosUnnotify({0, 0});
    scene_->addItem(w);
    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);
        board_widgets_[widget_id] = w;
    }
 
    online_desk::board::BoardUpdate request;
    request.set_action_type(online_desk::board::CREATE);
    request.set_widget_id(widget_id);
    auto* diff = request.mutable_update_data();
    diff->set_coord_x(0);
    diff->set_coord_y(0);
    diff->set_content(Widget::EncodeContent(WidgetType::ARROW, data));
    worker_->sendSessionUpdate(std::move(request));
}

void BoardScreen::createDrawing(const std::vector<QPointF>& points) {
    if (points.size() < 2) return;
 
    const uint64_t widget_id = gen64_();
 
    QStringList parts;
    for (const QPointF& pt : points) {
        parts << QString::number(static_cast<int>(pt.x())) << QString::number(static_cast<int>(pt.y()));
    }
    const QString data = parts.join(",");
 
    Widget* w = ProduceWidget(widget_id, WidgetType::DRAWING);
    w->setContentSilent(WidgetType::DRAWING, data);
    w->setPosUnnotify({0, 0});
    scene_->addItem(w);

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);
        board_widgets_[widget_id] = w;
    }
 
    online_desk::board::BoardUpdate request;
    request.set_action_type(online_desk::board::CREATE);
    request.set_widget_id(widget_id);
    auto* diff = request.mutable_update_data();
    diff->set_coord_x(0);
    diff->set_coord_y(0);
    diff->set_content(Widget::EncodeContent(WidgetType::DRAWING, data));
    worker_->sendSessionUpdate(std::move(request));
}



void BoardScreen::deleteSelectedWidgets() {
    std::vector<uint64_t> widget_ids;

    for (QGraphicsItem* item : scene_->selectedItems()) {
        Widget* widget = dynamic_cast<Widget*>(item);
        if (widget == nullptr) {
            continue;
        }

        widget_ids.push_back(widget->GetId());
    }

    for (uint64_t widget_id : widget_ids) {
        requestDelete(widget_id);
    }
}

void BoardScreen::exportBoardToPng() {
    QRectF scene_rect = scene_->sceneRect();

    if (scene_rect.isEmpty()) {
        QMessageBox::warning(this, "Экспорт PNG", "Невозможно экспортировать пустую доску");
        return;
    }

    QString default_path = QDir::homePath()
                           + QString("/board-%1.png").arg(board_id_);

    QString file_name = QFileDialog::getSaveFileName(
        this,
        "Экспорт доски в PNG",
        default_path,
        "PNG images (*.png)"
    );

    if (file_name.isEmpty()) {
        return;
    }

    if (!file_name.endsWith(".png", Qt::CaseInsensitive)) {
        file_name += ".png";
    }

    QSize image_size = scene_rect.size().toSize();
    QImage image(image_size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene_->render(
        &painter,
        QRectF(QPointF(0, 0), QSizeF(image_size)),
        scene_rect
    );
    painter.end();

    if (!image.save(file_name, "PNG")) {
        QMessageBox::warning(this, "Экспорт PNG", "Не удалось сохранить PNG-файл");
        return;
    }

    QMessageBox::information(this, "Экспорт PNG", "Доска сохранена в PNG");
}

void BoardScreen::applyZoom(double factor) {
    double new_zoom = current_zoom_ * factor;

    if (new_zoom < kMinZoom || new_zoom > kMaxZoom) {
        return;
    }

    scene_view_->scale(factor, factor);
    current_zoom_ = new_zoom;
}

void BoardScreen::zoomIn() {
    applyZoom(kZoomFactor);
}

void BoardScreen::zoomOut() {
    applyZoom(1.0 / kZoomFactor);
}

void BoardScreen::resetZoom() {
    scene_view_->resetTransform();
    current_zoom_ = 1.0;
}

bool BoardScreen::eventFilter(QObject* watched, QEvent* event) {
    if (watched != scene_view_->viewport()) {
        return QMainWindow::eventFilter(watched, event);
    }
    
    if (event->type() == QEvent::Wheel) {
        auto* we = static_cast<QWheelEvent*>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            applyZoom(we->angleDelta().y() > 0 ? kZoomFactor : 1.0 / kZoomFactor);
            we->accept();
            return true;
        }
    }

    if (current_widget_type_ == WidgetType::ARROW && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPointF sp = scene_view_->mapToScene(me->position().toPoint());
        if (me->button() == Qt::LeftButton) {
            if (!placing_arrow_) {
                arrow_start_   = sp;
                placing_arrow_ = true;
            } else {
                createArrow(arrow_start_, sp);
                placing_arrow_ = false;
            }
            return true;
        }
        if (me->button() == Qt::RightButton) {
            placing_arrow_ = false;
            return true;
        }
    }

    if (current_widget_type_ == WidgetType::DRAWING) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                is_drawing_ = true;
                current_drawing_points_.clear();
                current_drawing_points_.push_back(
                    scene_view_->mapToScene(me->position().toPoint()));
                if (drawing_preview_) {
                    scene_->removeItem(drawing_preview_);
                    delete drawing_preview_;
                }
                drawing_preview_ = scene_->addPath(
                    QPainterPath(),
                    QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                drawing_preview_->setZValue(100);
                return true;
            }
        }
 
        if (event->type() == QEvent::MouseMove && is_drawing_) {
            auto* me = static_cast<QMouseEvent*>(event);
            const QPointF sp = scene_view_->mapToScene(me->position().toPoint());
            if (current_drawing_points_.empty() ||
                QLineF(current_drawing_points_.back(), sp).length() >= 3.0) {
                current_drawing_points_.push_back(sp);
                QPainterPath path;
                path.moveTo(current_drawing_points_.front());
                for (size_t i = 1; i < current_drawing_points_.size(); ++i) {
                    path.lineTo(current_drawing_points_[i]);
                }
                drawing_preview_->setPath(path);
            }
            return true;
        }
 
        if (event->type() == QEvent::MouseButtonRelease && is_drawing_) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                is_drawing_ = false;
                if (drawing_preview_) {
                    scene_->removeItem(drawing_preview_);
                    delete drawing_preview_;
                    drawing_preview_ = nullptr;
                }
                if (current_drawing_points_.size() > 1) {
                    createDrawing(current_drawing_points_);
                }
                current_drawing_points_.clear();
                return true;
            }
        }
    }

    if (current_widget_type_ == WidgetType::ERASER) {
        const bool press   = event->type() == QEvent::MouseButtonPress;
        const bool move    = event->type() == QEvent::MouseMove;
        const bool release = event->type() == QEvent::MouseButtonRelease;
 
        auto* me = static_cast<QMouseEvent*>(event);
 
        if (press && me->button() == Qt::LeftButton) {
            is_erasing_ = true;
        }
        if (release && me->button() == Qt::LeftButton) {
            is_erasing_ = false;
            return true;
        }
 
        if (is_erasing_ && (press || move)) {
            const QPointF sp = scene_view_->mapToScene(me->position().toPoint());
            std::vector<uint64_t> to_erase;
            for (QGraphicsItem* item : scene_->items(sp, Qt::IntersectsItemShape)) {
                Widget* w = dynamic_cast<Widget*>(item);
                if (w) to_erase.push_back(w->GetId());
            }
            for (uint64_t id : to_erase) {
                requestDelete(id);
            }
            return true;
        }
    }
 
    return QMainWindow::eventFilter(watched, event);
}

void BoardScreen::acceptBoardUpdate(BoardUpdate upd) {
    // std::cout << "acceptBoardUpdate action=" << upd.action_type() << std::endl;
     if (is_closing_ && upd.action_type() != online_desk::board::BOARD_DELETED){
        return;
     }
    using namespace online_desk::board;
 
    const ActionType action = upd.action_type();
    const uint64_t widget_id = upd.widget_id();
    const WidgetInfo& info = upd.update_data();
    Widget* widget_ptr = nullptr;
    // std::cout << "online income widget_id=" << widget_id << std::endl;  

    switch (action) {
        case (ActionType::CREATE): {
            // std::cout << "create widget, id=" << widget_id << std::endl;
 
            const auto [type, text] = Widget::DecodeContent(info.content());
            widget_ptr = ProduceWidget(widget_id, type);
            widget_ptr->setContentSilent(type, text);

            {
                std::lock_guard<std::mutex> lock(widget_edit_mutex_);
                board_widgets_[widget_id] = widget_ptr;
            }
 
            widget_ptr->setPosUnnotify({info.coord_x(), info.coord_y()});
            scene_->addItem(widget_ptr);
            break;
        } 

        case (ActionType::BOARD_DELETED): {
            QMetaObject::invokeMethod(this, "onBoardDeleted", Qt::QueuedConnection);
            break;
        }
 
        case (ActionType::DELETE): {
            {
                std::lock_guard<std::mutex> lock(widget_edit_mutex_);

                if (!board_widgets_.contains(widget_id)) {
                    return;
                }

                widget_ptr = board_widgets_[widget_id];
                board_widgets_.erase(widget_id);
            }

            scene_->removeItem(widget_ptr);
            widget_ptr->deleteLater();
            break;
        }
 
        case (ActionType::UPDATE): {
            // std::cout << "update widget, id=" << widget_id << std::endl;
 
            bool item_exists = true;
 
            WidgetType upd_type = WidgetType::STICKER;
            QString upd_text;
            const bool has_content = !info.content().empty();
            if (has_content) {
                const auto [t, tx] = Widget::DecodeContent(info.content());
                upd_type = t;
                upd_text = tx;
            }
            
            {
                std::lock_guard<std::mutex> lock(widget_edit_mutex_);
                item_exists = board_widgets_.contains(widget_id);
 
                if (!item_exists) {
                    board_widgets_[widget_id] = ProduceWidget(widget_id, upd_type);
                }

                widget_ptr = board_widgets_[widget_id];
            }
 
            widget_ptr->setPosUnnotify({info.coord_x(), info.coord_y()});
 
            if (has_content){
                widget_ptr->setContentSilent(upd_type, upd_text);
            }
 

            if (!item_exists) {
                scene_->addItem(widget_ptr);
            }
        } break;
        default: {
        } break;
    }

}

void BoardScreen::requestUpdate(WidgetUpdate upd) {

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);

        if (!board_widgets_.contains(upd.widget_id)) {
            return;
        }

    }

    online_desk::board::BoardUpdate request;

    online_desk::board::WidgetInfo* difference = request.mutable_update_data();
    difference->set_coord_x(upd.new_x);
    difference->set_coord_y(upd.new_y);
    difference->set_content(upd.content);

    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::UPDATE);
    request.set_widget_id(upd.widget_id);

    worker_->sendSessionUpdate(std::move(request));

    // std::cout << "after emit" <<std::endl;
}


void BoardScreen::requestDelete(uint64_t widget_id) {

    Widget* widget_ptr = nullptr;

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);

        if (!board_widgets_.contains(widget_id)) {
            return;
        }

        widget_ptr = board_widgets_[widget_id];
        board_widgets_.erase(widget_id);
    }

    scene_->removeItem(widget_ptr);
    widget_ptr->deleteLater();

    online_desk::board::BoardUpdate request;
    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::DELETE);
    request.set_widget_id(widget_id);

    worker_->sendSessionUpdate(std::move(request));
}

void BoardScreen::onBoardDeleted() {
    shutdownWorker();
    emit boardDeletedByOwner(board_id_);
    emit boardClosed();
    close();         
}

void BoardScreen::shutdownWorker() {
    if (worker_shutdown_.exchange(true)){
        return;
    }
    is_closing_ = true;

    if (drawing_preview_) {
        scene_->removeItem(drawing_preview_);
        delete drawing_preview_;
        drawing_preview_ = nullptr;
    }

    if (worker_) {
        worker_->Shutdown();
        worker_->thread()->quit();
        worker_->thread()->wait();
        delete worker_;
        worker_ = nullptr;
    }
}

void BoardScreen::closeEvent(QCloseEvent* event) {
    shutdownWorker();
    QMainWindow::closeEvent(event);
}

void BoardScreen::onBackToMenuClicked() {
    shutdownWorker();
    emit boardClosed();
    close();
}