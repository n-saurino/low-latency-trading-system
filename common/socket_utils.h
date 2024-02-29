#pragma once
#include <iostream>
#include <string>
#include <unordered_set>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "macros.h"
#include "logging.h"

namespace Common{
    constexpr int MAXTCPServerBacklog = 1024;
    
    auto GetIfaceIP(const std::string &iface) -> std::string{
        char buf[NI_MAXHOST]  = {'\0'};
        ifaddrs *ifaddr = nullptr;
        if(getifaddrs(&ifaddr) != -1){
            for(ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next){
                if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name){
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        return buf;
    }

    auto SetNonBlocking(int fd) -> bool{
        const auto flags = fcntl(fd, F_GETFL, 0);
        if(flags == -1){
            return false;
        }

        if(flags & O_NONBLOCK){
            return true;
        }

        return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }
    
    auto SetNoDelay(int fd) -> bool;
    auto SetSOTimestamp(int fd) -> bool;
    auto WouldBlock() -> bool;
    auto SetMcastTTL(int fd, int ttl) -> bool;
    auto SetTTL(int fd, int ttl) -> bool;
    auto Join(int fd, const std::string &ip, const std::string &iface, int port) -> bool;
    auto CreateSocket(Logger &logger, const std::string& t_ip, const std::string& iface, int port, bool is_udp, bool is_blocking, bool is_listening, int ttl, bool needs_so_timestamp) -> int;
}