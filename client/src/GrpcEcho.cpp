#include "GrpcEcho.h"

namespace Echo {

constexpr const char* SERVER_ADDRESS = "localhost:50051";

EchoClient::EchoClient() {
    channel_ = grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials());
    echo_stub_ = desk::Greater::NewStub(channel_);
}
EchoClient::~EchoClient() {
    echo_stub_.reset();
    channel_.reset();
}

EchoClient::LocalResponse EchoClient::send_message(const std::string& text) {

    desk::EchoRequest request;
    request.set_text(text);

    desk::EchoResponse response;
    grpc::ClientContext context;

    grpc::Status status = echo_stub_->Echo(&context, request, &response);

    if (!status.ok()) {
        return {false, "bad"};
    }

    return {response.success(), response.text()};
}

}