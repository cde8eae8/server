//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_FILE_DESCRIPTOR_H
#define SERVER_FILE_DESCRIPTOR_H


#include <algorithm>
#include <unistd.h>

struct weak_file_descriptor {
    weak_file_descriptor(int fd) : fd_(fd) {}
    int fd() {
        return fd_;
    }

    bool valid() {
        return fd_ >= 0;
    }

private:
    int fd_;
};

struct file_descriptor {
    file_descriptor() : descriptor_(-1) { }
    explicit file_descriptor(int descriptor) : descriptor_(descriptor) { }
    file_descriptor(file_descriptor const&) = delete;

    file_descriptor& operator=(file_descriptor&& f) {
        std::swap(f.descriptor_, descriptor_);
        return *this;
    }

    file_descriptor(file_descriptor&& f) {
        std::swap(f.descriptor_, descriptor_);
    }

    int release() {
        int r = descriptor_;
        descriptor_ = -1;
        return r;
    }

    bool valid() {
        return descriptor_ >= 0;
    }

    weak_file_descriptor weak() {
        return descriptor_;
    }

    int fd() {
        return descriptor_;
    }

    ~file_descriptor() {
        if (descriptor_ == -1) return;
        ::close(descriptor_);
    }

private:
    int descriptor_;
};


#endif //SERVER_FILE_DESCRIPTOR_H
