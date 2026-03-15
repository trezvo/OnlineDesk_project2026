#pragma once

#include <grpcpp/grpcpp.h>
#include "board.grpc.pb.h"
#include "board.pb.h"

using namespace online_desk::board;

class SessionReactorInterface : public grpc::experimental::ClientBidiReactor<BoardUpdate, BoardUpdate> {

public:
    virtual void AddUpdate(BoardUpdate request) = 0;
    virtual ~SessionReactorInterface() = default;
};
