#pragma once
#include "tcp_socket.h"

// log information confirming that the callback was invoked
auto DefaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept{
    logger_.Log("%:% %() %TCPSocket::DefaultRecvCallback() socket:% len:% rx:%\n",
    __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
    socket->fd_, socket->next_rcv_valid_index_, rx_time);
}