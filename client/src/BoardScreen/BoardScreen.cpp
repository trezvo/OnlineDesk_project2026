#include "BoardScreen.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"
#include <QCoreApplication>
#include <QPushButton>
#include <QToolBar>
#include <QThread>
#include <memory>
#include <chrono>
#include <iostream>

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

    QPushButton* create_snapshot_button = new QPushButton("Создать снапшот", this);
    tool_bar->addWidget(create_snapshot_button);

    QThread* worker_thread = new QThread(this);
    worker_ = new BoardWorker(grpc_client_, board_id_);

    worker_->moveToThread(worker_thread);

    connect(create_widget_button, &QPushButton::clicked, this, &BoardScreen::createWidget);
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

        case (ActionType::DELETE): {
            
            std::lock_guard<std::mutex> lock(widget_edit_mutex_);
        
            if (!board_widgets_.contains(widget_id)) {
                return;
            }
            widget_ptr = board_widgets_[widget_id];
            board_widgets_.erase(widget_id);
            scene_->removeItem(widget_ptr);
            delete widget_ptr;
            
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

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);

        if (!board_widgets_.contains(widget_id)) {
            return;
        }

        Widget* widget_ptr_ = board_widgets_[widget_id];
        board_widgets_.erase(widget_id);
        delete widget_ptr_;
    }

    online_desk::board::BoardUpdate request;
    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::DELETE);
    request.set_widget_id(widget_id);

    worker_->sendSessionUpdate(std::move(request));
}
