#pragma once
#include <sstream>
#include "../../common/types.h"

using namespace Common;

namespace Exchange{
#pragma pack(push,1)
enum class MarketUpdateType: uint8_t{
    INVALID = 0,
    ADD = 1,
    MODIFY = 2,
    CANCEL = 3,
    TRADE = 4
};

inline std::string MarketUpdateTypeToString(MarketUpdateType market_update_type) -> std::string{
    switch (market_update_type){
    case MarketUpdateType::ADD:
        return "ADD";

    case MarketUpdateType::MODIFY:
        return "MODIFY";

    case MarketUpdateType::CANCEL:
        return "CANCEL";

    case MarketUpdateType::TRADE:
        return "TRADE";
    case MarketUpdateType::CANCEL:
        return "CANCEL";
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
#pragma pack(pop)

typedef Common::LFQueue<Exchange::MEMarketUpdate> MEMarketUpdateLFQueue;

}