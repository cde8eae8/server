cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wconversion -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wunused")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(server
        main.cpp
        server_epoll/file_descriptor.cpp
        server_epoll/file_descriptor.h
        server_epoll/epoll.cpp
        server_epoll/epoll.h
        server_epoll/server.h
        server_epoll/server.cpp
        server_epoll/address.h
        server_epoll/network.h
        packet_connection/connection.cpp
        packet_connection/connection.h
        getaddrinfo.h
        worker.h
        )

add_executable(client client/main.cpp)
