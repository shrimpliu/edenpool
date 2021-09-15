#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>

using namespace eosio;

namespace token {
  struct account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }
  };

  typedef eosio::multi_index<name("accounts"), account > account_tlb;

  asset get_balance(name account, name contract, symbol sym) {
    account_tlb accounts(contract, account.value);
    auto itr = accounts.find(sym.code().raw());
    if(itr == accounts.end()){
        return asset(0, sym);
    }
    return itr->balance;
  };
}