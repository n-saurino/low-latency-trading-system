#pragma once

#include "../../common/types.h"
#include "../../common/mem_pool.h"
#include "../../common/logging.h"
#include "../order_server/client_response.h"
#include "../market_data/market_update.h"
#include "matching_engine.h"
#include "me_order.h"

using namespace Common;

namespace Exchange{
class MatchingEngine;

class MEOrderBook final{
public:
    // Add()
    void Add(ClientId client_id, OrderId order_id, 
            TickerId ticker_id, Side side, 
            Price price, Qty qty){

            }

    // Cancel()
    void Cancel(ClientId client_id, OrderId order_id, TickerId ticker_id_){

    }

    // MEOrderBook constructor
    MEOrderBook(size_t, Logger*, Exchange::MatchingEngine*){

    }
};

typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}