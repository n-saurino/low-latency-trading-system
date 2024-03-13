#pragma once
#include "common/thread_utils.h"
#include "common/lf_queue.h"
#include "common/macros.h"
#include "order_server/client_request.h"
#include "order_server/client_response.h"
#include "market_data/market_update.h"
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
    
    // deleted default, copy & move constructors and assignment-operators
    MatchingEngine() = delete;
    MatchingEngine(const MatchingEngine &) = delete;
    MatchingEngine(const MatchingEngine &&) = delete;
};
}