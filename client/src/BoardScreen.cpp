#include "GrpcBoardClient.hpp"
#include "BoardScreen.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"
#include <QPushButton>
#include <memory>
#include <chrono>

BoardScreen::BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id, QWidget* parent)
    : QMainWindow(parent)
    , grpc_client_(grpc_client)
    , board_id_(board_id)
    , gen64_(std::chrono::system_clock::now().time_since_epoch().count()) {
    stream_ = grpc_client_->connectToBoard(*this, board_id_);
    SetupUI();
}

void BoardScreen::SetupUI() {

    setWindowTitle("Окно сессии");
    resize(800, 600);
    
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QPushButton* create_widget_button_ = new QPushButton("Создать виджет", this);

    centralWidget->setStyleSheet("background-color: #ed9898;");

    connect(create_widget_button_, &QPushButton::clicked, this, &BoardScreen::create_widget);
}

Widget* BoardScreen::ProduceWidget(uint64_t widget_id) {
    Widget* new_widget = new Widget(widget_id, this);
    connect(new_widget, &Widget::requestUpdate, this, &BoardScreen::requestUpdate);
    connect(new_widget, &Widget::requestDelete, this, &BoardScreen::requestDelete);
    new_widget->show();
    return new_widget;
}

void BoardScreen::create_widget() {
    uint64_t widget_id = gen64_();

    Widget* new_widget = ProduceWidget(widget_id);

    std::lock_guard<std::mutex> lock(widget_edit_mutex_);
    board_widgets_[widget_id] = new_widget;
}

void BoardScreen::UpdateBoard(BoardUpdate upd) {

    using namespace online_desk::board;

    ActionType action = upd.action_type();
    uint64_t widget_id = upd.widget_id();

    std::cout << "online income widget_id=" << widget_id << std::endl;
    
    const WidgetInfo& info = upd.update_data();
    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);
        
        switch (action) {
            case (ActionType::CREATE): {
                if (board_widgets_.contains(widget_id)) {
                    return;
                }                
                Widget* new_widget = ProduceWidget(widget_id);
                new_widget->UpdateCoords(info.coord_x(), info.coord_y());
                board_widgets_[widget_id] = new_widget;
            } break;

            case (ActionType::DELETE): {
                if (!board_widgets_.contains(widget_id)) {
                    return;
                }
                Widget* widget_ptr_ = board_widgets_[widget_id];
                board_widgets_.erase(widget_id);
                widget_ptr_->setParent(nullptr);
                delete widget_ptr_;
            } break;

            case (ActionType::UPDATE): {
                if (!board_widgets_.contains(widget_id)) {
                    board_widgets_[widget_id] = ProduceWidget(widget_id);
                }
                
                Widget* new_widget = board_widgets_[widget_id];
                new_widget->UpdateCoords(info.coord_x(), info.coord_y());                
            } break;
        }
    }
}

void BoardScreen::requestUpdate(uint64_t widget_id, WidgetUpdate upd) {

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);

        if (!board_widgets_.contains(widget_id)) {
            return;
        }

        board_widgets_[widget_id]->UpdateCoords(upd.new_x, upd.new_y);
    }

    online_desk::board::BoardUpdate request;

    online_desk::board::WidgetInfo* difference = request.mutable_update_data();
    difference->set_coord_x(upd.new_x);
    difference->set_coord_y(upd.new_y);
    difference->set_content("");

    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::UPDATE);
    request.set_widget_id(widget_id);

    stream_->AddUpdate(std::move(request));
}

void BoardScreen::requestDelete(uint64_t widget_id) {

    {
        std::lock_guard<std::mutex> lock(widget_edit_mutex_);

        if (!board_widgets_.contains(widget_id)) {
            return;
        }

        Widget* widget_ptr_ = board_widgets_[widget_id];
        board_widgets_.erase(widget_id);
        widget_ptr_->setParent(nullptr);
        delete widget_ptr_;
    }

    online_desk::board::BoardUpdate request;
    request.set_user_token(grpc_client_->GetUserToken());
    request.set_action_type(online_desk::board::ActionType::DELETE);
    stream_->AddUpdate(std::move(request));
}
