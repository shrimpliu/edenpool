#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/system.hpp>

#include "token.hpp"

using namespace eosio;

#define PERIOD_START 1631491200

// dividend ratio
#define DIVIDEND_RATIO 0.05

// dividend period
#define PERIOD_TTL 604800

// reward token
#define REWARD_TOKEN_SYMBOL symbol("EOS", 4)
// reward token contract
#define REWARD_TOKEN_CONTRACT name("eosio.token")
// stake token
#define STAKE_TOKEN_SYMBOL symbol("SPT", 4)
// stake token contract
#define STAKE_TOKEN_CONTRACT name("tokenfreedom")

namespace eden {
  uint32_t current_secs() {
    time_point tp = current_time_point();
    return tp.sec_since_epoch();
  };

  uint32_t pad_secs(uint32_t secs, uint32_t ttl, int32_t offset = 0) {
    return secs - (secs + offset)%ttl;
  }

  class [[eosio::contract]] edenpool : public contract {
  public:
    edenpool(name self, name first_receiver, datastream<const char*> ds) :
      contract(self, first_receiver, ds), periods(self, self.value), 
      accstats(self, self.value) {}

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(name from, name to, asset quantity, std::string memo);

    [[eosio::action]]
    void unstake(name account, asset quantity);

    [[eosio::action]]
    void claim(name account);
  
  private:
    struct [[eosio::table]] period {
      uint32_t dividended_at;
      asset reward;
      asset claimed;
      asset staked;

      uint64_t primary_key() const {
        return dividended_at;
      }
    };
    typedef eosio::multi_index<name("period"), period> period_tlb;
    period_tlb periods;

    uint32_t current_period_secs() {
      uint32_t secs = current_secs();
      return pad_secs(secs, PERIOD_TTL, -PERIOD_START);
    };

    period_tlb::const_iterator current_period();

    struct [[eosio::table]] accstat {
      name account;
      asset staked;
      uint32_t dividended_at;

      uint64_t primary_key() const {
        return account.value;
      }
    };
    typedef eosio::multi_index<name("accstat"), accstat> accstat_tlb;
    accstat_tlb accstats;

    asset claim(period_tlb::const_iterator pitr, accstat_tlb::const_iterator itr);

    void _transfer_out(name to, name contract, asset quantity, std::string memo) {
      action(
        permission_level{_self, name("active")},
        contract,
        name("transfer"),
        std::make_tuple(_self, to, quantity, memo)
      ).send();
    };
  };
}