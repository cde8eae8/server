//
// Created by nikita on 12/16/19.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include "epoll.h"
#include "address.h"
#include "sys/timerfd.h"

struct server;
struct server_socket;

namespace {
    const size_t MAX_CONNECTIONS_QUEUE = 10000;
    file_descriptor socket_create() {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        std::cout << "socket created " << fd << std::endl;
        return file_descriptor{fd};
    }

    int socket_bind(weak_file_descriptor fd, ipv4_address addr) {
        sockaddr_in a;
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = addr.address();
        a.sin_port = addr.port();
        // FIXME sizeof(a)
        int res = ::bind(fd.fd(), reinterpret_cast<sockaddr*>(&a), sizeof(a));
        std::cout << "socket binded " << a.sin_addr.s_addr << " " << a.sin_port << " " << res << std::endl;
        return res;
    }

    int socket_listen(weak_file_descriptor fd) {
        std::cout << "socket start listen" << std::endl;
        int res = listen(fd.fd(), MAX_CONNECTIONS_QUEUE);
        std::cout << res << std::endl;
        return res;
    }

    weak_file_descriptor socket_accept(weak_file_descriptor fd, ipv4_address& addr) {
        sockaddr_in a;
        socklen_t size = 0;
        int r = accept4(fd.fd(), reinterpret_cast<sockaddr *>(&a), &size, SOCK_NONBLOCK | SOCK_CLOEXEC);
        addr = ipv4_address(a.sin_addr, a.sin_port);
        std::cout << "socket accepted " << a.sin_addr.s_addr << " " << a.sin_port << "desc=" << r << std::endl;
        return weak_file_descriptor{r};
    }

    struct socket_fd {
        socket_fd(ipv4_address addr) {
            descriptor_ = socket_create();
            std::cout << "socket: " << descriptor_.fd() << std::endl;
            if (!descriptor_.valid()) {
                throw std::runtime_error(LINE_INFO + "can't create socket");
            }
            int success = socket_bind(descriptor_.weak(), addr);
            if (success < 0) {
                throw std::runtime_error(LINE_INFO + "can't bind socket");
            }
            success = socket_listen(descriptor_.weak());
            if (success < 0) {
                throw std::runtime_error(LINE_INFO + "can't listen");
            }
            std::cout << "socket: " << descriptor_.fd() << std::endl;
        }

        int fd() {
            std::cout << "socket fd: " << descriptor_.fd() << std::endl;
            return descriptor_.fd();
        }

    private:
        file_descriptor descriptor_;
    };

}

// todo
struct timeouts {
    // todo
    struct time_list {

        struct time_list_node_base {
            time_list_node_base *next, *prev;
        };

        time_list() {
            clear();
        }

        void clear() {
            head.next = head.prev = &head;
        }

        static void remove(time_list_node_base& base) {
            std::cout << "remove" << std::endl;
            auto p = base.prev;
            auto n = base.next;
            if (!n || !p) return;
            n->prev = p;
            p->next = n;
            base.next = base.prev = nullptr;
        }

        void push(time_list_node_base& base) {
            std::cout << "push" << std::endl;
            auto* p = head.prev;
            auto* n = &head;
            n->prev = &base;
            p->next = &base;
            base.next = n;
            base.prev = p;
            print();
        }

        void swap(time_list& b) {
            print();
            b.print();
            std::cout << "swap" << std::endl;
            bool empty = head.next == &head;
            bool b_empty = b.head.next == &b.head;
            if (empty && b_empty) return;
            if (empty || b_empty) {
                auto* emp = this;
                auto* not_empty = &b;
                if (b_empty) {
                    std::swap(emp, not_empty);
                }
                not_empty->head.next->prev = &emp->head;
                not_empty->head.prev->next = &emp->head;
                std::swap(emp->head, not_empty->head);
                not_empty->clear();
                print();
                b.print();
                return;
            }

            std::swap(head.prev->next, b.head.prev->next);
            std::swap(head.next->prev, b.head.next->prev);
            std::swap(head, b.head);
            print();
            b.print();
        }

        void print() {
            std::cout << "print" << std::endl;
            time_list::time_list_node_base* it = head.next;
            for ( ; it != &head && it; it = it->next) {
                std::cout << it->prev << "(" << it << ")" << it->next << " ::: ";
            }
            std::cout << std::endl;
        }

        time_list_node_base head;
    };

    void update_timeout(time_list::time_list_node_base &s);
    void remove(time_list::time_list_node_base &s) {
        time_list::remove(s);
    }

    friend server_socket;
    void check();


    time_list cur, prev;
    std::mutex m;
};

struct server_socket : public timeouts::time_list::time_list_node_base {
    using writer = epoll_event_handler::writer;
    using reader = epoll_event_handler::reader;
    using on_read_t = std::function<void(reader)>;
    using on_write_t = std::function<void(writer)>;
    using on_close_t = std::function<void()>;

    void close();

    server_socket(epoll_event_handler *h, server* s);

    void set_on_read(epoll_event_handler::on_read_t callback) {
        if (callback) {
            h_->set_on_read(std::bind(&server_socket::on_read, this, std::placeholders::_1));
        } else {
            h_->set_on_read(nullptr);
        }
        this->on_read_ = callback;
    }

    void set_on_write(epoll_event_handler::on_write_t callback) {
        if (callback) {
            h_->set_on_write(std::bind(&server_socket::on_write, this, std::placeholders::_1));
        } else {
            h_->set_on_write(nullptr);
        }
        this->on_write_ = callback;
    }

    void set_on_close(on_close_t callback) {
        on_close_ = callback;
    }

    void on_close(weak_file_descriptor fd) {
        std::cout << __FUNCTION__ << std::endl;
        on_close_();
        ::close(fd.fd());
    }

    void on_read(reader r) {
        update_timeout();
        on_read_(r);
    }

    void on_write(writer w) {
        update_timeout();
        on_write_(w);
    }

private:

    void update_timeout();

    epoll_event_handler *h_;
    server *s_;
    on_read_t on_read_;
    on_write_t on_write_;
    on_close_t on_close_;
};


struct server {
    using on_new_connection_t = std::function<void(server_socket*)>;

    server(server&&) = delete;
    server(server&) = delete;
    server operator=(server&) = delete;

    server(ipv4_address addr) : epoll_(), s_(addr), h(epoll_.create_handler(s_.fd())) {
        h->set_on_read([this] (epoll_event_handler::reader) {
            this->on_new_connection();
        });
        epoll_.register_descriptor(h);
        weak_file_descriptor t(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
        if (!t.valid()) {
            throw std::runtime_error(LINE_INFO + "can't create timer");
        }
        std::cout << "timer " << t.fd() << std::endl;
        timer = epoll_.create_handler(t);
        timer->set_on_read([this] (epoll_event_handler::reader r) {
            uint64_t value;
            r.read(reinterpret_cast<char*>(&value), 8);

            this->timeouts_checker.check();
        });
        itimerspec spec = {0, 0, 0, 0};
        spec.it_interval.tv_sec = 2;
        spec.it_interval.tv_nsec = 0;
        spec.it_value.tv_sec = 2;
        spec.it_value.tv_nsec = 0;
        timerfd_settime(t.fd(), 0, &spec, NULL);
        epoll_.register_descriptor(timer);
    }

    void run() {
        epoll_.run();
    }

    void set_on_new_connection_handler(on_new_connection_t handler) {
        new_connection_callback = handler;
    }

private:
    void on_new_connection() {
        std::cout << "real new connection" << std::endl;
        ipv4_address to(0, 0);

        weak_file_descriptor fd = socket_accept(s_.fd(), to);
        if (!fd.valid()) {
            throw std::runtime_error(LINE_INFO + "can't accept");
        }
        auto h = epoll_.create_handler(fd.fd());
        epoll_.register_descriptor(h);
        if (new_connection_callback)
            new_connection_callback(&connections.emplace_back(h, this));
    }

    friend server_socket;

    std::list<server_socket> connections;
    on_new_connection_t new_connection_callback;
    epoll epoll_;
    socket_fd s_;
    epoll_event_handler *h, *timer;

    timeouts timeouts_checker;
};

#endif //SERVER_SERVER_H
