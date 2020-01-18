//
// Created by nikita on 12/28/19.
//
#include "server.h"

void timeouts::update_timeout(time_list::time_list_node_base &s) {
    std::lock_guard<std::mutex> l(m);
    time_list::remove(s);
    assert(&s != &cur.head);
    assert(&s != &prev.head);
    cur.push(s);
}

void timeouts::check() {
    std::lock_guard<std::mutex> l(m);
    time_list::time_list_node_base* it = (prev.head.next);
    for ( ; it != &prev.head; ) {
        server_socket* s = static_cast<server_socket*>(it);
        assert(it != &prev.head);
        assert(it != &cur.head);
        it = it->next;
        s->close();
    }
    prev.clear();
    cur.swap(prev);
}

void server_socket::update_timeout() {
    if (closed) return;
    s_->timeouts_checker.update_timeout(*this);
}

void server_socket::close() {
    if (closed) return;
    closed = true;
    std::cout << "timeout closing" << std::endl;
//    s_->timeouts_checker.remove(*this);
    this->h_->close();
}

server_socket::server_socket(epoll_handler_builder &h, server *s) : time_list_node_base(), h_(h.ref()), s_(s) {
    h.set_on_close(std::bind(&server_socket::on_close, this, std::placeholders::_1));
    s_->timeouts_checker.update_timeout(*this);
}

void server_socket::on_close(weak_file_descriptor fd) {
//    if (closed) return;
    on_close_();
    ::close(fd.fd());
    std::cout << fd.fd() << " closed" << std::endl;
    s_->remove(it_);
    s_->timeouts_checker.remove(*this);
}

weak_file_descriptor create_timerfd(size_t period) {
    weak_file_descriptor t(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
    if (!t.valid()) {
        throw std::runtime_error(LINE_INFO + "can't create timer");
    }
    itimerspec spec = {0, 0, 0, 0};
    spec.it_interval.tv_sec = 5;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = 5;
    spec.it_value.tv_nsec = 0;
    timerfd_settime(t.fd(), 0, &spec, NULL);
    return t;
}

