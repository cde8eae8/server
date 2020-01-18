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
    struct args {
        worker& w;
    };

    worker(process_t process) noexcept :
            process_(process),
            this_queue(),
            finished_(false) {
        pthread_create(&t, NULL, run, this);
    }

    static void* run(void* w_) {
        static sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);

        if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
            perror("pthread_sigmask");
            exit(1);
        }

        std::cout << "task started" << std::endl;
        worker* w = static_cast<worker*>(w_);
        while (!w->finished()) {
            TaskType task;
            w->pop_task(task);
            if (w->finished()) break;
            w->process_(task);
        }
        return nullptr;
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
        finished_ = true;
        this_queue.finish();
        pthread_kill(t, SIGUSR1);
        std::cout << "join" << std::endl;
        pthread_join(t, NULL);
    }

protected:
    void pop_task(TaskType& s) {
        this_queue.pop(s);
    }


private:
    process_t process_;
//    std::thread t;
    pthread_t t;
    TaskQueue<TaskType> this_queue;
    std::atomic<bool> finished_;
};



#endif //SERVER_WORKER_H
