#include <iostream>
#include "server_epoll/server.h"
#include "packet_connection/connection.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "worker.h"
#include "getaddrinfo.h"

int main() {
struct sockaddr_in sa;

inet_pton(AF_INET, "127.0.0.1", &(sa.sin_addr));
server s(ipv4_address(sa.sin_addr, htons(50006)));
using elements = std::pair<connection, worker<std::vector<char>>>;
std::list<connection> conns;
std::list<worker<std::vector<char>>> workers;
s.set_on_new_connection_handler([&conns, &workers] (server_socket* socket) {
        conns.emplace_back(socket);
        connection &c = conns.back();

        workers.emplace_back(std::bind(&getaddrinfo_worker,
                std::placeholders::_1,
                [&c](std::string s) {
                    c.write(s.data(), s.size());
                }));
        auto &w = workers.back();
        c.set_on_new_message([] (std::vector<char> data) {
            std::cout << std::string(data.begin(), data.end()) << std::endl;
        });

        c.set_on_new_message([&w] (std::vector<char> v) {
            std::cout << "new message" << std::endl;
            std::cout << std::string(v.begin(), v.end()) << std::endl;
            w.add_task(v);
        });
    });
    s.run();
    return 0;
}