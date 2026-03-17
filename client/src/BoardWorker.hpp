#pragma once 

#include "board.grpc.pb.h"
#include "board.pb.h"
#include "BoardWidgets.hpp"
#include "GrpcBoardClientFwd.hpp"
#include "SessionReactorInterface.hpp"
#include "BoardWorkerInterface.hpp"
#include <QObject>
#include <memory>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <atomic>

using namespace online_desk::board;

class BoardWorker : public BoardWorkerInterface {
    Q_OBJECT
    
public:

    enum class Action {
        WRITE = 1,
        READ = 2
    };

    explicit BoardWorker(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id);
    void sendSessionUpdate(online_desk::board::BoardUpdate update) override;
    void addUpdate(online_desk::board::BoardUpdate update) override; 
    ~BoardWorker() override;

private:

    SessionReactorInterface* stream_;

    std::atomic<bool> is_running_;
    std::mutex income_update_mutex_;
    std::condition_variable income_update_cv_;
    std::queue<std::pair<BoardWorker::Action, BoardUpdate>> income_updates_queue_;

public slots:

    void runWorking() override;
    void Shutdown() override;

signals:

    void printUpdate(online_desk::board::BoardUpdate update) override;

};
