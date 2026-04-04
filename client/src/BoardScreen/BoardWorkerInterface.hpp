#pragma once
#include <QObject>

#include "board.grpc.pb.h"
#include "board.pb.h"

class BoardWorkerInterface : public QObject {

    Q_OBJECT
    
public slots:

    virtual void runWorking() = 0;
    virtual void Shutdown() = 0;


public:

    virtual void printUpdate(online_desk::board::BoardUpdate update) = 0;
    virtual void sendSessionUpdate(online_desk::board::BoardUpdate update) = 0;
    virtual void addUpdate(online_desk::board::BoardUpdate update) = 0;

    virtual ~BoardWorkerInterface() = default;

};
