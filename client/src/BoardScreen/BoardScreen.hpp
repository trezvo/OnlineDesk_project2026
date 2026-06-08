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
#include <QGraphicsItem>
#include <QComboBox>
#include <QPushButton>
#include <memory>
#include <queue>
#include <random>
#include <unordered_map>
#include <mutex>
#include <vector>

using namespace online_desk::board;

class BoardScreen : public QMainWindow {
    Q_OBJECT

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    uint64_t board_id_;

    std::mt19937_64 gen64_; 
    std::mutex widget_edit_mutex_;
    std::unordered_map<uint64_t, Widget*> board_widgets_;
    double current_zoom_{1.0};

    BoardWorker* worker_;
    QGraphicsScene* scene_;
    QGraphicsView* scene_view_;
    QGraphicsPathItem* drawing_preview_{nullptr};
    QComboBox* widget_type_selector_;
    QPushButton* create_widget_button_{nullptr};
    WidgetType current_widget_type_{WidgetType::STICKER};

    bool placing_arrow_{false};
    QPointF arrow_start_;

    bool is_drawing_{false};
    std::vector<QPointF> current_drawing_points_;
    uint64_t current_drawing_id_{0};
    int drawing_points_since_last_sync_{0};

    bool is_erasing_{false};

    std::atomic<bool> worker_shutdown_{false};
    std::atomic<bool> is_closing_{false};

    void SetupUI();
    Widget* ProduceWidget(uint64_t widget_id, WidgetType type = WidgetType::STICKER);
    void applyZoom(double factor);

private slots:
    void onBackToMenuClicked();
    void onBoardDeleted();
    void updateToolMode();
    void createArrow(QPointF p1, QPointF p2);
    void createDrawing(const std::vector<QPointF>& points);
    
protected:
    void closeEvent(QCloseEvent* event) override;    

public slots:
    void createWidget();
    void createSnapshot();
    void deleteSelectedWidgets();
    void exportBoardToPng();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void requestUpdate(WidgetUpdate request);
    void requestDelete(uint64_t widget_id); 
    void acceptBoardUpdate(BoardUpdate update);

signals:
    void sendSessionUpdate(online_desk::board::BoardUpdate update);
    void boardClosed();
    void boardDeletedByOwner(uint64_t board_id);

public:
    explicit BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id, QWidget* parent = nullptr);
    ~BoardScreen();
    uint64_t getBoardId() const { return board_id_; }
    void shutdownWorker();
    bool eventFilter(QObject* watched, QEvent* event) override;
};

Q_DECLARE_METATYPE(online_desk::board::BoardUpdate)