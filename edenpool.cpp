#include "edenpool.hpp"

namespace eden {
  void edenpool::on_transfer(name from, name to, asset quantity, std::string memo) {
    if (from == _self || to != _self) return;

    if (quantity.symbol != STAKE_TOKEN_SYMBOL || 
      get_first_receiver() != STAKE_TOKEN_CONTRACT) return;
    
    auto pitr = current_period();

    auto itr = accstats.find(from.value);
    if (itr == accstats.end()) {
      accstats.emplace(_self, [&](auto& row) {
        row.account = from;
        row.staked = quantity;
        row.dividended_at = pitr->dividended_at + PERIOD_TTL;
      });
    } else {
      claim(pitr, itr);
      accstats.modify(itr, _self, [&](auto& row) {
        row.staked += quantity;
      });
    }
  };

  void edenpool::unstake(name account, asset quantity) {
    require_auth(account);

    auto itr = accstats.require_find(account.value, "no stake object found");
    check(itr->staked >= quantity, "overdrawn staked");

    auto pitr = current_period();
    claim(pitr, itr);
    
    if (itr->staked == quantity) {
      accstats.erase(itr);
    } else {
      accstats.modify(itr, _self, [&](auto& row) {
        row.staked -= quantity;
      });
    }

    _transfer_out(account, STAKE_TOKEN_CONTRACT, quantity, "unstake");
  };

  void edenpool::claim(name account) {
    require_auth(account);

    auto itr = accstats.require_find(account.value, "no stake object found");

    auto pitr = current_period();
    asset dividend = claim(pitr, itr);
    check(dividend.amount > 0, "insufficient dividend to claim");
  };

  edenpool::period_tlb::const_iterator edenpool::current_period() {
    uint32_t period_secs = current_period_secs();

    asset staked = asset(0, STAKE_TOKEN_SYMBOL);
    asset reward = asset(0, REWARD_TOKEN_SYMBOL);

    auto itr = periods.begin();
    if (itr != periods.end()) {
      if (itr->dividended_at == period_secs) return itr;

      periods.erase(itr);

      for (auto itr = accstats.begin(); itr != accstats.end(); itr++) {
        staked += itr->staked;
      }
      asset reward_balance = token::get_balance(_self, REWARD_TOKEN_CONTRACT, REWARD_TOKEN_SYMBOL);
      reward.amount = reward_balance.amount * DIVIDEND_RATIO;
    }

    itr = periods.emplace(_self, [&](auto& row) {
      row.dividended_at = period_secs;
      row.reward = reward;
      row.claimed = asset(0, REWARD_TOKEN_SYMBOL);
      row.staked = staked;
    });
    return itr;
  }

  asset edenpool::claim(period_tlb::const_iterator pitr, accstat_tlb::const_iterator itr) {
    asset dividend = asset(0, pitr->reward.symbol);

    if (itr->dividended_at <= pitr->dividended_at) {
      double rate = (double)itr->staked.amount / pitr->staked.amount;
      dividend.amount = pitr->reward.amount * rate;

      if (dividend.amount > 0) {
        _transfer_out(itr->account, REWARD_TOKEN_CONTRACT, dividend, "dividend");

        periods.modify(pitr, _self, [&](auto& row) {
          row.claimed += dividend;
        });
      }

      accstats.modify(itr, _self, [&](auto& row) {
        row.dividended_at = pitr->dividended_at + PERIOD_TTL;
      });
    }
    return dividend;
  };
}