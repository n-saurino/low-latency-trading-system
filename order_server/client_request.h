#pragma once
#include <sstream>
#include "common/types.h"
#include "common/lf_queue.h"

using namespace Common;

namespace Exchange{
    enum class ClientRequestType : uint8_t{
        INVALID = 0,
        NEW = 1,
        CANCEL = 2
    };

    inline std::string ClientRequestToString(ClientRequestType type) -> std::string{
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

    
}