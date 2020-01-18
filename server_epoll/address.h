//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_ADDRESS_H
#define SERVER_ADDRESS_H
#include <array>
#include <arpa/inet.h>
#include "network.h"


struct ipv4_address {
    ipv4_address(uint32_t addr, uint16_t port) : addr_(htonl(addr)),
                                                port_(htons(port)) { }

    ipv4_address(in_addr addr, in_port_t port) : addr_((addr.s_addr)),
                                                 port_((port)) { }

    ipv4_address(std::string ip, uint16_t port) : port_(htons(port)) {
        struct sockaddr_in sa;
        inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
        addr_ = sa.sin_addr.s_addr;
    }

    uint32_t address() {
        return addr_;
    }

    uint16_t port() {
        return port_;
    }

private:
    uint32_t addr_;
    uint16_t port_;
};


#endif //SERVER_ADDRESS_H
