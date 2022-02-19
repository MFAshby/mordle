#include <stdio.h>
#include <string.h>
#include "game.h"

static bool is_correct(struct guess guess);

/**
 * Validate's the move against the current game, then makes the move
 * Validation: 
 *  - is the supplied user actually a player?
 *  - is the guess a valid length?
 *  - have they already won?
 *  - have they already lost?
 */ 
bool guess(struct storage* storage, struct game_user game_user, char* guess_input, char** error_message) {
    struct game_state state = todays_game(storage, game_user);

    if (won(state)) {
        *error_message = "You have already won!";
        return false;
    }

    if (lost(state)) {
        *error_message = "You have already lost!";
        return false;
    }
    
    if (strlen(guess_input) < wordle_len) {
        *error_message = "Invalid length of guess!";
        return false;
    }

    if (!in_wordlist(storage, guess_input)) {
        *error_message = "Not in word list!";
        return false;
    }

    save_guess(storage, game_user, guess_input);

    // re-fetch, check  if we won.lost with this addition
    state = todays_game(storage, game_user);

    if (won(state) | lost(state)) {
        save_game_result(storage, game_user, won(state), state.turns_len);
    }

    return true;
}

/**
 * Checks if a game has been won
 */ 
bool won(struct game_state state) {
    for (uint i=0; i<state.turns_len; i++) {
        if (is_correct(state.turns[i])) {
            return true;
        }
    }
    return false;
}

bool lost(struct game_state state) {
    return state.turns_len == max_turns
        && !won(state);
}

/**
 * Checks if a guess is completely correct
 */ 
static bool is_correct(struct guess guess) {
    bool all_correct = true;
    for (uint i=0; i<wordle_len; i++) {
        all_correct = all_correct && guess.guess[i].state == letter_state_correct;
    }
    return all_correct;
}
