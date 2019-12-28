//
// Created by nikita on 12/26/19.
//

#include "epoll.h"


void epoll_event_handler::handle_event(events ev) {
    if (ev.check_event(EPOLLIN)) {
        on_read(reader(fd_));
    }
    if (ev.check_event(EPOLLOUT)) {
        on_write(writer(fd_));
    }
    if (ev.check_event(EPOLLRDHUP)) {
        on_close(fd_);
        epoll_->remove_descriptor(fd_);
    }
}

void epoll::modify_descriptor(weak_file_descriptor fd, epoll_event_handler &handler) {
    std::cout << "modify " << fd.fd() << std::endl;
    assert(fd.valid());
    assert(poll.valid());
    epoll_event ev = {0, 0};
    ev.events = handler.get_events();
    ev.data.ptr = &handler;
    std::cout << "epoll: modify descriptor " << fd.fd() << " " << ev.events << std::endl;
    epoll_ctl(poll.fd(), EPOLL_CTL_MOD, fd.fd(), &ev);
}

void epoll::run() {
    std::array<epoll_event, 100> events;
    for (;;) {
//            std::cout << "wait beg" << std::endl;
        int r = epoll_wait(poll.fd(), events.data(), events.size(), -1);
            std::cout << "wait end" << std::endl;
        if (r < 0) {
            throw std::runtime_error(LINE_INFO + "epoll_wait()");
        }
        if (r == 0) continue;
        // todo EINTR
        for (auto *it = events.begin(); it != events.begin() + r; ++it) {
            std::cout << "--> events=" << it->events << std::endl;
            auto *handler = static_cast<epoll_event_handler*>(it->data.ptr);
            handler->handle_event(it->events);
        }
    }
}

epoll_event_handler *epoll::create_handler(weak_file_descriptor fd) {
    active_connections.push_back(epoll_event_handler(this, fd));
    std::cout << fd.fd() << " _-- " << &active_connections.back() << std::endl;
    return &active_connections.back();
}

void epoll::register_descriptor(epoll_event_handler *handler) {
    weak_file_descriptor fd = handler->fd_;
    assert(fd.valid());
    assert(poll.valid());
    epoll_event ev = {0, 0};

    ev.events = handler->get_events();
    ev.data.ptr = handler;
    std::cout << "epoll: new descriptor " << fd.fd() << handler << " " << ev.events << std::endl;
    epoll_ctl(poll.fd(), EPOLL_CTL_ADD, fd.fd(), &ev);
}
