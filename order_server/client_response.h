#pragma once
#include <sstream>
#include "common/types.h"
#include "common/lf_queue.h"

using namespace Common;

namespace Exchange{
#pragma pack(push, 1)
enum class ClientResponseType : uint8_t{
    INVALID = 0,
    ACCEPTED = 1,
    CANCELED = 2,
    FILLED = 3,
    CANCEL_REJECTED = 4
};

inline std::string ClientResponseTypeToString(ClientResponseType type){
    switch (type){
    case ClientResponseType::ACCEPTED:
        return "ACCEPTED";
    case ClientResponseType::CANCELED:
        return "CANCELLED";
    case ClientResponseType::FILLED:
        return "FILLED";
    case ClientResponseType::CANCEL_REJECTED:
        return "CANCEL_REJECTED";
    case ClientResponseType::INVALID:
        return "INVALID";
    }
    return "UNKNOWN";
}

struct MEClientResponse{
    ClientResponseType type_ = ClientResponseType::INVALID;
    ClientId client_id_ = ClientId_INVALID;
    TickerId ticker_id_ = TickerId_INVALID;
    OrderId client_order_id_ = OrderId_INVALID;
    OrderId market_order_id_ = OrderId_INVALID;
    Side side_ = Side::INVALID;
    Price price_ = Price_INVALID;
    Qty exec_qty_ = Qty_INVALID;
    Qty leaves_qty_ = Qty_INVALID;
    auto ToString() const{
        std::stringstream ss;
        ss << "MEClientResponse" << " ["
        << "type: " << ClientResponseTypeToString(type_)
        << " client: " << ClientIdToString(client_id_)
        << " ticker: " << TickerIdToString(ticker_id_)
        << " client_order_id: " << OrderIdToString(client_order_id_)
        << " market_order_id: " << OrderIdToString(market_order_id_)
        << " side: " << SideToString(side_)
        << " exec_qty: " << QtyToString(exec_qty_)
        << " leaves_qty: " << QtyToString(leaves_qty_)
        << " price: " << PriceToString(price_)
        << "]";
        return ss.str();
    }
};
#pragma pack(pop)

// client response lock-free queue
typdef LFQueue<MEClientResponse> ClientResponseLFQueue;
}