# eden pool contract

## deploy the contract

### modify reward token and stake token

modify constants in file `edenpool.hpp`:
- `REWARD_TOKEN_SYMBOL`
- `REWARD_TOKEN_CONTRACT`
- `STAKE_TOKEN_SYMBOL`
- `STAKE_TOKEN_CONTRACT`

### build

```shell
eosio-cpp edenpool.cpp -o edenpool.wasm
```

### deploy

```shell
cleos set contract {{CONTRACT_ACCOUNT}} ./ edenpool.wasm edenpool.abi
```

### add eosio.code permission

```shell
cleos set account permission edenpoolcont active --add-code 
```

### transfer reward token to the contract account

the reward for each period is 5% of the contract account's balance

## use

### how to stake

transfer the stake token to the contract

### how to unstake

push the action `unstake`

### how to claim reward

push the action `claim`