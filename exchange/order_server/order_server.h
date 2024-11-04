#pragma once
#include <functional>
#include "common/lf_queue.h"
#include "common/thread_utils.h"
#include "common/macros.h"
#include "common/tcp_server.h"
#include "order_server/client_request.h"
#include "order_server/client_response.h"

// UNCOMMENT once we build the FIFO sequencer
// #include "order_server/fifo_sequencer.h"

namespace Exchange{
class OrderServer
{
private:
    const std::string iface_;
    const int port_{};
    volatile bool run_{false};
    std::string time_str_;
    Logger logger_;
    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_;
    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_;
    std::array<Common::TCPSocket*, ME_MAX_NUM_CLIENTS> cid_tcp_socket_;
    Common::TCPServer tcp_server_;
    ClientResponseLFQueue* outgoing_responses_;
    // Need to add FIFOSequencer class
    FIFOSequencer fifo_sequencer_;


public:
    OrderServer(ClientRequestLFQueue* client_requests, 
                ClientResponseLFQueue* client_responses, 
                const std::string& iface, int port);
    ~OrderServer();
    auto Start() -> void;
    auto Stop() -> void;
};
}