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
    ClientOrderHashmap cid_oid_to_order_;
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
      const auto orders_at_price = GetOrdersAtPrice(order->price_);
      if(!orders_at_price){
         order->next_order_ = order->prev_order_ = order;
         auto new_orders_at_price = orders_at_price_pool_.Allocate(order->side_, order->price_, order, nullptr, nullptr);
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
         bool add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) 
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
         if(add_after){
            target = target->next_entry_;
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_)
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
         }

         while(add_after && target != best_orders_by_price){
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) 
                           || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
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
      
      matching_engine_->SendClientResponse(&client_response_);

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
   auto Cancel(ClientId client_id, OrderId order_id, TickerId ticker_id) noexcept -> void{
      // checking that the client_id is valid
      auto is_cancelable = (client_id < cid_oid_to_order_.size());
      MEOrder* exchange_order = nullptr;
      // checking that the order_id specified exists in the orderbook and belongs to the client_id
      if(LIKELY(is_cancelable)){
         auto& co_itr = cid_oid_to_order_.at(client_id);
         exchange_order = co_itr.at(order_id);
         is_cancelable = (exchange_order != nullptr);
      }

      // if we don't find the order or it doesn't belong to the client, send a Cancel_Reject
      // back to the client
      if(UNLIKELY(!is_cancelable)){
         client_response_ = {ClientResponseType::CANCEL_REJECTED, client_id, ticker_id, order_id,
                             OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID};
      }else{
         client_response_ = {ClientResponseType::CANCELED, client_id, ticker_id, order_id,
                             exchange_order->market_order_id_, exchange_order->side_, 
                             exchange_order->price_, Qty_INVALID, exchange_order->qty_};
         market_update_ = {MarketUpdateType::CANCEL, exchange_order->market_order_id_, ticker_id,
                           exchange_order->side_, exchange_order->price_, 0, exchange_order->priority_};
         RemoveOrder(exchange_order);
         matching_engine_->SendMarketUpdate(&market_update_);
      }
      matching_engine_->SendClientResponse(&client_response_);
    }

    auto RemoveOrder(MEOrder* order) noexcept -> void{
      auto orders_at_price = GetOrdersAtPrice(order->price_);

      // only one element
      if(order->prev_order_ == order){ 
         RemoveOrdersAtPrice(order->side_, order->price_);
      }else{
         const auto order_before = order->prev_order_;
         const auto order_after = order->next_order_;
         order_before->next_order_ = order_after;
         order_after->prev_order_ = order_before;
         if(orders_at_price->first_me_order_ == order){
            orders_at_price->first_me_order_ = order_after;
         }
         order->prev_order_ = order->next_order_ = nullptr;
      }

      cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = nullptr;
      order_pool_.Deallocate(order);
    }

    auto RemoveOrdersAtPrice(Side side, Price price) noexcept{
      const auto best_orders_by_price = (side == Side::BUY ? bids_by_price_ : asks_by_price_);
      auto orders_at_price = GetOrdersAtPrice(price);
      if(UNLIKELY(orders_at_price->next_entry_ == orders_at_price)){
         (side == Side::BUY ? bids_by_price_ : asks_by_price_) = nullptr;
      }else{
         orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
         orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;

         if(orders_at_price == best_orders_by_price){
            (side == Side::BUY ? bids_by_price_ : asks_by_price_) = orders_at_price->next_entry_;
         }
         orders_at_price->prev_entry_ = orders_at_price->next_entry_ = nullptr; 
      }
      price_orders_at_price_.at(PriceToIndex(price)) = nullptr;
      orders_at_price_pool_.Deallocate(orders_at_price);
    }

    auto CheckForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, 
                        Qty qty, Qty new_market_order_id) noexcept{
      auto leaves_qty = qty;

      // keep matching through the OrdersAtPrice LinkedList and taking out orders at each price level
      // as long as the client order quantity > 0 and there are still price levels that the client 
      // order crosses

      if(side == Side::BUY){
         while(leaves_qty && asks_by_price_){
            const auto ask_itr = asks_by_price_->first_me_order_;
            if(LIKELY(price < ask_itr->price_)){
               break;
            }
            match(ticker_id, client_id, side, client_order_id, new_market_order_id, ask_itr, &leaves_qty);
         }
      }

      if(side == Side::SELL){
         while(leaves_qty && bids_by_price_){
            const auto bid_itr = bids_by_price_->first_me_order_;
            if(LIKELY(price > bid_itr->price_)){
               break;
            }
            match(ticker_id, client_id, side, client_order_id, new_market_order_id, bid_itr, &leaves_qty);
         }
      }

      return leaves_qty;
    }

    auto match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, Orderid new_market_order_id
               MEOrder* itr, Qty* leaves_qty) noexcept{
      const auto order = itr;
      const auto order_qty = order->qty_;
      const auto fill_qty = std::min(*leaves_qty, order_qty);
      *leaves_qty -= fill_qty;
      order->qty_ -= fill_qty;
      
      // send response to client of new order about fill
      client_response_ = {ClientResponseType::FILLED, client_id, ticker_id, client_order_id,
                           new_market_order_id, side, itr->price_, fill_qty, *leaves_qty};
      matching_engine_->SendClientResponse(&client_response_);

      // send response to client of passive order about fill
      client_response_ = {ClientResponseType::FILLED, order->client_id_, ticker_id, order->client_order_id_, 
                           order->market_order_id_, order->side_, itr->price_, fill_qty, order->qty_};
      matching_engine_->SendClientResponse(&client_response_);

      // send trade message to market as a market update
      market_update_ = {MarketUpdateType::TRADE, OrderId_INVALID, ticker_id, side, itr->price_,
                        fill_qty, Priority_INVALID};
      matching_engine_->SendMarketUpdate(&market_update_);

      // send modify or cancel of passive order to market as a market update
      if(!order->qty_){
         market_update_ = {MarketUpdateType::CANCEL, order->market_order_id_, ticker_id, order->side_,
                           order->price_, order_qty, Priority_INVALID};
         matching_engine_->SendMarketUpdate(&market_update_);
         removeOrder(order);
      }else{
         market_update_ = {MarketUpdateType::MODIFY, order->market_order_id_, ticker_id, order->side_,
                           order->price_, order->qty_, order->priority_};
         matching_engine_->SendMarketUpdate(&market_update_);
      }

    }

};

typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}