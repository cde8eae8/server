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
struct epoll_handler_builder;

struct epoll {
    epoll();

    void run();

    void clear_all();

    void request_finishing() volatile { finished_ = true; }

    epoll_handler_builder new_handler(weak_file_descriptor fd);

private:
    friend epoll_event_handler;
    friend epoll_handler_builder;
    void modify_descriptor(weak_file_descriptor fd,  epoll_event_handler& handler);

    void remove_descriptor(std::list<epoll_event_handler>::iterator it, weak_file_descriptor fd);

    epoll_event_handler* create_handler(weak_file_descriptor fd);

    // todo copy ctor for handler
    void register_descriptor(epoll_event_handler* handler);


    file_descriptor poll;
    std::list<epoll_event_handler> active_connections;
    bool finished_;
};


struct events {
    events() : events_(0) {}
    events(uint32_t ev) : events_(ev) {}

    bool check_event(uint32_t event) const {
        return events_ & event;
    }

    void set_event_state(uint32_t event, bool set) {
        events_ &= ~event;
        events_ |= event * set;
    }

    uint32_t get_events() const {
        return events_;
    }

private:
    uint32_t events_;
};

struct epoll_event_handler {
    struct reader {
        explicit reader(weak_file_descriptor fd) : fd_(fd) {}
        ssize_t read(char* data, size_t size) {
            return ::read(fd_.fd(), data, size);
        }
    private:
        weak_file_descriptor fd_;
    };

    struct writer {
        explicit writer(weak_file_descriptor fd) : fd_(fd) {}
        ssize_t write(char* data, size_t size) {
            return ::write(fd_.fd(), data, size);
        }
    private:
        weak_file_descriptor fd_;
    };

    using on_read_t = std::function<void(reader)>;
    using on_write_t = std::function<void(writer)>;
    using on_close_t = std::function<void(weak_file_descriptor fd)>;

//    epoll_event_handler(epoll_event_handler&) = delete;
//    epoll_event_handler operator=(epoll_event_handler&) = delete;
//    epoll_event_handler (epoll_event_handler&&) = delete;
//    epoll_event_handler operator=(epoll_event_handler&&) = delete;

    bool is_open() { return fd_.valid(); }

    // todo ref
    void set_on_read(on_read_t callback) {
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

    // FIXME: check if its correct
    void close() {
        if (on_close)
            on_close(fd_.fd());
        epoll_->remove_descriptor(it_, fd_.fd());
    }

private:
    friend struct epoll;

    void set_iterator(std::list<epoll_event_handler>::iterator it) {
        std::cout << "iterator for " << this->fd_.fd() << std::endl;
        it_ = it;
    }

    // todo exceptions
    void handle_event(events ev);

    epoll_event_handler(epoll* epoll, weak_file_descriptor fd) : epoll_(epoll), fd_(fd),
                                                        on_read(nullptr),
                                                        on_write(nullptr),
                                                        on_close(nullptr) { }

    // TODO modify epoll
    template <typename T>
    void set_callback(uint32_t event, std::function<T> &dest, std::function<T> &callback) {
        dest = callback;
        expected_events.set_event_state(event, dest != nullptr);
        if (epoll_)
            epoll_->modify_descriptor(fd_, *this);
    }

    uint32_t get_events() { return expected_events.get_events(); }

    on_read_t on_read;
    on_write_t on_write;
    on_close_t on_close;
    events expected_events;
    epoll* epoll_;
    weak_file_descriptor fd_;
    std::list<epoll_event_handler>::iterator it_;
};

struct epoll_handler_builder {
    epoll_handler_builder(const epoll_handler_builder&) = delete;
    epoll_handler_builder operator=(const epoll_handler_builder&) = delete;

    epoll_handler_builder& set_on_read(epoll_event_handler::on_read_t f) {
        _handler->set_on_read(f);
        return *this;
    }
    epoll_handler_builder& set_on_write(epoll_event_handler::on_write_t f) {
        _handler->set_on_write(f);
        return *this;
    }
    epoll_handler_builder& set_on_close(epoll_event_handler::on_close_t f) {
        _handler->set_on_close(f);
        return *this;
    }

    // You can't use this handler before calling register_handler
    epoll_event_handler* ref() {
        return _handler;
    }

    epoll_event_handler* register_handler() {
        _epoll->register_descriptor(_handler);
        _registered = true;
        return _handler;
    }

    ~epoll_handler_builder() {
        assert(_registered);
    }

private:
    friend epoll;
    epoll_handler_builder(epoll* e, weak_file_descriptor fd) :
            _epoll(e),
            _handler(_epoll->create_handler(fd)),
            _registered(false) { }

    epoll* _epoll;
    epoll_event_handler* _handler;
    bool _registered;
};

#endif //SERVER_EPOLL_H
