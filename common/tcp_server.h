#pragma once
#include "tcp_socket.h"

 namespace Common{
struct TCPServer{
public:
    int efd_ = -1;
    TCPSocket listener_socket_;
    epoll_event events_[1024];
    std::vector<TCPSocket*> sockets_, receive_sockets_, send_sockets_, disconnected_sockets_;
    std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_;
    std::function<void()> recv_finished_callback_;
    std::string time_str_;
    Logger &logger_;

    explicit TCPServer(Logger& logger): listener_socket_(logger), logger_(logger){
        recv_callback_ = [this](auto socket, auto rx_time){
            DefaultRecvCallback(socket, rx_time);
            recv_finished_callback_ = [this](){
                DefaultRecvFinishedCallback();
            };
        }
    }

    auto DefaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept{
        logger_.Log("%:% %() % TCPServer::DefaultRecvCallback() socket:% len:% rx:%\n",
        __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str),
        socket->fd_, socket->next_rcv_valid_index, rx_time);
    }

    auto DefaultRecvFinishedCallback() noexcept{
        logger_.Log("%:% %() % TCPServer::DefaultRecvFinishedCallback()\n", __FILE__,
        __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_));
    }

    auto TCPServer::Destroy(){
        close(efd_);
        efd_ = -1;
        listener_socket_.destroy();
    }

    auto TCPServer::Listen(const std::string &iface, int port) -> void{
        destroy();
        efd_ = epoll_create(1);
        ASSERT(efd_ >= 0, "epoll_create() failed error: " + std::string(std::strerror(errno)));

        ASSERT(listener_socket_.Connect("", iface, port, true) >= 0, "Listener socket failed to connect. iface:" + iface + " port:" + std::to_string(port) + " error: "
        + std::string(std::strerror(errno)));

        ASSERT(epoll_add(&listener_socket_), "epoll_ctl() failed. error: " + std::string(std::strerror(errno)));
    }

    auto TCPServer::epoll_add(TCPSocket* socket){
        epoll_event ev{};
        ev.events = EPOLLET | EPOLLIN;
        ev.data.ptr = reinterpret_cast<void*>(socket);
        return (epoll_ctl(efd_,EPOLL_CTL_ADD, socket->fd_, &ev) != -1);
    }

    auto TCPServer::epoll_del(TCPSocket* socket){
        return (epoll_ctl(efd_, EPOLL_CTL_DEL, socket->fd_, nullptr) != -1);
    }

    auto TCPServer::Del(TCPSocket* socket){
        epoll_del(socket);
        sockets_.erase(std::remove(sockets_.begin(), sockets_.end(), socket), sockets_.end());
        receive_sockets.erase(std::remove(receive_sockets_.begin(), receive_sockets_.end(), socket), receive_sockets_.end());
        send_sockets_.erase(std::remove(send_sockets_.begin(), send_sockets_.end(), socket), send_sockets_.end());
    }

    auto TCPServer::Poll() noexcept -> void{
        const int max_events = 1 + sockets_.size();

        for(auto socket: disconnected_sockets_){
            Del(socket);
        }

        const int n = epoll_wait(efd_, events_, max_events, 0);

        bool have_new_connection = false;
        for(int i = 0; i < n; ++i){
            epoll_event &event = events_[i];
            auto socket = reinterpret_cast<TCPSocket*>(event.data.ptr);

            if(event.events & EPOLLIN){
                if(socket == &listener_socket_){
                    logger_.Log("%:% %() % EPOLLIN listener_socket:%\n", 
                    __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
                    socket->fd_);
                    have_new_connection = true;
                    continue;
                }
                logger_.Log("%:% %() % EPOLLIN socket:%\n",
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), socket->fd_);
                if(std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end()){
                    receive_sockets_.push_back(socket);
                }
            }

            if(event.events & EPOLLOUT){
                logger_.log("%:% %() % EPOLLOUT socket:%\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), socket->fd_);
                if(std::find(send_sockets_.begin(), send_sockets_.end(), socket) == send_sockets_.end()){
                    send_sockets_.push_back(socket);
                }
            }

            if(event.events & (EPOLLERR | EPOLLHUP)){
                logger_.log("%:% %() % EPOLLERR socket:%\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), socket->fd_);
                if(std::find(disconnected_sockets_.begin(), disconnected_sockets_.end(), socket) == disconnected_sockets_.end()){
                    disconnected_sockets_.push_back(socket);
                }
            }

            while(have_new_connection){
                logger_.Log("%:% %() % have_new_connection\n",
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_));
                sockaddr_storage addr;
                socklen_t addr_len = sizeof(addr);
                int fd = accept(listener_socket_.fd_, reinterpret_cast<sockaddr*>(&addr), &addr_len);
                if(fd == -1){
                    break;
                }

                ASSERT(SetNonBlocking(fd) && SetNoDelay(fd), 
                "Failed to set non-blocking or no-delay on socket:"
                + std::to_string(fd));

                logger_.Log("%:% %() % accepted socket:%\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), fd);

                TCPSocket* socket = new TCPSocket(logger_);
                socket->fd_ = fd;
                socket->recv_callback_ = recv_callback_;
                ASSERT(epoll_add(socket), "Unable to add socket. error: " + std::string(std::strerror(errno)));

                if(std::find(sockets_.begin(), sockets_.end(), socket) == sockets_.end()){
                    sockets_.push_back(socket);
                }

                if(std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end()){
                    receive_sockets_.push_back(socket);
                }
            }
        }   
    }

    auto TCPServer::SendAndRecv() noexcept -> void{
        auto recv = false;

        for(auto socket: receive_sockets_){
            if(socket->SendAndRecv()){
                recv = true;
            }
        }

        if(recv){
            recv_finished_callback_();
        }

        for(auto socket: send_sockets_){
            socket->SendAndRecv();
        }
    }

    TCPServer() = delete;
    TCPServer(const TCPServer&) = delete;
    TCPServer(const TCPServer&&) = delete;
    TCPServer& operator = (const TCPServer&) = delete;
    TCPServer& operator = (const TCPServer&&) = delete;
};
}