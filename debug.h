//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_DEBUG_H
#define SERVER_DEBUG_H
#include <string>
#include <cstring>

#define LINE_INFO (std::string(strerror(errno)) + "|" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + ":")

#endif //SERVER_DEBUG_H
