//
// Created by nikita on 12/26/19.
//

#include "epoll.h"


void epoll_event_handler::handle_event(events ev) {
    if (ev.check_event(EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        close();
        return;
    }
    if (ev.check_event(EPOLLIN)) {
        on_read(reader(fd_));
    }
    if (ev.check_event(EPOLLOUT)) {
        on_write(writer(fd_));
    }
}

//template<typename T>
//void epoll_event_handler::set_callback(uint32_t event, std::function<T> &dest, std::function<T> &callback) {
//    dest = callback;
//    expected_events.set_event_state(event, dest != nullptr);
//    if (epoll_)
//        epoll_->modify_descriptor(fd_, *this);
//}

void epoll::modify_descriptor(weak_file_descriptor fd, epoll_event_handler &handler) {
//    std::cout << "modify " << fd.fd() << std::endl;
    assert(fd.valid());
    assert(poll.valid());
    epoll_event ev = {0, 0};
    ev.events = handler.get_events();
    ev.data.ptr = &handler;
//    std::cout << "epoll: modify descriptor " << fd.fd() << " " << ev.events << std::endl;
    epoll_ctl(poll.fd(), EPOLL_CTL_MOD, fd.fd(), &ev);
}


void epoll::run() {
    std::array<epoll_event, 100> events;
    for (;;) {
        if (finished_) { break; }
        int r = epoll_wait(poll.fd(), events.data(), events.size(), -1);
        if (r < 0) {
            if (errno != EINTR) {
                throw std::runtime_error(LINE_INFO + "epoll_wait() ");
            } else {
                break;
            }
        }
        if (finished_) { break; }
        if (r == 0) continue;
        for (auto *it = events.begin(); it != events.begin() + r; ++it) {
            std::cout << "epoll catched events: " << it->events << std::endl;
            auto *handler = static_cast<epoll_event_handler*>(it->data.ptr);
            handler->handle_event(it->events);
        }
    }
    std::cout << "finishing " << std::endl;
    clear_all();
}

epoll_event_handler *epoll::create_handler(weak_file_descriptor fd) {
    active_connections.push_front(epoll_event_handler(this, fd));
    active_connections.front().set_iterator(active_connections.begin());
    std::cout << "for " << fd.fd() << " created handler " << &active_connections.front() << std::endl;
    return &active_connections.front();
}

void epoll::register_descriptor(epoll_event_handler *handler) {
    weak_file_descriptor fd = handler->fd_;
    assert(fd.valid());
    assert(poll.valid());
    epoll_event ev = {0, 0};

    ev.events = handler->get_events();
    ev.data.ptr = handler;
    epoll_ctl(poll.fd(), EPOLL_CTL_ADD, fd.fd(), &ev);

}

void epoll::clear_all() {
    for (auto it = active_connections.begin(); it != active_connections.end(); ) {
        auto next = it++;
        if (next->is_open())
            next->close();
    }
}

epoll::epoll() {
    poll = file_descriptor{epoll_create1(EPOLL_CLOEXEC)};
    if (!poll.valid()) {
        throw std::runtime_error(LINE_INFO + "can't create epoll");
    }
}

void epoll::remove_descriptor(std::list<epoll_event_handler>::iterator it, weak_file_descriptor fd) {
    std::cout << "delete " << fd.fd() << std::endl;
    assert(fd.valid());
    assert(poll.valid());
    epoll_event ev = {0, 0};
    epoll_ctl(poll.fd(), EPOLL_CTL_DEL, fd.fd(), &ev);
    active_connections.erase(it);
}

epoll_handler_builder epoll::new_handler(weak_file_descriptor fd) {
    return epoll_handler_builder(this, fd);
}
