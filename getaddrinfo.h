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

void getaddrinfo_worker(std::vector<char> task_, std::function<void(std::string)> new_result) {
    usleep(1000);
    std::cout << "...answer..." << std::endl;
    return;
    std::string task(task_.data(), task_.size());
    addrinfo *result;
    std::string out;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int r = getaddrinfo(NULL, task.data(), &hints, &result);
    if (r != 0) {
        std::cout << strerror(errno) << std::endl;
    }

    for (auto rp = result; rp != NULL; rp = rp->ai_next) {
        sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(rp->ai_addr);
        char s[15];
        inet_ntop(AF_INET, addr, s, 15);
        out += s + '\n';
    }
    new_result(std::move(out));
};

#endif //SERVER_GETADDRINFO_WORKER_H
