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
      const auto orders_at_price = GetOrdersAtPrice(order->price);
      if(!orders_at_price){
         order->next_order_ = order->prev_order_ = order;
         auto new_orders_at_price = orders_at_price_pool_.Allocate(order->side_, orders->price_, order, nullptr, nullptr);
         AddOrdersAtPrice(new_orders_at_price);
      }else{
         auto first_order = (orders_at_price ? orders_at_price->first_me_order_ : nullptr);
         first_order->prev_order_ = first_order->next_order_ = order;
         order->prev_order_ = first_order->prev_order_;
         order->next_order_ = first_order;
         first_order->prev_order_ = order;
      }

      cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = order;
   }

   auto AddOrdersAtPrice(MEOrdersAtPrice* new_orders_at_price) noexcept {
      price_orders_at_price_.at(PriceToIndex(new_orders_at_price->price_)) = new_orders_at_price;
      // get best bids and best asks for either side that's associated with our MEOrdersAtPrice object
      const auto best_orders_by_price = (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_);

      // need to handle edge cased where bids or asks are empty
      // then we need to set the bids_by_price_ or asks_by_price_ ourselves
      // which points to the head of the sorted list for it's side
      if(UNLIKELY(!best_orders_by_price)){
         (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
         new_orders_at_price->prev_entry_ = new_orders_at_price->next_entry_ = new_orders_at_price;
      }else{
         // now we have a list of orders and need to find the correct entry in teh linked list of price levels
         // so we will walk through the bids or asks till we find the correct price level and then we will insert
         // we will use a boolean "add_after" to determine if we should insert before or after the target
         // price level
         auto target = best_orders_by_price;
         bool add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price) 
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
         if(add_after){
            target = target->next_entry_;
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_)
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
         }

         while(add_after && target != best_orders_by_price){
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) 
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->side_ < target->price_));
            if(add_after){
               target = target->next_entry_;
            }
         }

         // Need to append the new price level by updating the prev_entry_ or next_entry variables
         // add new_orders_at_price after target.
         if(add_after){
            if(target == best_orders_by_price){
               target = best_orders_by_price->prev_entry_;
            }
            new_orders_at_price->prev_entry_ = target;
            target->next_entry_->prev_entry_ = new_orders_at_price;
            new_orders_at_price->next_entry_ = target->next_entry_;
            target->next_entry_ = new_orders_at_price;
         }else{
            // add new_orders_at_price before target
            new_orders_at_price->prev_entry_ = target->prev_entry_;
            new_orders_at_price->next_entry_ = target;
            target->prev_entry_->next_entry_ = new_orders_at_price;
            target->prev_entry_ = new_orders_at_price;

            // Need to check if this price level is now best bid/best ask
            if((new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_)
                  || (new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ < best_orders_by_price->price_)){
               target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price : target->next_entry_);
               (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
            }
         }
      }

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