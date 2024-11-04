#include "order_server/order_server.h"

namespace Exchange{
// Constructor accepts pointers to two lock-free queue objects:
// one to forward MEClientRequests to the matching engine
// and one to receive MEClientResponses from teh matching engine
// it also accepts a network interface and port to use that the order gateway
// will listen to and accept client connections on.
OrderServer::OrderServer(ClientRequestLFQueue* client_requests, 
                ClientResponseLFQueue* client_responses, 
                const std::string& iface, int port): iface_(iface), port_(port),
                outgoing_responses_(client_responses),
                logger_("exchange_order_server.log"), tcp_server_(logger_),
                fifo_sequencer_(client_requests, &logger_){
    cid_next_exp_seq_num_.fill(1);
    cid_next_outgoing_seq_num_.fill(1);
    cid_tcp_socket_.fill(nullptr);
    // Need to implement RecvCallback()
    tcp_server_.recv_callback_ = [this](auto socket, auto rx_time){
        RecvCallback(socket, rx_time);
    };
    // Need to implement RecvFinishedCallback()
    tcp_server_.recv_finished_callback_ = [this](){
        RecvFinishedCallback();
    };
}

OrderServer::~OrderServer()
{
    Stop();
    // sleeps so that thread can finish any pending tasks
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
}

// sets bool run_ to true (flag that controls how long main thread runs)
// initializes TCPServer member to listen on interface and port from the 
// OrderServer constructor.
// creates and launches a thread that will execute Run() method
auto OrderServer::Start() -> void{
    run_ = true;
    tcp_server_.Listen(iface_, port_);
    ASSERT(Common::CreateAndStartThread(-1, "Exchnage/OrderServer", 
           [this](){Run();}) != nullptr, "Failed to start OrderServer thread.");
}

// sets bool run_ to false which ends Run() method execution
auto OrderServer::Stop() -> void{
    run_ = false;
}
}