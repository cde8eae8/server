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
    s_->timeouts_checker.update_timeout(*this);
}

void server_socket::close() {
    std::cout << "timeout closing" << std::endl;
//    assert(&*this != &cur.head);
//    assert(&*this != &prev.head);
    s_->timeouts_checker.remove(*this);
    this->h_->close();
}

server_socket::server_socket(epoll_event_handler *h, server *s) : time_list_node_base(), h_(h), s_(s) {
    h_->set_on_close(std::bind(&server_socket::on_close, this, std::placeholders::_1));
    s_->timeouts_checker.update_timeout(*this);
}
