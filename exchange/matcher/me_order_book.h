#pragma once

#include "../../common/types.h"
#include "../../common/mem_pool.h"
#include "../../common/logging.h"
#include "../order_server/client_response.h"
#include "../market_data/market_update.h"
#include "matching_engine.h"
#include "me_order.h"

using namespace Common;

/*
Limit Order book data members:
1. A matching_engine_ pointer variable to the MatchingEngine parent for the orderbook to publish
   order responses and market data updates to

2. The ClientOrderHashMap variable, cid_oid_to_order_, to track the OrderHashMap objects by their ClientId key.
   As a reminder, OrderHashMap tracks the MEOrder objects by their OrderId keys.

3. The orders_at_price_pool_ memory pool variable of the MEOrdersAtPrice objects to create
   new objects from and return dead objects back to.

4. The head of the doubly linked list of bids (bids_by_price_) and asks (asks_by_price_), since we track orders
   at the price level as a list of MEOrdersAtPrice objects.

5. A hashmap, OrdersAtPriceHashmap, to track the MEOrdersAtPrice objects for the price levels, using the price 
   of the level as a key into the map.

6. A memory pool of the MEOrder objects, called order_pool_, where MEOrder objects are created from and returned 
   to without incurring dynamic memory allocations.

7. Some minor members, such as TickerId for the instrument for this order book, OrderId to track the next 
   market data order ID, an MEClientResponse variable (client_response_), an MEMarketUpdate object 
   (market_update_), a string to log time, and the Logger object for logging purposes.
*/

namespace Exchange{
class MatchingEngine;

class MEOrderBook final{
private:
    TickerId ticker_id_ = TickerId_INVALID;
    MatchingEngine* matching_engine_ = nullptr;
    ClientOrderHashMap cid_oid_to_order_;
    OrdersAtPriceHashMap price_orders_at_price_;
    MemPool<MEOrdersAtPrice> orders_at_price_pool_;
    MEOrdersAtPrice* bids_by_price_ = nullptr;
    MEOrdersAtPrice* asks_by_price_ = nullptr;
    MemPool<MEOrder> order_pool_;
    OrderId next_market_order_id_ = 1;
    MEClientResponse client_response_;
    MEMarketUpdate market_update_;
    std::string time_str_;
    Logger* logger_ = nullptr;

public:
    // Constructor
    MEOrderBook(TickerId ticker_id, Logger* logger, Exchange::MatchingEngine* matching_engine): 
                ticker_id_(ticker_id), logger_(logger), matching_engine_(matching_engine),
                orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS){
        
    }

    MEOrderBook::~MEOrderBook(){
        logger_->Log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__, 
                    Common::GetCurrentTimeStr(&time_str_), ToString(false, true));
        matching_engine_ = nullptr;
        bids_by_price_ = nullptr;
        asks_by_price_ = nullptr;
        for(auto &itr : cid_oid_to_order_){
            itr.fill(nullptr);
        }
    }

    // deleted copy constructor, move constructor and assignment operators
    MEOrderBook() = delete;
    MEOrderBook(const MEOrderBook&) = delete;
    MEOrderBook(const MEOrderBook&&) = delete;
    MEOrderBook& operator=(const MEOrderBook&) = delete;
    MEOrderBook& operator=(const MEOrderBook&&) = delete;

   // return the next market order id
   auto GenerateNewMarketOrderId() noexcept -> OrderId{
      return next_market_order_id_;
   }

   // converts a price to an index that ranges between 0 and ME_MAX_PRICE_LEVELS-1
   // used to index the prices levels std::array
   auto PriceToIndex(Price price) const noexcept{
      return (price % ME_MAX_PRICE_LEVELS);
   }

   auto GetOrdersAtPrice(Price price) const noexcept -> MEOrdersAtPrice*{
      return price_orders_at_price_.at(PriceToIndex(price));
   }

      // adds order to the book
   auto AddOrder(MEOrder* order) noexcept{

   }

   // get priority value by checking if there are orders at a price level
   // and returning the last order at a price level's priority + 1
   auto GetNextPriority(TickerId ticker_id, Price price){
      const auto orders_at_price = GetOrdersAtPrice(price);
      if(!orders_at_price){
         return 1lu;
      return orders_at_price->first_me_order_->prev_order_->priority_ + 1;
      }
   }

    // Add()
   auto Add(ClientId client_id, OrderId client_order_id, 
            TickerId ticker_id, Side side, 
            Price price, Qty qty) noexcept -> void{
      // set client_response attributes
      const auto new_market_order_id = GenerateNewMarketOrderId();
      client_response_ = {ClientResponseType::ACCEPTED, client_id, ticker_id, client_order_id, 
                          new_market_order_id, side, price, 0, qty};
      
      matching_engine->SendClientResponse(&client_response_);

      // check if order has matches with passive orders in orderbook
      const auto leaves_qty = CheckForMatch(client_id, client_order_id, ticker_id, side, price, qty, new_market_order_id);

      if(LIKELY(leaves_qty)){
         const auto priority = GetNextPriority(ticker_id, price);
         // create new order based on MEOrder Constructor and allocate it
         // from order_pool
         auto order = order_pool_.Allocate(ticker_id, client_id, client_order_id,
                                           new_market_order_id, side, price, leaves_qty,
                                           priority, nullptr, nullptr);
         // add order to book
         AddOrder(order);
          // create new market update
         market_update_ = {MarketUpdateType::ADD, client_response_.market_order_id_,
                                          ticker_id, side, price, leaves_qty, priority};
         matching_engine_->SendMarketUpdate(&market_update_);
      }
   }

   // Cancel()
   void Cancel(ClientId client_id, OrderId order_id, TickerId ticker_id_){

    }

};

typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}