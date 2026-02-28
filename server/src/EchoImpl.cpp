#include "EchoImpl.h"

namespace Echo {

grpc::Status EchoServer::Echo (
    grpc::ServerContext* context,
    const desk::EchoRequest* request,
    desk::EchoResponse* response
) {

    response->set_success(true);
    response->set_text(request->text());

    return grpc::Status::OK;
}

}