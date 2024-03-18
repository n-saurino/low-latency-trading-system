#include "matching_engine.h"

namespace Exchange{
    MatchingEngine::MatchingEngine(ClientRequestLFQueue* client_requests, 
                                    ClientResponseLFQueue* client_responses, 
                                    MEMarketUpdateLFQueue* market_updates):
                                    incoming_requests_(client_requests), 
                                    outgoing_ogw_responses_(client_responses),
                                    outgoing_md_responses_(market_updates),
                                    logger_("exchange_matching_engine.log"){
        for(size_t i = 0; i < ticker_order_book_.size(); ++i){
            ticker_order_book_[i] = new MEOrderBook(i, &logger_, this);
        }
    }

MatchingEngine::~MatchingEngine(){
    run_ = false;
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
    incoming_requests_ = nullptr;
    outgoing_ogw_responses = nullptr;
    outgoing_md_updates = nullptr;
    for(auto& order_book : ticker_order_book_){
        delete order_book;
        order_book = nullptr;
    }
}

// creates and launches a new thread, assigning it the MatchingEngine::Run() method
MatchingEngine::Start() -> void{
    run_ = true;
    ASSERT(COMMON::CreateAndStartThread(-1, "Exchange/MatchingEngine", [this]() {Run();}) != nullptr,
            "Failed to start MatchingEngine thread.");
}

MatchingEngine::Stop() -> void{
    run_ = false;
}

auto Run() noexcept{
    logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_));
    while(run_){
        const auto me_client_request = incoming_requests_->GetNextToRead();
        if(LIKELY(me_client_request)){
            logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, Common::GetCurrentTimeStr(&time_str_), me_client_request->ToString());
            ProcessClientRequest(me_client_request);
            incoming_requests_->UpdateReadIndex();
        }
    }
}
}