#pragma once
#include "../../common/thread_utils.h"
#include "../../common/lf_queue.h"
#include "../../common/macros.h"
#include "../../order_server/client_request.h"
#include "../../order_server/client_response.h"
#include "../market_data/market_update.h"
#include "me_order.h"

namespace Exchange{
class MatchingEngine final{
private:
    OrderBookHashMap ticker_order_book_;
    ClientRequestLFQueue* incoming_requests_ = nullptr;
    ClientResponseLFQueue* outgoing_ogw_responses_ = nullptr;
    ClientResponseLFQueue* outgoing_md_responses_ = nullptr;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;

public:
    MatchingEngine(ClientRequestLFQueue* client_requests, ClientResponseLFQueue* client_responses, MEMarketUpdateLFQueue* market_updates);
    ~MatchingEngine();
    auto Start() -> void;
    auto Stop() -> void;
    
    // checks for the type of the MEClientRequest and forwards it
    // to the limit order book of the corresponding instrument
    auto ProcessClientRequest(const MEClientRequest* client_request) noexcept{
        // find the security's corresponding order book
        auto order_book = ticker_order_book_[client_request -> ticker_id_];
        switch(client_request->type_){
            case ClientRequestType::NEW:
                {
                    order_book -> Add(client_request->client_id_, client_request->order_id_, 
                                    client_request->ticker_id_, client_request->side_, 
                                    client_request->price_, client_request->qty_);
                }
                break;

            case ClientRequestType::CANCEL:
                {
                 order_book->Cancel(client_request->client_id_, client_request->order_id_
                                    client_request->ticker_id_);
                }
                break;

            default:
                {
                    FATAL("Received invalid client-request-type:" + ClientRequestTypeToString(client_request->type_));
                }
                break;
        }
    }

    // writes client response to outgoing_ogw_responses_ lf queue
    // and then advances the writer index
    auto SendClientResponse(const MEClientResponse* client_response) noexcept{
        logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
                    client_response->ToString());
        auto next_write = outgoing_ogw_responses_->GetNextToWriteTo();
        *next_write = std::move(*client_response);
        outgoing_ogw_responses_->UpdateWriteIndex();
    }

    auto SendMarketUpdate(const MEMarketUpdate* market_update) noexcept{
        logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_),
                    market_update->ToString());
        auto next_write = outgoing_md_updates_->GetNextToWriteTo();
        *next_write = std::move(*market_update);
        outgoing_md_updates_->UpdateWriteIndex();
    }
    
    // deleted default, copy & move constructors and assignment-operators
    MatchingEngine() = delete;
    MatchingEngine(const MatchingEngine &) = delete;
    MatchingEngine(const MatchingEngine &&) = delete;
};
}