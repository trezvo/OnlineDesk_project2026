#include "BoardScreen.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QImage>
#include <QPainter>
#include <QPushButton>
#include <QToolBar>
#include <QThread>
#include <QMessageBox>
#include <memory>
#include <chrono>
#include <iostream>
#include <vector>

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
    resize(900, 600);

    setCentralWidget(scene_view_);

    scene_view_->setRenderHint(QPainter::Antialiasing);
    scene_view_->setDragMode(QGraphicsView::RubberBandDrag);

    QToolBar *tool_bar = addToolBar("Actions");
    tool_bar->addSeparator();

    QPushButton* create_widget_button = new QPushButton("Создать виджет", this);
    tool_bar->addWidget(create_widget_button);

    QPushButton* delete_widget_button = new QPushButton("Удалить выбранное", this);
    tool_bar->addWidget(delete_widget_button);

    QPushButton* export_png_button = new QPushButton("Экспорт PNG", this);
    tool_bar->addWidget(export_png_button);

    QPushButton* create_snapshot_button = new QPushButton("Создать снапшот", this);
    tool_bar->addWidget(create_snapshot_button);

    QThread* worker_thread = new QThread(this);
    worker_ = new BoardWorker(grpc_client_, board_id_);

    worker_->moveToThread(worker_thread);

    connect(create_widget_button, &QPushButton::clicked, this, &BoardScreen::createWidget);
    connect(delete_widget_button, &QPushButton::clicked, this, &BoardScreen::deleteSelectedWidgets);
    connect(export_png_button, &QPushButton::clicked, this, &BoardScreen::exportBoardToPng);
    connect(create_snapshot_button, &QPushButton::clicked, this, &BoardScreen::createSnapshot);

    connect(worker_thread, &QThread::started, worker_, &BoardWorker::runWorking);
    connect(this, &BoardScreen::sendSessionUpdate, worker_, &BoardWorker::sendSessionUpdate);

    std::cout << this->thread() << ' ' << worker_->thread() <<  std::endl;

    connect(worker_, &BoardWorker::printUpdate, this, &BoardScreen::acceptBoardUpdate);
    connect(worker_thread, &QThread::finished, worker_, &QObject::deleteLater);

    scene_view_->show();
    worker_thread->start();

    if (!worker_thread->isRunning()) {
        std::cout << "does not running" << std::endl;
    }
}

Widget* BoardScreen::ProduceWidget(uint64_t widget_id) {
    Widget* new_widget = new Widget(widget_id);
    connect(new_widget, &Widget::updateSignal, this, &BoardScreen::requestUpdate);
    connect(new_widget, &Widget::deleteSignal, this, &BoardScreen::requestDelete);
    return new_widget;
}

void BoardScreen::createSnapshot() {
    grpc_client_->createBoardSnapshot(board_id_);
}

void BoardScreen::createWidget() {
    uint64_t widget_id = gen64_();

    Widget* new_widget = ProduceWidget(widget_id);
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

    worker_->sendSessionUpdate(std::move(request)); //TODO
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

void BoardScreen::acceptBoardUpdate(BoardUpdate upd) {

    using namespace online_desk::board;

    ActionType action = upd.action_type();
    uint64_t widget_id = upd.widget_id();

    std::cout << "online income widget_id=" << widget_id << std::endl;

    const WidgetInfo& info = upd.update_data();

    Widget* widget_ptr = nullptr;

    switch (action) {
        case (ActionType::CREATE): {
            std::cout << "create widget, id=" << widget_id << std::endl;
            widget_ptr = ProduceWidget(widget_id);

            {
                std::lock_guard<std::mutex> lock(widget_edit_mutex_);
                board_widgets_[widget_id] = widget_ptr;
            }

            widget_ptr->setPosUnnotify({info.coord_x(), info.coord_y()});
            scene_->addItem(widget_ptr);

        } break;
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

        } break;

        case (ActionType::UPDATE): {
            std::cout << "update widget, id=" << widget_id << std::endl;

            bool item_exists = true;
            {
                std::lock_guard<std::mutex> lock(widget_edit_mutex_);

                item_exists = board_widgets_.contains(widget_id);

                if (!item_exists) {
                    board_widgets_[widget_id] = ProduceWidget(widget_id);
                }

                widget_ptr = board_widgets_[widget_id];
            }

            widget_ptr->setPosUnnotify({info.coord_x(), info.coord_y()});

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

    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::UPDATE);
    request.set_widget_id(upd.widget_id);
    /*
    std::cout << "update request got to end of Board method" << std::endl;

    qDebug() << "[MainWindow::handleData] Поток выполнения:"
                // << QThread::currentThreadId();
    qDebug() << "[MainWindow::handleData] Главный поток:"
                << QCoreApplication::instance()->thread()->currentThreadId();
    qDebug() << "[MainWindow::handleData] Главный поток:" << qMetaTypeId<BoardUpdate>();
    */
    worker_->sendSessionUpdate(std::move(request));

    std::cout << "after emit" <<std::endl;
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
    QMessageBox::information(this, "Доска удалена", "Эта доска была удалена владельцем");

    emit boardClosed();

    if (worker_) {
        worker_->Shutdown();
        worker_->thread()->quit();
        worker_->thread()->wait();
    }

    this->close();
}

void BoardScreen::shutdownWorker() {
    if (worker_) {
        worker_->Shutdown();
        worker_->thread()->quit();
        worker_->thread()->wait();
    }
}
