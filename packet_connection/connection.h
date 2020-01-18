//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <thread>
#include <queue>
#include "../server_epoll/server.h"
#include "../taskqueue.h"

struct connection {
    using on_new_message_t = std::function<void (std::string)>;
    using on_disconnect_t = std::function<void ()>;

    connection(server_socket *h) : handler_(h) {
        std::cout << "connection created" << std::endl;
        h->set_on_read(std::bind(&connection::on_read, this, std::placeholders::_1));
        h->set_on_write(std::bind(&connection::on_write, this, std::placeholders::_1));
        h->set_on_close(std::bind(&connection::on_close, this));
    }

    server_socket* handler() {
        return handler_;
    }

    void write(char* data, size_t size) {
        std::cout << "new data to send " << std::endl;
        auto v = std::vector(data, data + size);
        to_write.push(v);
        std::lock_guard<std::mutex> l(write_lock);
        handler_->set_on_write(std::bind(&connection::on_write, this, std::placeholders::_1));
    }

    void set_on_new_message(on_new_message_t callback) {
        on_new_message = callback;
    }

    void set_on_disconnect(on_disconnect_t callback) {
        on_disconnect = callback;
    }

    void close() {
        to_write.finish();
        handler_->close();
    }

private:
    void on_close() {
        std::cout << "closing..." << std::endl;
        if (on_disconnect)
            on_disconnect();
    }

    void on_read(server_socket::reader reader) {
        std::array<char, 1024> data;
        size_t size = reader.read(data.data(), data.size());
        auto end = data.begin() + size;
        auto beg = data.begin();
        for (auto it = beg; it != end; ) {
            it = std::find(beg, end, '\n');
            if (it != end) {
                msg.insert(msg.end(), beg, it - 1);
                for (char c : msg) {
                    std::cout << static_cast<int>(c) << std::endl;
                }
                std::cout << "message " << msg << ") " << std::endl;

                on_new_message(std::move(msg));
                msg.clear();
                beg = it + 1;
            }
        }
        msg.insert(msg.end(), beg, end);
    }

    void on_write(server_socket::writer w) {
        std::lock_guard<std::mutex> l(write_lock);
        std::vector<char> msg;
        while(to_write.nonblock_pop(msg)) {
            w.write(msg.data(), msg.size());
        }
        handler_->set_on_write(nullptr);
    }

    std::mutex write_lock;

    std::string msg;
    server_socket* handler_;
    on_new_message_t on_new_message;
    on_disconnect_t on_disconnect;
    TaskQueue<std::vector<char>> to_write;
};

#endif //SERVER_CONNECTION_H
