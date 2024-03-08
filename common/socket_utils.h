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
    
    auto SetNoDelay(int fd) -> bool{
        int one = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void *>(&one), sizeof(one)) != -1);
    }

    auto SetSOTimestamp(int fd) -> bool{
        int one = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, reinterpret_cast<void*>(&one), sizeof(one))!= -1);
    }

    auto WouldBlock() -> bool{
        return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    }

    auto SetMcastTTL(int fd, int mcast_ttl) -> bool{
        return (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<void*>(&mcast_ttl), sizeof(mcast_ttl)) != -1);
    }

    auto SetTTL(int fd, int ttl) -> bool{
        return (setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast<void*>(&ttl), sizeof(ttl)) != -1);
    }

    auto Join(int fd, const std::string &ip, const std::string &iface, int port) -> bool{
        const ip_mreq mreq{{inet_addr(ip.c_str())}, {htonl(INADDR_ANY)}};
        return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1);
    }

    auto CreateSocket(Logger &logger, const std::string& t_ip, const std::string& iface, int port, bool is_udp, bool is_blocking, bool is_listening, int ttl, bool needs_so_timestamp) -> int{
        std::string time_str;
        const auto ip = t_ip.empty() ? GetIfaceIP(iface) : t_ip;
        logger.Log("%:% %() % ip:% iface:% port:% is_udp:% is_blocking:% is_listening:% ttl:% SO_time:%\n", __FILE__, __LINE__, __FUNCTION__,
        Common::GetCurrentTimeStr(&time_str), ip, iface, port, is_udp, is_blocking, is_listening, ttl, needs_so_timestamp);

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_protocol = is_udp ? IPPROTO_UDP : IPPROTO_TCP;
        hints.ai_flags = is_listening ? AI_PASSIVE : 0;
        if(std::isdigit(ip.c_str()[0])){
            hints.ai_flags |= AI_NUMERICHOST;
        }    
        hints.ai_flags |= AI_NUMERICSERV;

        addrinfo *result = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
        if(rc){
            logger.Log("getaddrinfo() failed. error:% errno:%\n", gai_strerror(rc), strerror(errno));
            return -1;
        }

        // create the socket
        int fd = -1;
        int one = 1;
        for(addrinfo* rp = result; rp; rp = rp->ai_next){
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if(fd == -1){
                logger.Log("socket() failed. errno:%\n", strerror(errno));
                return -1;
            }
            
            // set socket to be non-blocking and disable Nagle's algorithm
            if(!is_blocking){
                if(!SetNonBlocking(fd)){
                    logger.Log("SetNonBlocking() failed. errno:%\n", strerror(errno));
                    return -1;
                }
                if(!is_udp && !SetNoDelay(fd)){
                    logger.Log("SetNoDelay() failed. errno:%\n", strerror(errno));
                    return -1;
                }
            }

            // connect socket to the target address if it is not a listening socket
            if(!is_listening && connect(fd, rp->ai_addr, rp->ai_addrlen) == 1 && !WouldBlock()){
                logger.Log("Connect() failed. errno:%\n", strerror(errno));
                return -1;
            }
            
            // make the socket a listening socket if is_listening = true
            if(is_listening && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof(one)) == -1){
                logger.Log("setsockopt() SO_REUSEADDR failed. errno:%\n", strerror(errno));
                return -1;
            }
            
            if(is_listening && bind(fd, rp->ai_addr, rp->ai_addrlen) == -1){
                logger.Log("bind() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if(!is_udp && is_listening && listen(fd, MAXTCPServerBacklog) != -1){
                logger.Log("listen() failed. errno:%\n", strerror(errno));
                return -1;
            }

            // set TTL value for socket
            if(is_udp && ttl){
                const bool is_multicast = atoi(ip.c_str()) & 0xe0;
                if(is_multicast && !SetMcastTTL(fd, ttl)){
                    logger.Log("SetMcast() failed. errno:%\n", strerror(errno));
                    return -1;
                }

                if(!is_multicast && !SetTTL(fd, ttl)){
                    logger.Log("SetTTL() failed. errno:%\n", strerror(errno));
                    return -1;
                }
            }

            // give access for timestamps of incoming packets
            if(needs_so_timestamp && !SetSOTimestamp(fd)){
                logger.Log("SetSOTimestamp() failed. errno:%\n", strerror(errno));
                return -1;
            }
        }

        if(result){
            freeaddrinfo(result);
        }

        return fd;
    }
}