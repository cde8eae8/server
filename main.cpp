#include <iostream>
#include "server_epoll/server.h"
#include "packet_connection/connection.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>
#include "worker.h"
#include "getaddrinfo.h"

volatile server* s_ptr = nullptr;

void sig_handler(int sig) {
    std::cout << "new signal" << std::endl;
    if (s_ptr)
        s_ptr->finish();
}

int main() {
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    server s(ipv4_address("127.0.0.1", PORT));
    s_ptr = &s;
    using worker = worker<std::string>;
    std::list<connection> conns;
    std::list<worker> workers;

    s.set_on_new_connection_handler([&conns, &workers] (server_socket* socket) {
        connection &c = conns.emplace_front(socket);

        auto worker_process = std::bind(&getaddrinfo_worker,
                                        std::placeholders::_1,
                                        [&c](std::string s) {
                                            c.write(s.data(), s.size());
                                        });

        auto &w = workers.emplace_front(worker_process);
        auto conn_it = conns.begin();
        auto w_it = workers.begin();

        c.set_on_new_message([&w] (std::string v) {
            w.add_task(v);
        });

        c.set_on_disconnect([&conns, &workers, conn_it, w_it] () {
            workers.erase(w_it);
            conns.erase(conn_it);
        });
    });
    s.run();
    assert(conns.empty());
    assert(workers.empty());
    return 0;
}