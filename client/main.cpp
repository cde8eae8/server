//
// Created by nikita on 12/27/19.
//
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

int main() {
    struct sockaddr_in sa;
    memset(&sa, '0', sizeof(sa));

    sa.sin_family = AF_INET;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << inet_pton(AF_INET, "127.0.0.1", &(sa.sin_addr)) << std::endl;
    sa.sin_port = htons(50000);
    std::cout << s << std::endl;
    std::cout << connect(s, (sockaddr*)&sa, sizeof(sa)) << std::endl;
    std::cout << strerror(errno) << std::endl;
    usleep(10000);
    return 0;
}
