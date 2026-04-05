#pragma once

#include "board.grpc.pb.h"
#include "board.pb.h"
#include "GrpcClient/SessionReactorInterface.hpp"
#include "GrpcClient/GrpcBoardClient.hpp"
#include "BoardWidgets.hpp"
#include "BoardWorker.hpp"
#include <QMainWindow>
#include <QMetaType>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <memory>
#include <queue>
#include <random>

using namespace online_desk::board;

class BoardScreen : public QMainWindow {
    Q_OBJECT

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    uint64_t board_id_;

    std::mt19937_64 gen64_; 
    std::mutex widget_edit_mutex_;
    std::unordered_map<uint64_t, Widget*> board_widgets_;

    BoardWorker* worker_;
    QGraphicsScene* scene_;
    QGraphicsView* scene_view_;

    void SetupUI();
    Widget* ProduceWidget(uint64_t widget_id);

public slots:

    void createWidget();
    void createSnapshot();
    void requestUpdate(WidgetUpdate request);
    void requestDelete(uint64_t widget_id);

    void acceptBoardUpdate(BoardUpdate update);

signals:

    void sendSessionUpdate(online_desk::board::BoardUpdate update);

public:

    explicit BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id, QWidget* parent = nullptr);
    ~BoardScreen();
};

Q_DECLARE_METATYPE(online_desk::board::BoardUpdate)
