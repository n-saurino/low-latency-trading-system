#pragma once
#include <functional>
#include "common/lf_queue.h"
#include "common/thread_utils.h"
#include "common/macros.h"
#include "common/tcp_server.h"
#include "order_server/client_request.h"
#include "order_server/client_response.h"
#include "order_server/fifo_sequencer.h"

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
    
    auto RecvCallback(TCPSocket* socket, Nanos rx_time) noexcept{
        logger_.Log("%:% %() % Received socket:% len:% rx:%\n", __FILE__,
        __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), 
        socket->fd_, socket->next_rcv_valid_index_, rx_time);

        if(socket->next_rcv_valid_index_ >= sizeof(OMClientRequest)){
            size_t i = 0;
            for(; i + sizeof(OMClientRequest) <= socket->next_rcv_valid_index_;
                i += sizeof(OMClientRequest)){
                auto request = reinterpret_cast<const OMClientRequest*>(
                               socket->rcv_buffer_+i);
                
                logger_.Log("%:% %() % Received %\n", __FILE__, __LINE__,
                            __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
                            request->ToString());
                
                if(UNLIKELY(cid_tcp_socket_[request->
                            me_client_request_.client_id_] == nullptr)){
                            cid_tcp_socket_[request->me_client_request_.client_id_]
                            = socket;
                            }
                
                if(cid_tcp_socket_[request
                   ->me_client_request_.client_id_] != socket){
                    logger_.Log("%:% %() % Received ClientRequest from \
                                ClientId: % on different socket: % \
                                expected: % \n", __FILE__, __LINE__,
                                __FUNCTION__, 
                                Common::GetCurrentTimeStr(&time_str_), 
                                request->me_client_request_.client_id_,
                                socket->fd_,
                                cid_tcp_socket_[request->me_client_request_
                                                .client_id_]->fd_);
                    continue;
                }
                
                auto next_exp_seq_num = cid_next_exp_seq_num_[request->
                                        me_client_request_.client_id_];
                if(request->seq_num_ != next_exp_seq_num){
                    logger_.Log("%:% %() % Incorrect sequence number. \
                                ClientId: % SeqNum expected: % received: % \
                                \n", __FILE__, __LINE__, __FUNCTION__,
                                Common::GetCurrentTimeStr(&time_str_),
                                request->me_client_request_.client_id_,
                                next_exp_seq_num,
                                request->seq_num_);
                    
                    continue;
                }
                
                // add client request to the FIFO sequencer
                ++next_exp_seq_num;
                fifo_sequencer_.AddClientRequest(rx_time, 
                                                 request->me_client_request_);                
            }
            
            memcpy(socket->rcv_buffer_, socket->rcv_buffer_ + i,
                   socket->next_rcv_valid_index_ - i); 
                   socket->next_rcv_valid_index_ -= i;
        }
    }
    
    auto RecvFinishedCallback() noexcept{
        fifo_sequencer_.SequenceAndPublish();
    }
};
}