# betacorn
A marketplace for 50/50 ACORN bets between bot hosts and players, similar to the original EOSIO "Dice" tutorial contract.

I have done some local testing and this seems to work, but this is still EXPERIMENTAL SOFTWARE.

This contract is intended for deployment in the Telos network (http://telosfoundation.io) and to interact with the "acornaccount" contract, which implements the ACORN token on Telos.

This contract allows "hosts" to deposit a balance of ACORN and then publish a series of SHA256 hashes of 256-bit random numbers as game commitments.

Then "players" can guess the source 256-bit random numbers to be either odd or even, by placing a bet that is at most 1% of the host's current deposited balance.

Finally, "hosts" reveal the 256-bit random number source, and the "player" receives either an "Lose" transaction of 0.0001 ACORN or a "Win" transaction with their winnings.

The contract has several checks and mitigations against spam and DoS attempts, but they might now be sufficient if the contract and/or ACORN gets too popular.

Players can play by merely sending ACORN into the contract. No need to interact with any other custom actions. The contract intercepts incoming transfers and sorts them out depending on the memo field.

An incoming transfer with a memo of "odd" (or "0") or "even" (or "1") is interpreted as a player bet.

An incoming transfer with a memo of "deposit" is interpreted as a games host increasing their available bankroll.

It is recommended that a bot/script (or several copies) is hosted at some always-on server machine to monitor a host's pending game offers. If a player accepts a game offer, the host has FIVE MINUTES to respond with a proper reveal() action call otherwise the player will be able to win by default by calling the collect() action.

