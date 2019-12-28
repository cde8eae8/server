//
// Created by nikita on 12/26/19.
//

#ifndef SERVER_EPOLL_H
#define SERVER_EPOLL_H
#include "../debug.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdexcept>
#include <netinet/in.h>
#include <cassert>
#include <iostream>
#include <list>
#include "file_descriptor.h"
#include "address.h"

struct epoll_event_handler;

struct epoll {
    epoll() {
        poll = file_descriptor{epoll_create1(EPOLL_CLOEXEC)};
        if (!poll.valid()) {
            throw std::runtime_error(LINE_INFO + "can't create epoll");
        }
    }

    void run();

    epoll_event_handler* create_handler(weak_file_descriptor fd);

    // todo copy ctor for handler
    void register_descriptor(epoll_event_handler* handler);


private:
    friend epoll_event_handler;
    void modify_descriptor(weak_file_descriptor fd,  epoll_event_handler& handler);

    void remove_descriptor(weak_file_descriptor fd) {
        std::cout << "delete " << fd.fd() << std::endl;
        assert(fd.valid());
        assert(poll.valid());
        epoll_event ev = {0, 0};
        epoll_ctl(poll.fd(), EPOLL_CTL_DEL, fd.fd(), &ev);
    }

    file_descriptor poll;
    std::list<epoll_event_handler> active_connections;
};


struct events {
    events() : events_(0) {}
    events(uint32_t ev) : events_(ev) {}

    bool check_event(uint32_t event) const {
        return events_ & event;
    }

    void set_event(uint32_t event, bool set) {
        events_ &= ~event;
        events_ |= event * set;
    }

    uint32_t get_events() {
        return events_;
    }

private:
    uint32_t events_;
};

struct epoll_event_handler {
    struct reader {
        reader(weak_file_descriptor fd) : fd_(fd) {}
        size_t read(char* data, size_t size) {
            return ::read(fd_.fd(), data, size);
        }
    private:
        weak_file_descriptor fd_;
    };

    struct writer {
        writer(weak_file_descriptor fd) : fd_(fd) {}
        size_t write(char* data, size_t size) {
            return ::write(fd_.fd(), data, size);
        }
    private:
        weak_file_descriptor fd_;
    };
//    epoll_event_handler(epoll_event_handler&) = delete;
//    epoll_event_handler operator=(epoll_event_handler&) = delete;
//    epoll_event_handler (epoll_event_handler&&) = delete;
//    epoll_event_handler operator=(epoll_event_handler&&) = delete;
    using on_read_t = std::function<void(reader)>;
    using on_write_t = std::function<void(writer)>;
    using on_close_t = std::function<void(weak_file_descriptor fd)>;

    // todo ref
    void set_on_read(on_read_t callback) {
        std::cout << "this" << this << " " << &callback << std::endl;
        set_callback(EPOLLIN, on_read, callback);
    }

    // todo ref
    void set_on_write(on_write_t callback) {
        set_callback(EPOLLOUT, on_write, callback);
    }

    // todo ref
    void set_on_close(on_close_t callback) {
        set_callback(EPOLLRDHUP, on_close, callback);
    }

    // todo exceptions
    void handle_event(events ev);

    void close() {
        std::cout << "closing" << std::endl;
        epoll_->remove_descriptor(fd_.fd());
        on_close(fd_.fd());
    }

private:
    friend struct epoll;

    epoll_event_handler(epoll* epoll, weak_file_descriptor fd) : epoll_(epoll), fd_(fd) { }

    // TODO modify epoll
    template <typename T>
    void set_callback(uint32_t event, std::function<T> &dest, std::function<T> &callback) {
        dest = callback;
        if (dest != nullptr) {
            std::cout << "new event " << event;
        }
        expected_events.set_event(event, dest != nullptr);
        epoll_->modify_descriptor(fd_, *this);
    }

    uint32_t get_events() {
        return expected_events.get_events();
    }

    on_read_t on_read;
    on_write_t on_write;
    on_close_t on_close;
    events expected_events;
    epoll* epoll_;
    weak_file_descriptor fd_;
};

#endif //SERVER_EPOLL_H
