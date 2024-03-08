#include "time_utils.h"
#include "logging.h"
#include "tcp_server.h"

int main(int, char**){
    using namespace Common;

    std::string time_str_;
    Logger logger_("socket_example.log");

    auto TCPServerRecvCallback = [&](TCPSocket* socket, Nanos rx_time) noexcept{
        logger_.Log("TCPServer::DefaultRecvCallback() socket:% len:% rx:%\n", 
        socket->fd_, socket->next_rcv_valid_index_, rx_time);
        const std::string reply = "TCPServer received msg:"
        + std::string(socket->rcv_buffer_, socket->next_rcv_valid_index_);
        socket->next_send_valid_index_ = 0;
        socket->Send(reply.data(), reply.length());
    };

    auto TCPServerRecvFinishedCallback = [&]() noexcept{
        logger_.Log("TCPServer::DefaultRecvFinishedCallback()\n");
    };

    auto TCPClientRecvCallback = [&](TCPSocket* socket, Nanos rx_time) noexcept{
        const std::string recv_msg = std::string(socket->rcv_buffer_, socket->next_rcv_valid_index_);
        socket->next_rcv_valid_index_ = 0;

        logger_.Log("TCPSocket::DefaultRecvCallback() socket:% len:% rx:% msg:%\n",
        socket->fd_, socket->next_rcv_valid_index_, rx_time, recv_msg);
    };

    const std::string iface = "lo";
    const std::string ip = "127.0.0.1";
    const int port = 12345;

    logger_.Log("Creating TCPServer on iface: % port:% \n", iface, port);
    TCPServer server(logger_);
    server.recv_callback_ = TCPServerRecvCallback;
    server.recv_finished_callback_ = TCPServerRecvFinishedCallback;
    server.Listen(iface, port);

    std::vector<TCPSocket*> clients(5);

    for(size_t i = 0; i < clients.size(); ++i){
        clients[i] = new TCPSocket(logger_);
        clients[i]->recv_callback_ = TCPClientRecvCallback;

        logger_.Log("Connecting TCPClient-[%] on ip:% iface:% port:% \n", i, ip, iface, port);
        clients[i]->Connect(ip, iface, port, false);
        server.Poll();
    }

    using namespace std::literals::chrono_literals;

    for(auto itr = 0; itr < 5; ++itr){
        for(size_t i = 0; i < clients.size(); ++i){
            const std::string client_msg = "CLIENT-[" + std::to_string(i) + "] : Sending "
            + std::to_string(itr * 100 + i);
            logger_.Log("Sending TCPClient-[%] %]\n", i, client_msg);
            clients[i]->Send(client_msg.data(), client_msg.length());
            clients[i]->SendAndRecv();
        
            std::this_thread::sleep_for(500ms);
            server.Poll();
            server.SendAndRecv();
        }
    }

    for(auto itr = 0; itr < 5; ++itr){
        for(auto& client: clients){
            client->SendAndRecv();
        }
        server.Poll();
        server.SendAndRecv();
        std::this_thread::sleep_for(500ms);
    }

    return 0;
}