# UNIJACK

UNIJACK is a text-based blackjack game written in C based off of [InsightBlackjack](https://github.com/YoannAubineau/InsightBlackjack).

## Installation

### C Version

The C version of UNIJACK can be compiled using either MinGW (GCC) or MSVC on Windows.
It also supports cross-compilation for UNIVAC systems with proper BSS initialization.

To build the C version:

```bat
build.bat
```

The build script will prompt you to:
1. Select target platform (Windows or UNIVAC)
2. Select compiler (MinGW or MSVC for Windows builds)

The resulting executable will be `blackjack.exe` (Windows) or `blackjack_univac.exe` (UNIVAC).

## Usage

### C Version

To run the C version after building:

```bat
blackjack.exe
```

The program will prompt you for your name and then start the game with the UNIJACK ruleset
using a single player with 100 starting chips.

### Select a ruleset

UNIJACK comes with 4 slightly differents rulesets:

* basic
* european
* american
* UNIJACK

**Basic** ruleset is single player only. It uses only one deck of 52 cards. Minimum bet is 1 chip and Blackjack payout ratio is 2:1. It is the most simple ruleset used by beginners.

**European** ruleset is multi-player with up to 7 concurrent players. It uses 6 decks of 52 cards. Minimum bet is 10 chips and Blackjack payout ratio is 3:2. It is the most common ruleset you would encounter in casinos in Europe.

**American** ruleset is multi-player with up to 7 concurrent players. It uses 8 decks of 52 cards with an auto-shuffling shoe. Minimum bet is 10 chips and Blackjack payout ratio is 3:2. It is the most common ruleset you would encounter in casinos in the USA.

**UNIJACK** ruleset is a custom ruleset specifically designed for UNIJACK Coding Challenge. It plays with only one player, using 8 decks of 52 cards with an auto-shuffling shoe. Minimum bet is 1 chip and Blackjack payout ratio is 3:2.

The default ruleset is **UNIJACK**. To choose a different ruleset, use the
`--ruleset` option. For example:

```sh
./blackjack --ruleset american Stuey
```

This command starts a new game with one player called *Stuey*, following
**american** ruleset.

In any case, each player starts with 100 chips and is allowed to play as long as he owns enough chips to honor the minimum bet.

### Multi-player game

In order for multiple players to play on the same table, each player's name has
to be given on the command line.

The maximum number of names that can be given at the same time depends on the
selected ruleset (see above for details). Note that, currently, only the
**european** and **american** rulesets allow multiple players to play on the
same table.

For example, if I want to play on the same table as Stuey, here is how I would
launch the game:

```sh
./blackjack --ruleset american Stuey Yoann
```
