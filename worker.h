//
// Created by nikita on 12/28/19.
//

#ifndef SERVER_WORKER_H
#define SERVER_WORKER_H

#include <thread>
#include <functional>
#include "taskqueue.h"

template <typename TaskType>
class worker {
public:
    using process_t = std::function<void(TaskType)>;

    worker(process_t process) noexcept :
            process_(process),
            this_queue(),
            finished_(false),
            t(std::bind(&worker::run, this)) {}

    void run() {
        while (!finished()) {
            TaskType task;
            pop_task(task);

            if (finished()) break;

            process_(task);
        }
    }

    // may be called multiple times
    void finish() {
        finished_ = true;
        this_queue.finish();
    }

    template<typename It>
    void add_tasks(It begin, It end) {
        for (auto i = begin; i != end; ++i) {
            this_queue.push(*i);
        }
    }

    void add_task(TaskType& s) {
        this_queue.push(s);
    }

    bool finished() {
        return finished_;
    }

    ~worker() {
        finish();
        t.join();
    }

protected:
    void pop_task(TaskType& s) {
        this_queue.pop(s);
    }


private:
    process_t process_;
    std::thread t;
    TaskQueue<TaskType> this_queue;
    std::atomic<bool> finished_;
};



#endif //SERVER_WORKER_H
