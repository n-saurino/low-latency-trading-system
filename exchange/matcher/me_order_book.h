#pragma once

#include "../../common/types.h"
#include "../../common/mem_pool.h"
#include "../../common/logging.h"
#include "../order_server/client_response.h"
#include "../market_data/market_update.h"
#include "me_order.h"

using namespace Common;

namespace Exchange{
class MatchingEngine;
class MEOrderBook final{
    // Add()

    // Cancel()

    // MEOrderBook constructor

    // Need a type for MatchingEngine::Start()
};

typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}