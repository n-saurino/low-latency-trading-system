#pragma once
#include <sstream>
#include "../../common/types.h"

using namespace Common;

namespace Exchange{
#pragma pack(push,1)
enum class MarketUpdateType: uint8_t{
    INVALID = 0,
    CLEAR = 1, // let's clients know that they should clear/empty their orderbook
    ADD = 2,
    MODIFY = 3,
    CANCEL = 4,
    TRADE = 5,
    SNAPSHOT_START = 6, // notify clients that a snapshot update is starting
    SNAPSHOT_END = 7 // notify clients that all snapshot updates have been delivered
    
};

inline std::string MarketUpdateTypeToString(MarketUpdateType market_update_type){
    switch (market_update_type){
    case MarketUpdateType::ADD:
        return "ADD";

    case MarketUpdateType::MODIFY:
        return "MODIFY";

    case MarketUpdateType::CANCEL:
        return "CANCEL";

    case MarketUpdateType::TRADE:
        return "TRADE";
    case MarketUpdateType::INVALID:
        return "INVALID";
    }

    return "UNKNOWN";
}

struct MEMarketUpdate{
    MarketUpdateType type_ = MarketUpdateType::INVALID;
    OrderId order_id_ = OrderId_INVALID;
    TickerId ticker_id_ = TickerId_INVALID;
    Side side_ = Side::INVALID;
    Price price_ = Price_INVALID;
    Qty qty_ = Qty_INVALID;
    Priority priority_ = Priority_INVALID;

    auto ToString() const{
        std::stringstream ss;
        ss << "MEMarketUpdate" << " ["
        << "type: " << MarketUpdateTypeToString(type_)
        << " ticker: " << TickerIdToString(ticker_id_)
        << " market_order_id: " << OrderIdToString(order_id_)
        << " side: " << SideToString(side_)
        << " qty: " << QtyToString(qty_)
        << " price: " << PriceToString(price_)
        << " priority: " << PriorityToString(priority_)
        << "]";
        return ss.str();
    }
};

struct MDPMarketUpdate{
    size_t seq_num_ = 0;
    MEMarketUpdate me_market_update_;
    auto ToString() const{
        std::stringstream ss;
        ss << "MDPMarketUpdate"
            << " ["
            << " seq:" << seq_num_
            << " " << me_market_update_.ToString()
            << "]";
        return ss.str();
    }
};
#pragma pack(pop)

typedef Common::LFQueue<Exchange::MEMarketUpdate> MEMarketUpdateLFQueue;
typedef Common::LFQueue<Exchange::MDPMarketUpdate> MDPMarketUpdateLFQueue;
}