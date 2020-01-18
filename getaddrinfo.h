//
// Created by nikita on 12/28/19.
//

#ifndef SERVER_GETADDRINFO_WORKER_H
#define SERVER_GETADDRINFO_WORKER_H
#include <list>
#include <vector>
#include <string>
#include <netdb.h>
#include <arpa/inet.h>
#include <functional>

std::string to_string(uint32_t addr) {
    return std::to_string((addr << UINT32_C(24)) >> UINT32_C(24)) + '.' +
           std::to_string((addr << UINT32_C(16)) >> UINT32_C(24)) + '.' +
           std::to_string((addr << UINT32_C(8)) >> UINT32_C(24)) + '.' +
           std::to_string((addr << UINT32_C(0)) >> UINT32_C(24));
}

void getaddrinfo_worker(std::string const& task_, std::function<void(std::string)> new_result) {
    struct addrinfo hints, *res = nullptr;
    int err;
    uint32_t tmp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    err = getaddrinfo(task_.c_str(), nullptr, &hints, &res);

    std::string result(task_);
    result += ':';
    for (auto p = res; err == 0 && p != nullptr; p = p->ai_next) {
        std::memcpy(reinterpret_cast<char *>(&tmp), &p->ai_addr->sa_data[2], 4);
        sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(p->ai_addr);
        char s[15];
        inet_ntop(AF_INET, addr, s, 15);
        result += to_string(tmp) + ' ';
    }
    freeaddrinfo(res);
    result += '\n';
    new_result(std::move(result));
    return;
};

#endif //SERVER_GETADDRINFO_WORKER_H
