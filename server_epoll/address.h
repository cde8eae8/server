//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_ADDRESS_H
#define SERVER_ADDRESS_H
#include <array>
#include "network.h"


struct ipv4_address {
    ipv4_address(std::array<char, 4> addr) {
        // TODO
        throw std::runtime_error("not implemented yet");
    }

    ipv4_address(uint32_t addr, uint16_t port) : addr_(to_network_byteorder(addr)),
                                                port_(to_network_byteorder(port)) { }

    ipv4_address(in_addr addr, in_port_t port) : addr_((addr.s_addr)),
                                                 port_((port)) { }

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
