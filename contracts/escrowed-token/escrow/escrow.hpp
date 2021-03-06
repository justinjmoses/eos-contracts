
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/system.h>
#include <eosiolib/time.hpp>

using std::string;

namespace eosio {

// taken from multisig contract
time_point current_time_point() {
  const static time_point ct{
      microseconds{static_cast<int64_t>(current_time())}};
  return ct;
}

class [[eosio::contract("escrow")]] escrow : public contract {
public:
  using contract::contract;

  [[eosio::action]] void addperiod(string symbol_str, uint64_t timestamp,
                                   uint64_t numerator, uint64_t denominator);
  [[eosio::action]] void delperiods(string symbol_str);
  [[eosio::action]] void delaccount(name user, string symbol_str);
  [[eosio::action]] void vest(string symbol_str, name user);
  [[eosio::action]] void transfer(name from, name to, asset quantity,
                                  string memo);

  // The contract of the token that this
  static constexpr name token_contract = name("eosio.token");

private:
  /**
   * A table of escrow periods.
   * These stipulate what fraction a user can transfer
   */
  struct [[eosio::table]] escrow_period {
    time_point from_time = time_point(microseconds(0));
    uint64_t numerator = 0;
    uint64_t denominator = 1;

    uint64_t primary_key() const { return from_time.elapsed.count(); }

    double fraction() const { return (double)numerator / (double)denominator; }
  };
  typedef multi_index<name("periods"), escrow_period> periods;

  /**
   * A table of account balances. By creating a table "accounts" with a primary
   * key of symbol, users can use `cleos get currency balance token.escrow
   * <name>`.
   */
  struct [[eosio::table]] account {
    asset remaining;
    asset total_escrowed;

    uint64_t primary_key() const { return remaining.symbol.code().raw(); }
  };
  typedef multi_index<name("accounts"), account> accounts;

  ///////
  // Internal functions
  //////

  escrow_period get_current_period(symbol sym) {
    const auto current_time = current_time_point();

    periods plist(_self, sym.code().raw());
    uint64_t numerators = 0;
    escrow_period last_entry;

    // iterate through all escrow periods
    for (auto itr = plist.begin(); itr != plist.end(); itr++) {
      if (itr->from_time <= current_time &&
          itr->from_time > last_entry.from_time) {
        last_entry = *itr;
        numerators += last_entry.numerator;
      }
    }
    // the period is cumulative - the sum of all previous vesting periods
    last_entry.numerator = numerators;
    return last_entry;
  }
};

} // namespace eosio
