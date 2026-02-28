#include "GrpcEcho.h"
#include <memory>
#include <iostream>


int main(int argc, char* argv[]) {

    auto me = std::make_shared<Echo::EchoClient>();

    std::string s;

    while (std::cin >> s) {
        if (s == "-1") {
            break;
        }

        auto response = me->send_message(s);

        if (response.sucsess_flag) {
            std::cout << response.text << std::endl;
        }
    }

    return 1;
}