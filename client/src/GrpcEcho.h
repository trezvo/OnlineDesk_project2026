#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "contracts.grpc.pb.h"
#include "contracts.pb.h"

namespace Echo {

class EchoClient {

    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<desk::Greater::Stub> echo_stub_;

public:

    struct LocalResponse {
        bool sucsess_flag;
        std::string text;
    };

    EchoClient();
    ~EchoClient();


    LocalResponse send_message(const std::string& text);

};

}