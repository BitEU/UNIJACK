/*
 * UNIJACK - Text-based Blackjack Game
 * Main implementation
 * Converted from Python implementation
 */

#include "blackjack.h"

// Suit and rank names
static const char* SUIT_NAMES[NUM_SUITS] = {"Spade", "Heart", "Diamond", "Club"};
static const char* RANK_NAMES[NUM_RANKS] = {
    "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"
};

// BSS initialization for UNIVAC
#ifdef UNIVAC
Card g_deck_template[MAX_CARDS_IN_DECK];
int g_bss_initialized = 0;

void init_bss(void) {
    if (!g_bss_initialized) {
        memset(g_deck_template, 0, sizeof(g_deck_template));
        g_bss_initialized = 1;
    }
}
#endif

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(void) {
#ifndef UNIVAC
    console_setup();
#endif

#ifdef UNIVAC
    init_bss();
#endif
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Default to UNIJACK ruleset
    Ruleset ruleset;
    init_UNIJACK_ruleset(&ruleset);
    
    // Default player
    Table table;
    init_table(&table, &ruleset);
    
    // Get player name
    char player_name[MAX_NAME_LEN];
    printf("Enter player name: ");
    if (fgets(player_name, sizeof(player_name), stdin)) {
        size_t len = strlen(player_name);
        if (len > 0 && player_name[len - 1] == '\n') {
            player_name[len - 1] = '\0';
        }
    }
    if (strlen(player_name) == 0) {
        SAFE_STRCPY(player_name, "Player", sizeof(player_name));
    }
    
    // Initialize player
    init_player(&table.players[0], player_name, 100);
    table.player_count = 1;
    
    // Create and run game
    Game game;
    init_game(&game, &ruleset);
    run_game(&game, &table);
    
    return 0;
}

// ============================================================================
// CARD OPERATIONS
// ============================================================================
void init_card(Card* card, int suit, int rank) {
    card->suit = suit;
    card->rank = rank;
    card->visible = 0;
}

void get_card_name(const Card* card, char* buffer, size_t buffer_size) {
    if (card->visible) {
        snprintf(buffer, buffer_size, "%s of %ss", 
                RANK_NAMES[card->rank], SUIT_NAMES[card->suit]);
    } else {
        snprintf(buffer, buffer_size, "<hidden>");
    }
}

void get_card_values(const Card* card, int* values, int* value_count) {
    if (card->rank == 0) {  // Ace
        values[0] = 1;
        values[1] = 11;
        *value_count = 2;
    } else if (card->rank >= 10) {  // Jack, Queen, King
        values[0] = 10;
        *value_count = 1;
    } else {  // 2-10
        values[0] = card->rank + 1;
        *value_count = 1;
    }
}

// ============================================================================
// DECK OPERATIONS
// ============================================================================
void create_deck(Card* deck) {
    int card_idx = 0;
    for (int suit = 0; suit < NUM_SUITS; suit++) {
        for (int rank = 0; rank < NUM_RANKS; rank++) {
            init_card(&deck[card_idx], suit, rank);
            card_idx++;
        }
    }
}

// ============================================================================
// SHOE OPERATIONS
// ============================================================================
void init_shoe(Shoe* shoe, int deck_count, int auto_shuffling) {
    shoe->auto_shuffling = auto_shuffling;
    shoe->total_cards = deck_count * MAX_CARDS_IN_DECK;
    shoe->current_index = 0;
    
    // Create multiple decks
    Card single_deck[MAX_CARDS_IN_DECK];
    create_deck(single_deck);
    
    for (int deck = 0; deck < deck_count; deck++) {
        for (int card = 0; card < MAX_CARDS_IN_DECK; card++) {
            int idx = deck * MAX_CARDS_IN_DECK + card;
            shoe->cards[idx] = single_deck[card];
            shoe->cards[idx].visible = 0;
        }
    }
    
    shuffle_shoe(shoe);
}

void shuffle_shoe(Shoe* shoe) {
    // Fisher-Yates shuffle
    for (int i = shoe->total_cards - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card temp = shoe->cards[i];
        shoe->cards[i] = shoe->cards[j];
        shoe->cards[j] = temp;
    }
}

void reload_shoe(Shoe* shoe) {
    shoe->current_index = 0;
    // Reset all cards to face down
    for (int i = 0; i < shoe->total_cards; i++) {
        shoe->cards[i].visible = 0;
    }
}

Card* draw_card(Shoe* shoe, int visible) {
    if (shoe->auto_shuffling) {
        shuffle_shoe(shoe);
    }
    
    if (shoe->current_index >= shoe->total_cards) {
        reload_shoe(shoe);
    }
    
    Card* card = &shoe->cards[shoe->current_index];
    card->visible = visible;
    shoe->current_index++;
    
    return card;
}

// ============================================================================
// HAND OPERATIONS
// ============================================================================
void init_hand(Hand* hand) {
    hand->card_count = 0;
    hand->wager = 0;
    memset(hand->cards, 0, sizeof(hand->cards));
}

void add_card_to_hand(Hand* hand, Card* card) {
    if (hand->card_count < MAX_CARDS_IN_HAND) {
        hand->cards[hand->card_count] = *card;
        hand->card_count++;
    }
}

void reveal_all_cards(Hand* hand) {
    for (int i = 0; i < hand->card_count; i++) {
        hand->cards[i].visible = 1;
    }
}

int get_hand_score(const Hand* hand) {
    return score_from_hand(hand);
}

// ============================================================================
// SCORE OPERATIONS
// ============================================================================
int score_from_hand(const Hand* hand) {
    int scores[100];
    int score_count = 0;
    scores[0] = 0;
    score_count = 1;
    
    // Calculate all possible scores
    for (int i = 0; i < hand->card_count; i++) {
        int values[2];
        int value_count;
        get_card_values(&hand->cards[i], values, &value_count);
        
        int new_score_count = 0;
        int new_scores[100];
        
        for (int s = 0; s < score_count; s++) {
            for (int v = 0; v < value_count; v++) {
                int new_score = scores[s] + values[v];
                // Add unique scores only
                int exists = 0;
                for (int n = 0; n < new_score_count; n++) {
                    if (new_scores[n] == new_score) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && new_score_count < 100) {
                    new_scores[new_score_count++] = new_score;
                }
            }
        }
        
        for (int n = 0; n < new_score_count && n < 100; n++) {
            scores[n] = new_scores[n];
        }
        score_count = new_score_count;
    }
    
    // Find best score (highest <= 21, or lowest > 21)
    int best_under = -1;
    int best_over = 9999;
    
    for (int i = 0; i < score_count; i++) {
        if (scores[i] <= TARGET_SCORE && scores[i] > best_under) {
            best_under = scores[i];
        }
        if (scores[i] > TARGET_SCORE && scores[i] < best_over) {
            best_over = scores[i];
        }
    }
    
    if (best_under >= 0) {
        return best_under;
    }
    return best_over;
}

void compare_hands(const Hand* hand1, const Hand* hand2, int* outcome1, int* outcome2) {
    *outcome1 = WIN;
    *outcome2 = WIN;
    
    // Examine hand1
    int score1 = score_from_hand(hand1);
    if (score1 > TARGET_SCORE) {
        *outcome1 = BUST;
    }
    if (score1 == TARGET_SCORE && hand1->card_count == 2) {
        *outcome1 = BLACKJACK;
    }
    
    // Examine hand2
    int score2 = score_from_hand(hand2);
    if (score2 > TARGET_SCORE) {
        *outcome2 = BUST;
    }
    if (score2 == TARGET_SCORE && hand2->card_count == 2) {
        *outcome2 = BLACKJACK;
    }
    
    // Resolve two blackjacks
    if (*outcome1 == BLACKJACK && *outcome2 == BLACKJACK) {
        *outcome1 = PUSH;
        *outcome2 = PUSH;
        return;
    }
    
    // Resolve one blackjack
    if (*outcome1 == BLACKJACK) {
        if (*outcome2 < LOOSE) *outcome2 = LOOSE;
        return;
    }
    if (*outcome2 == BLACKJACK) {
        if (*outcome1 < LOOSE) *outcome1 = LOOSE;
        return;
    }
    
    // Compare scores
    if (*outcome1 == WIN && *outcome2 == WIN) {
        if (score1 < score2) {
            *outcome1 = LOOSE;
            *outcome2 = WIN;
        } else if (score1 > score2) {
            *outcome1 = WIN;
            *outcome2 = LOOSE;
        } else {
            *outcome1 = PUSH;
            *outcome2 = PUSH;
        }
    }
}

// ============================================================================
// PLAYER OPERATIONS
// ============================================================================
void init_player(Player* player, const char* name, int chip_count) {
    SAFE_STRCPY(player->name, name, sizeof(player->name));
    player->chip_count = chip_count;
    init_hand(&player->hand);
}

int bet_chips(Player* player, int chip_count) {
    if (chip_count > player->chip_count) {
        return 0;  // Not enough chips
    }
    player->chip_count -= chip_count;
    player->hand.wager = chip_count;
    return 1;
}

void earn_chips(Player* player, int chip_count) {
    player->chip_count += chip_count;
}

void drop_player_hand(Player* player) {
    init_hand(&player->hand);
}

// ============================================================================
// DEALER OPERATIONS
// ============================================================================
void init_dealer(Dealer* dealer) {
    init_hand(&dealer->hand);
}

void drop_dealer_hand(Dealer* dealer) {
    init_hand(&dealer->hand);
}

// ============================================================================
// TABLE OPERATIONS
// ============================================================================
void init_table(Table* table, const Ruleset* ruleset) {
    init_shoe(&table->shoe, ruleset->deck_count_in_shoe, ruleset->auto_shuffling_shoe);
    init_dealer(&table->dealer);
    table->player_count = 0;
    table->active_player_count = 0;
}

// ============================================================================
// GAME OPERATIONS
// ============================================================================
void init_game(Game* game, const Ruleset* ruleset) {
    game->ruleset = *ruleset;
    game->running = 1;
}

void run_game(Game* game, Table* table) {
    game->running = 1;
    
    while (game->running) {
        print_colored("Starting new round...\n", "grey");
        
        // Check if anyone has chips
        int any_chips = 0;
        for (int i = 0; i < table->player_count; i++) {
            if (table->players[i].chip_count > 0) {
                any_chips = 1;
                break;
            }
        }
        
        if (!any_chips) {
            print_colored("Everyone is broke here! Bye-bye.\n", "grey");
            break;
        }
        
        int player_count = play_new_round(game, table);
        if (player_count == 0) {
            print_colored("No one wants to play anymore? Let's stop the game.\n", "grey");
            break;
        }
    }
}

int play_new_round(Game* game, Table* table) {
    int active_count = collect_wagers(game, table);
    if (active_count == 0) {
        return 0;
    }
    
    deal_initial_cards(game, table);
    
    for (int i = 0; i < table->active_player_count; i++) {
        int player_idx = table->active_player_indices[i];
        interact_with_player(game, table, player_idx);
    }
    
    interact_with_dealer(game, table);
    pay_gains(game, table);
    cleanup_table(table);
    
    return active_count;
}

int collect_wagers(Game* game, Table* table) {
    print_colored("Collecting wagers...\n", "grey");
    table->active_player_count = 0;
    
    for (int i = 0; i < table->player_count; i++) {
        Player* player = &table->players[i];
        display_player(player);
        
        while (1) {
            int chip_count = ask_integer(
                "How much would you like to bet for that round?",
                game->ruleset.minimum_wager,
                0,
                player->chip_count
            );
            
            if (chip_count == 0) {
                char msg[MAX_STRING_LEN];
                snprintf(msg, sizeof(msg), "Player \"%s\" not playing this round.\n", player->name);
                print_colored(msg, "grey");
                break;
            }
            
            if (chip_count < game->ruleset.minimum_wager) {
                char msg[MAX_STRING_LEN];
                snprintf(msg, sizeof(msg), "Minimum bet is %d\n", game->ruleset.minimum_wager);
                print_colored(msg, "grey");
                continue;
            }
            
            if (chip_count > player->chip_count) {
                print_colored("You do not have enough chips! Please lower your bet.\n", "grey");
                continue;
            }
            
            init_hand(&player->hand);
            bet_chips(player, chip_count);
            table->active_player_indices[table->active_player_count++] = i;
            break;
        }
    }
    
    if (table->active_player_count > 0) {
        init_hand(&table->dealer.hand);
    }
    
    return table->active_player_count;
}

void deal_initial_cards(Game* game, Table* table) {
    print_colored("Dealing initial two cards...\n", "grey");
    
    if (!game->ruleset.auto_shuffling_shoe) {
        shuffle_shoe(&table->shoe);
    }
    
    // First round - one card to each player and dealer
    for (int i = 0; i < table->active_player_count; i++) {
        int player_idx = table->active_player_indices[i];
        Card* card = draw_card(&table->shoe, 1);
        add_card_to_hand(&table->players[player_idx].hand, card);
    }
    Card* dealer_card = draw_card(&table->shoe, 1);
    add_card_to_hand(&table->dealer.hand, dealer_card);
    
    // Second round - second card to each player
    for (int i = 0; i < table->active_player_count; i++) {
        int player_idx = table->active_player_indices[i];
        Card* card = draw_card(&table->shoe, 1);
        add_card_to_hand(&table->players[player_idx].hand, card);
        display_player(&table->players[player_idx]);
    }
    
    // Dealer's hole card
    if (game->ruleset.dealer_receives_hole_card) {
        Card* hole_card = draw_card(&table->shoe, 0);
        add_card_to_hand(&table->dealer.hand, hole_card);
        
        if (game->ruleset.dealer_reveals_blackjack_hand) {
            if (get_hand_score(&table->dealer.hand) == TARGET_SCORE) {
                reveal_all_cards(&table->dealer.hand);
            }
        }
    }
    
    display_dealer(&table->dealer);
}

void interact_with_player(Game* game, Table* table, int player_idx) {
    Player* player = &table->players[player_idx];
    char msg[MAX_STRING_LEN * 2];
    
    (void)game;  // Suppress unused parameter warning
    snprintf(msg, sizeof(msg), "Interacting with player \"%s\"...\n", player->name);
    print_colored(msg, "grey");
    
    while (1) {
        display_player(player);
        
        int score = get_hand_score(&player->hand);
        if (score > TARGET_SCORE) {
            snprintf(msg, sizeof(msg), "Player's hand has gone bust with %d points!\n", score);
            print_colored(msg, "red");
            break;
        }
        
        char choice = ask_choice("[h]it or [s]tand?", "hs", 'h');
        
        if (choice == 'h') {
            Card* card = draw_card(&table->shoe, 1);
            add_card_to_hand(&player->hand, card);
            
            char card_name[MAX_STRING_LEN];
            get_card_name(card, card_name, sizeof(card_name));
            snprintf(msg, sizeof(msg), "Player hit and received a \"%s\".\n", card_name);
            print_colored(msg, "grey");
        } else if (choice == 's') {
            print_colored("Player stands.\n", "grey");
            break;
        }
    }
}

void interact_with_dealer(Game* game, Table* table) {
    print_colored("Interacting with dealer...\n", "grey");
    
    if (!game->ruleset.dealer_receives_hole_card) {
        Card* card = draw_card(&table->shoe, 1);
        add_card_to_hand(&table->dealer.hand, card);
    }
    
    reveal_all_cards(&table->dealer.hand);
    display_dealer(&table->dealer);
    
    char msg[MAX_STRING_LEN * 2];
    while (1) {
        int score = get_hand_score(&table->dealer.hand);
        
        if (score > TARGET_SCORE) {
            snprintf(msg, sizeof(msg), "Dealer has gone bust with %d points\n", score);
            print_colored(msg, "red");
            break;
        }
        
        if (score >= MINIMUM_DEALER_SCORE) {
            print_colored("Dealer stands.\n", "grey");
            break;
        }
        
        Card* card = draw_card(&table->shoe, 1);
        add_card_to_hand(&table->dealer.hand, card);
        
        char card_name[MAX_STRING_LEN];
        get_card_name(card, card_name, sizeof(card_name));
        snprintf(msg, sizeof(msg), "Dealer hit and received a \"%s\".\n", card_name);
        print_colored(msg, "grey");
        display_dealer(&table->dealer);
    }
}

void pay_gains(Game* game, Table* table) {
    print_colored("Paying gains...\n", "grey");
    
    char msg[MAX_STRING_LEN * 2];
    int dealer_score = get_hand_score(&table->dealer.hand);
    snprintf(msg, sizeof(msg), "Dealer has %d points with %d cards.\n",
            dealer_score, table->dealer.hand.card_count);
    print_colored(msg, "white");
    
    for (int i = 0; i < table->active_player_count; i++) {
        int player_idx = table->active_player_indices[i];
        Player* player = &table->players[player_idx];
        
        int outcome_player, outcome_dealer;
        compare_hands(&player->hand, &table->dealer.hand, &outcome_player, &outcome_dealer);
        
        int chip_payout = 0;
        int player_score = get_hand_score(&player->hand);
        
        if (outcome_player == BUST) {
            snprintf(msg, sizeof(msg), "Player \"%s\" busted with %d points.\n",
                    player->name, player_score);
            print_colored(msg, "red");
            chip_payout = 0;
        }
        else if (outcome_player == LOOSE) {
            snprintf(msg, sizeof(msg), "Player \"%s\" loses with %d points on %d cards.\n",
                    player->name, player_score, player->hand.card_count);
            print_colored(msg, "red");
            chip_payout = 0;
        }
        else if (outcome_player == PUSH) {
            snprintf(msg, sizeof(msg), 
                    "Player \"%s\" is on tie with %d points on %d cards and gets his wager back.\n",
                    player->name, player_score, player->hand.card_count);
            print_colored(msg, "yellow");
            chip_payout = player->hand.wager;
        }
        else if (outcome_player == WIN) {
            chip_payout = player->hand.wager;
            snprintf(msg, sizeof(msg), 
                    "Player \"%s\" wins with %d points on %d cards and earns %d more chips.\n",
                    player->name, player_score, player->hand.card_count, chip_payout);
            print_colored(msg, "green");
            chip_payout += player->hand.wager;
        }
        else if (outcome_player == BLACKJACK) {
            chip_payout = (int)(player->hand.wager * game->ruleset.blackjack_payout_ratio);
            snprintf(msg, sizeof(msg), "Player \"%s\" does Blackjack and earns %d more chips\n",
                    player->name, chip_payout);
            print_colored(msg, "green");
            chip_payout += player->hand.wager;
        }
        
        earn_chips(player, chip_payout);
    }
}

void cleanup_table(Table* table) {
    print_colored("Cleaning table...\n", "grey");
    
    for (int i = 0; i < table->active_player_count; i++) {
        int player_idx = table->active_player_indices[i];
        drop_player_hand(&table->players[player_idx]);
    }
    
    drop_dealer_hand(&table->dealer);
    reload_shoe(&table->shoe);
    table->active_player_count = 0;
}

// ============================================================================
// UI OPERATIONS
// ============================================================================
void print_colored(const char* msg, const char* color) {
#ifndef UNIVAC
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    if (strcmp(color, "red") == 0) {
        color_attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
    } else if (strcmp(color, "green") == 0) {
        color_attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    } else if (strcmp(color, "yellow") == 0) {
        color_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    } else if (strcmp(color, "cyan") == 0) {
        color_attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    } else if (strcmp(color, "white") == 0) {
        color_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    } else if (strcmp(color, "grey") == 0) {
        color_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
    
    SetConsoleTextAttribute(hConsole, color_attr);
    printf("%s", msg);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    (void)color;  // Suppress unused parameter warning
    printf("%s", msg);
#endif
}

void display_player(const Player* player) {
    char msg[MAX_STRING_LEN * 10];
    
    if (player->hand.card_count == 0) {
        snprintf(msg, sizeof(msg), "Player \"%s\" has %d remaining chips.\n",
                player->name, player->chip_count);
    } else {
        snprintf(msg, sizeof(msg), "Player \"%s\" has %d cards and %d remaining chips:\n",
                player->name, player->hand.card_count, player->chip_count);
        print_colored(msg, "white");
        
        for (int i = 0; i < player->hand.card_count; i++) {
            char card_name[MAX_STRING_LEN];
            get_card_name(&player->hand.cards[i], card_name, sizeof(card_name));
            snprintf(msg, sizeof(msg), "  Card \"%s\"\n", card_name);
            print_colored(msg, "white");
        }
        return;
    }
    
    print_colored(msg, "white");
}

void display_dealer(const Dealer* dealer) {
    char msg[MAX_STRING_LEN * 10];
    
    snprintf(msg, sizeof(msg), "Dealer has %d cards:\n", dealer->hand.card_count);
    print_colored(msg, "white");
    
    for (int i = 0; i < dealer->hand.card_count; i++) {
        char card_name[MAX_STRING_LEN];
        get_card_name(&dealer->hand.cards[i], card_name, sizeof(card_name));
        snprintf(msg, sizeof(msg), "  Card \"%s\"\n", card_name);
        print_colored(msg, "white");
    }
}

int ask_integer(const char* msg, int default_value, int min_value, int max_value) {
    char buffer[MAX_STRING_LEN];
    char prompt[MAX_STRING_LEN];
    
    while (1) {
        snprintf(prompt, sizeof(prompt), "%s (default=%d): ", msg, default_value);
        print_colored(prompt, "cyan");
        
        if (fgets(buffer, sizeof(buffer), stdin)) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            
            if (strlen(buffer) == 0) {
                return default_value;
            }
            
            int value = atoi(buffer);
            if (value < min_value || value > max_value) {
                char error[MAX_STRING_LEN];
                snprintf(error, sizeof(error), "Value must be between %d and %d\n", 
                        min_value, max_value);
                print_colored(error, "red");
                continue;
            }
            
            return value;
        }
    }
}

char ask_choice(const char* msg, const char* choices, char default_choice) {
    char buffer[MAX_STRING_LEN];
    char prompt[MAX_STRING_LEN];
    
    while (1) {
        snprintf(prompt, sizeof(prompt), "%s (%s) (default=%c): ", 
                msg, choices, default_choice);
        print_colored(prompt, "cyan");
        
        if (fgets(buffer, sizeof(buffer), stdin)) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            
            if (strlen(buffer) == 0) {
                return default_choice;
            }
            
            char choice = tolower(buffer[0]);
            if (strchr(choices, choice) != NULL) {
                return choice;
            }
            
            print_colored("Not a valid choice!\n", "red");
        }
    }
}

// ============================================================================
// RULESET OPERATIONS
// ============================================================================
void init_basic_ruleset(Ruleset* ruleset) {
    ruleset->maximum_player_count = 1;
    ruleset->deck_count_in_shoe = 1;
    ruleset->auto_shuffling_shoe = 0;
    ruleset->minimum_wager = 1;
    ruleset->dealer_receives_hole_card = 0;
    ruleset->dealer_reveals_blackjack_hand = 0;
    ruleset->blackjack_payout_ratio = 2.0;
}

void init_european_ruleset(Ruleset* ruleset) {
    ruleset->maximum_player_count = 7;
    ruleset->deck_count_in_shoe = 6;
    ruleset->auto_shuffling_shoe = 0;
    ruleset->minimum_wager = 10;
    ruleset->dealer_receives_hole_card = 0;
    ruleset->dealer_reveals_blackjack_hand = 0;
    ruleset->blackjack_payout_ratio = 1.5;
}

void init_american_ruleset(Ruleset* ruleset) {
    ruleset->maximum_player_count = 7;
    ruleset->deck_count_in_shoe = 8;
    ruleset->auto_shuffling_shoe = 1;
    ruleset->minimum_wager = 10;
    ruleset->dealer_receives_hole_card = 1;
    ruleset->dealer_reveals_blackjack_hand = 1;
    ruleset->blackjack_payout_ratio = 1.5;
}

void init_UNIJACK_ruleset(Ruleset* ruleset) {
    init_american_ruleset(ruleset);
    ruleset->maximum_player_count = 1;
    ruleset->minimum_wager = 1;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
void to_upper(char* str) {
    while (*str) {
        *str = (char)toupper(*str);
        str++;
    }
}

int safe_min(int a, int b) {
    return (a < b) ? a : b;
}

int safe_max(int a, int b) {
    return (a > b) ? a : b;
}

// ============================================================================
// PLATFORM-SPECIFIC FUNCTIONS
// ============================================================================
#ifndef UNIVAC
void console_setup(void) {
    // Enable virtual terminal processing for better console output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#endif
