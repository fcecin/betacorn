/**
 * This contract interacts with the 'acornaccount' ACORN token contract to allow
 *   'host' and 'player' clients to bet against each other.
 *
 * The 'host' offers commitments and reveals them. It can serve games as long as it has 
 *   a positive balance of deposits.
 *
 * Players play by simply transferring the token into this contract. The contract detects
 *   the transfer action and reacts accordingly, i.e. tries to immediately match it to a 
 *   host commitment. If it can't, the player's transfer is rejected.
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("betacorn")]] dice : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
	 void withdraw( name to, asset quantity );

         [[eosio::action]]
	 void commit( name host, const checksum256& commitment );

	 [[eosio::action]]
	 void cancelcommit( name host, const checksum256& commitment );

	 [[eosio::action]]
	 void reveal( const checksum256& commitment, const checksum256& source );

	 [[eosio::action]]
	 void collect( name player );

	 void acorn_transfer( name from, name to, asset quantity, string memo );

      private:

	 static constexpr symbol ACORN_SYMBOL = symbol("ACORN", 4);
	 static constexpr name NULL_NAME = name("");
	 static const char NULL_GUESS = 0x7F;
	 static const int MAX_BET_TO_BANKROLL_RATIO = 100; // Bets can't exceed 1% of a host's bankroll.
	 static const int MIN_TRANSFER_SHELLS = 100; // Minimum deposit+bet & withdrawal amount in ACORN shells. 
	 static const uint32_t GAME_TIMEOUT_SECS = 5*60; // 5 minute timeouts: hosts must be automated & poll frequently.
	 static const uint64_t ZERO_SOURCE = 0x6c8fc18b8e9f8e20; // 64-bit commitment key of a 0x00..000 32-byte sha256 hash source 
	 inline static const asset ZERO_ACORNS = asset(0, ACORN_SYMBOL);
	 inline static const asset ACORN_SHELL = asset(1, ACORN_SYMBOL);
	 inline static const asset MIN_BALANCE = asset(500000, ACORN_SYMBOL);

	 // --------------------------------------------------------------------------------------
	 // User deposits. This is a global table because we need to iterate on all of it.
	 // Only game hosts need to deposit into an account on this contract.
	 // --------------------------------------------------------------------------------------

	 struct [[eosio::table]] account {
	   name       owner;
	   asset      balance;

	   uint64_t primary_key() const { return balance.symbol.code().raw(); }

	   // So we can find a specific user's (host's) balance.
	   uint64_t get_owner() const { return owner.value; }
         };

	 typedef eosio::multi_index<"accounts"_n, account, 
				    indexed_by<"byowner"_n, const_mem_fun<account, uint64_t, &account::get_owner>>
				    > accounts;

	 // --------------------------------------------------------------------------------------
	 // This is a global list of offered games.
	 // When a player is matched to game, the game entry is deleted and a corresponding, new
	 //   match entry is created to track the ongoing game (see below).
	 // This *could* merge with the 'match' table, but I think it's faster and safer to have
	 //   a dedicated table that contains only open game offers (players find a vacant game
	 //   room faster, only having to waddle through the house/account list first).
	 // --------------------------------------------------------------------------------------

	 struct [[eosio::table]] game {
	   checksum256      commitment;
	   name             host;

	   // Commitment hashes cannot share the same 64-bit prefix (very low probability).
	   // Colliding commitments will have their submissions rejected. 
	   uint64_t primary_key() const { return get_hash_prefix(commitment); }

	   // We match an incoming player bet to a host (which has the sufficient balance to cover
	   //   the bet) and then we'll take that host's first game offer.
	   uint64_t get_host() const { return host.value; }
	 };

	 typedef eosio::multi_index<"games"_n, game, 
				    indexed_by<"byhost"_n, const_mem_fun<game, uint64_t, &game::get_host>>
				    > games;

	 // --------------------------------------------------------------------------------------
	 // This is a global list of offers taken by a player and that are waiting a commitment
	 //   reveal or a timeout.
	 // Unfortunately, since an intercepted incoming transfer from a player cannot
	 //   allocate RAM, the match is preallocated together with the game when the host
	 //   publishes a commitment. Empty matches have the guess set to NULL_GUESS.
	 // --------------------------------------------------------------------------------------

	 struct [[eosio::table]] match {
	   checksum256      commitment;
	   name             host;
	   char             guess;
	   name             player;
	   asset            bet;
	   time_point_sec   deadline;

	   // Commitment hashes cannot share the same 64-bit prefix (very low probability).
	   // Colliding commitments will have their submissions rejected. 
	   uint64_t primary_key() const { return get_hash_prefix(commitment); }

	   // Players want to check all their games for timeouts at once. 
	   uint64_t get_player() const { return player.value; }
	 };

	 typedef eosio::multi_index<"matches"_n, match, 
				    indexed_by<"byplayer"_n, const_mem_fun<match, uint64_t, &match::get_player>>
				    > matches;

	 // --------------------------------------------------------------------------------------
	 // Helpers
	 // --------------------------------------------------------------------------------------

	 void do_bet( name player, asset quantity, char guess );

	 void pay( name to, asset quantity, string memo );

	 void add_balance( name owner, asset value, bool enforce_min );
	 void sub_balance( name owner, asset value, bool enforce_min );

	 static uint64_t get_hash_prefix( const checksum256& hash ) {
	   const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
	   return p64[0];
	 }

	 static uint32_t get_current_time() { return current_time_point().sec_since_epoch(); }	 
   };

} /// namespace eosio
