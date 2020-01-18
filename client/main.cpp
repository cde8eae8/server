//
// Created by nikita on 12/27/19.
//
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <array>
#include <thread>
#include <vector>
#include "../debug.h"

void task() {
    struct sockaddr_in sa;
    memset(&sa, '0', sizeof(sa));

    sa.sin_family = AF_INET;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << inet_pton(AF_INET, "127.0.0.1", &(sa.sin_addr)) << std::endl;
    sa.sin_port = htons(PORT);
    std::cout << s << std::endl;
    std::cout << connect(s, (sockaddr*)&sa, sizeof(sa)) << std::endl;
    std::cout << strerror(errno) << std::endl;
    std::string str = "www.google.com=www.google.com=www.google.com=";
    write(s, str.c_str(), str.size());
    std::array<char, 1024> buf;
    ssize_t r;

    while ((r = read(s, buf.data(), buf.size())) > 0) {
        std::cout << std::string(buf.data(), r) << std::endl;
    }
}

int main() {
    std::vector<std::thread> t;
    for (size_t i = 0; i < 1000; ++i) {
        t.emplace_back(task);
    }

    for (auto& v : t) {
        v.join();
    }
    return 0;
}
