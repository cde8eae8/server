#ifndef TASKQUEUE_H
#define TASKQUEUE_H
#include <atomic>
#include <iostream>
#include <mutex>
#include <condition_variable>

template <typename T>
class TaskQueue {
public:
    TaskQueue() : head(nullptr), tail(nullptr), finished(false),
        n_sleepers(0), mutex(), cond()  {}

    ~TaskQueue() {
        node* cur = head;
        while (cur) {
            head = head.load()->next;
            delete cur;
            cur = head;
        }
    }

    bool nonblock_pop(T& out) {
        if (finished) return false;
        mutex.lock();
        if (empty() || finished) {
            mutex.unlock();
            return false;
        }
        node *n = head;
        head = head.load()->next;
        mutex.unlock();

        out = std::move(n->value);
        delete n;
        return true;
    }

    void pop(T& out) {
        if (finished) return;
        std::unique_lock l(mutex);
        // FIXME atomic is correct?
        while (empty() && !finished) {
            cond.wait(l);
        }
        if (finished) {
            l.unlock();
            return;
        }
        node *n = head;
        head = head.load()->next;
        l.unlock();

        out = std::move(n->value);
        delete n;
    }

    void push(T &value) {
        node* new_node = new node(std::move(value));

        mutex.lock();
        if (empty()) {
            head = new_node;
        } else {
            tail->next = new_node;
        }
        tail = new_node;
        cond.notify_one();
        mutex.unlock();
    }

    void reset() {
        head = tail = nullptr;
        finished = false;
        n_sleepers = 0;
    }

    void finish() {
        mutex.lock();
        // std::cout << "QUEUE finished" << std::endl;
        finished = true;
        cond.notify_all();
        mutex.unlock();
    }

private:
    struct node {
        node(T v) :
            next(nullptr),
            value(std::move(v)) {}

        node *next;
        T value;
    };

    bool empty() {
        return !head;
    }

    std::atomic<node*> head;
    node *tail;
    std::atomic<bool> finished;
    std::atomic<size_t> n_sleepers;

    std::mutex mutex;
    std::condition_variable cond;
};

#endif // TASKQUEUE_H
