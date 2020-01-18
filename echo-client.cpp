//
// Created by nikita on 1/15/20.
//

#include <iostream>
#include "server_epoll/server.h"
#include "packet_connection/connection.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>
#include "worker.h"
#include "getaddrinfo.h"

int main() {
    struct sockaddr_in sa;

    inet_pton(AF_INET, "127.0.0.1", &(sa.sin_addr));
    server s(ipv4_address(sa.sin_addr, htons(PORT)));
    using elements = std::pair<connection, worker<std::vector<char>>>;
    using worker = worker<std::vector<char>>;
    std::list<connection> conns;
    s.set_on_new_connection_handler([&conns] (server_socket* socket) {
        conns.emplace_front(socket);
        connection &c = conns.front();
        c.write("hello\n", 6);

        c.set_on_new_message([&c] (std::vector<char> v) {
            std::cout << "new message " << std::string(v.begin(), v.end()) << std::endl;
            c.write(v.data(), v.size());
        });

//        c.set_on_disconnect([&conns, conn_it] () {
//            conns.erase(conn_it);
//        });
    });
    s.run();
    return 0;
}
