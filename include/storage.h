#pragma once
#include <stddef.h>
#include <sys/types.h>

/**
 * Storage backend for game data.
 */

// Length of words in the game
#define wordle_len 5
// Maximum guesses per round
#define max_turns 6

/**
 * The word of the day
 */ 
struct wordle {
    char word[wordle_len];
};

/**
 * State of a letter in a guess
 */ 
enum guess_letter_state {
    // The letter is not present in the correct answertyp
    incorrect,
    // The letter is in the correct answer, but not in it's current position
    present_wrong_pos,
    // The letter is present in this position in the correct answer
    correct,
};

/**
 * An individual guess at a letter
 */ 
struct guess_letter {
    char letter;
    enum guess_letter_state state;
};

/**
 * An indvidual guess at a word
 */ 
struct guess {
    struct guess_letter guess[wordle_len];
};

/**
 * Game state for a player's current wordle.
 */
struct game_state {
    // How many guesses have been submitted?
    uint turns_len;
    // The guesses that have been submitted already
    struct guess turns[max_turns];
};

void game_state_print(struct game_state state);


/**
 * Handle for accessing the storage backend.
 * This is the first argument for any function which is expected to access the storage backend
 * And so will not be documented in any functions using it.
 */ 
struct storage;

/**
 * Initialize the game-data storage backend.
 * 
 * Expects the program arguments to be passed verbatim, so boot code doesn't have to 
 * care about how this module interprets arguments.
 * 
 * /argc 
 *   count of program arguments
 * /argv
 *   program arguments array
 * /error_message
 *   populated if some problem with the configuration was found
 * /return
 *   a handle to the game data storage, ready to use
 */ 
struct storage* init_storage(int argc, char* argv[], char** error_message);

void setup_test_storage(struct storage* storage);

void free_storage(struct storage* storage);

/**
 * Return today's word from the winning wordlist.
 * /buffer 
 *   buffer to put the word into
 */ 
struct wordle todays_answer(struct storage* storage);

/**
 * Return today's game for a given user
 * /user_name
 *   the name of the user whose game we're looking for
 * /buffer
 *   pre-allocated struct to fill with game data
 * /error_message
 *   if the user isn't found, it'll contain an error message
 */
struct game_state todays_game(struct storage* storage, char* user_name, char** error_message);

/**
 * Save a new guess in today's game for a user
 * /user_name
 *   name of the user making the guess
 * /guess
 *   the guess they made
 */ 
void save_guess(struct storage* storage, char* user_name, char guess[wordle_len]);