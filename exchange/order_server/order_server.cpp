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
}

OrderServer::~OrderServer()
{
}
}