#pragma once
#include <functional>
#include "socket_utils.h"
#include "logging.h"

namespace Common{
    constexpr size_t TCPBufferSize = 64*1024*1024;

    struct TCPSocket{
        int fd_ = -1;
        char* send_buffer_ = nullptr;
        size_t next_send_valid_index_ = 0;
        char* rcv_buffer_ = nullptr;
        size_t next_rcv_valid_index_ = 0;

        bool send_disconnected_ = false;
        bool recv_disconnected_ = false;

        struct sockaddr_in in_in_addr;

        std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback_;

        std::string time_str_;
        Logger& logger_;

        // log information confirming that the callback was invoked
        auto DefaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept{
            logger_.Log("%:% %() %TCPSocket::DefaultRecvCallback() socket:% len:% rx:%\n",
            __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
            socket->fd_, socket->next_rcv_valid_index_, rx_time);
        }

        explicit TCPSocket(Logger& logger): logger_(logger){
            send_buffer_ = new char[TCPBufferSize];
            rcv_buffer_ = new char[TCPBufferSize];
            recv_callback_ = [this](auto socket, auto rx_time){
                DefaultRecvCallback(socket, rx_time);
            };
        }

        auto Destroy() noexcept -> void{
            close(fd_);
            fd_ = -1;
        }

        ~TCPSocket(){
            Destroy();
            delete[] send_buffer_;
            send_buffer_ = nullptr;
            delete[] rcv_buffer_;
            rcv_buffer_ = nullptr;
        }

        // Delete default, copy or move constructors and assignment operators
        TCPSocket() = delete;
        TCPSocket(const TCPSocket&) = delete;
        TCPSocket(const TCPSocket&&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&&) = delete;

        auto Connect(const std::string& ip, const std::string &iface, int port, bool is_listening) -> int{
            Destroy();
            fd_ = CreateSocket(logger_, ip, iface, port, false, false, is_listening, 0, true);

            in_in_addr.sin_addr.s_addr = INADDR_ANY;
            in_in_addr.sin_port = htons(port);
            in_in_addr.sin_family = AF_INET;

            return fd_;
        }

        auto Send(const void* data, size_t len) noexcept -> void{
            if(len > 0){
                memcpy(send_buffer_ + next_send_valid_index_, data, len);
                next_send_valid_index_ += len;
            }
        }

        auto SendAndRecv() noexcept -> bool{
            char ctrl[CMSG_SPACE(sizeof(struct timeval))];
            struct cmsghdr* cmsg = (struct cmsghdr*) &ctrl;
            
            struct iovec iov;
            iov.iov_base = rcv_buffer_ + next_rcv_valid_index_;
            iov.iov_len = TCPBufferSize - next_rcv_valid_index_;

            msghdr msg;
            msg.msg_control = ctrl;
            msg.msg_controllen = sizeof(ctrl);
            msg.msg_name = &in_in_addr;
            msg.msg_namelen = sizeof(in_in_addr);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;

            const auto n_rcv = recvmsg(fd_, &msg, MSG_DONTWAIT);
            if(n_rcv > 0){
                next_rcv_valid_index_ += n_rcv;

                Nanos kernel_time = 0;
                struct timeval time_kernel;
                if(cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SCM_TIMESTAMP &&
                cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel))){

                    memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
                    kernel_time = time_kernel.tv_sec * NANOS_TO_SECS + time_kernel.tv_usec * NANOS_TO_MICROS;
                }

                const auto user_time = GetCurrentNanos();

                logger_.Log("%:% %() % read socket:% len:% utime:% ktime:% diff:%\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
                fd_, next_rcv_valid_index_, user_time, kernel_time, (user_time - kernel_time));

                recv_callback_(this, kernel_time);
            }

            ssize_t n_send = std::min(TCPBufferSize, next_send_valid_index_);
            
            while(n_send > 0){
                auto n_send_this_msg = std::min(static_cast<ssize_t>(next_send_valid_index_), n_send);
                const int flags = MSG_DONTWAIT | MSG_NOSIGNAL | (n_send_this_msg < n_send ? MSG_MORE : 0);
                // REVIEW
                auto n = ::send(fd_, send_buffer_, n_send_this_msg, flags);
                if(UNLIKELY(n < 0)){
                    if(!WouldBlock()){
                        send_disconnected_ = true;
                    }
                    break;
                }

                logger_.Log("%:% %() % send socket:% len:%\n",
                __FILE__, __LINE__, __FUNCTION__,
                Common::GetCurrentTimeStr(&time_str_), fd_, n);

                n_send -= n;
                ASSERT(n==n_send_this_msg, "Don't support partial send lengths yet.");
            }
            
            next_send_valid_index_ = 0;
            
            return (n_rcv > 0);
        }        
    };


}
