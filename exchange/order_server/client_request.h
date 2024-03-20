#pragma once
#include <sstream>
#include "../../common/types.h"
#include "../../common/lf_queue.h"

using namespace Common;

namespace Exchange{
#pragma pack(push,1)
enum class ClientRequestType : uint8_t{
    INVALID = 0,
    NEW = 1,
    CANCEL = 2
};

inline std::string ClientRequestTypeToString(ClientRequestType type){
    switch (type){
    case ClientRequestType::NEW:
        return "NEW";
    
    case ClientRequestType::CANCEL:
        return "CANCEL";
    
    case ClientRequestType::INVALID:
        return "INVALID";
    }
    return "UNKNOWN";
}

struct MEClientRequest{
    ClientRequestType type_ = ClientRequestType::INVALID;
    ClientId client_id_ = ClientId_INVALID;
    TickerId ticker_id_ = TickerId_INVALID;
    OrderId order_id_ = OrderId_INVALID;
    Side side_ = Side::INVALID;
    Price price_ = Price_INVALID;
    Qty qty_ = Qty_INVALID;
    auto ToString() const{
        std::stringstream ss;
        ss << "MEClientRequest" << " [" << "type: " << ClientRequestTypeToString(type_)
        << " client: " << ClientIdToString(client_id_)
        << " ticker: " << TickerIdToString(ticker_id_)
        << " oid: " << OrderIdToString(order_id_)
        << " side: " << SideToString(side_)
        << " qty: " << QtyToString(qty_)
        << " price: " << PriceToString(price_)
        << "]";
        return ss.str();
    }
};

#pragma pack(pop)
typedef LFQueue<MEClientRequest> ClientRequestLFQueue;
}