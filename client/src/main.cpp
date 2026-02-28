#include "auth.grpc.pb.h"
#include "auth.pb.h"
#include <grpcpp/grpcpp.h>

#include <string>
#include <iostream>
#include <memory>

int main() {

    std::shared_ptr<grpc::Channel> channel_ = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    std::unique_ptr<online_desk::auth::AuthenticationService::Stub> auth_stub_ = 
        online_desk::auth::AuthenticationService::NewStub(channel_);

    while (true) {
        int x;
        std::cin >> x;
        if (x == 1) {
            std::string n, p;
            std::cout << "Name" << std::endl;
            std::cin >> n;
            std::cout << "Password" << std::endl;
            std::cin >> p;

            online_desk::auth::RegisterRequest request;
            request.set_username(n);
            request.set_password(p);

            online_desk::auth::RegisterResponse response;
            grpc::ClientContext context;

            auth_stub_->UserRegister(&context, request, &response);
            std::cout << response.message();  
        }
        else if (x == 2) {
            std::string n, p;
            std::cout << "Name" << std::endl;
            std::cin >> n;
            std::cout << "Password" << std::endl;
            std::cin >> p;

            online_desk::auth::LoginRequest request;
            request.set_username(n);
            request.set_password(p);

            online_desk::auth::LoginResponse response;
            grpc::ClientContext context;

            auth_stub_->UserLogin(&context, request, &response);
            std::cout << response.message();
        }
    }


    return 0;
}