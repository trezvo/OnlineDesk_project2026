#pragma once

#include <grpcpp/grpcpp.h>
#include "contracts.grpc.pb.h"
#include "contracts.pb.h"

namespace Echo {

class EchoServer final : public desk::Greater::Service {

public:

    EchoServer() = default;
    ~EchoServer() override = default;

    grpc::Status Echo(
        grpc::ServerContext* context,
        const desk::EchoRequest* request,
        desk::EchoResponse* response
    ) override;

};

}