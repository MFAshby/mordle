#pragma once
#include "user.h"
#include <stddef.h>
#include <sys/types.h>
#include <sodium.h>

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

#define max_name_len 30
#define password_hash_len 30
#define password_hash_salt_len 30
#define session_token_len 30
#define csrf_token_len 30

/**
 * Represents a unique individual playing the game.
 * Users will appear on the leaderboard by name.
 * Users must be authenticated to play.
 */ 
struct game_user {
    // Surrogate key for the user.
    int id;
    // Unique string identifying the user. 
    // User selects their own name.
    char name[max_name_len];

    // argon2 hashed password for the user, and salt.
    char password_hash[password_hash_len];
    char password_hash_salt[password_hash_salt_len];
};

/**
 * Represents a user logged into a browser.
 * Same user could be logged into multiple browsers.
 */ 
struct session {
    // session token is stored in a cookie after login, identifies the user
    // without them providing a user+pass every request
    char session_token[session_token_len];

    // Cross Site Request Forgery prevention, a random token 
    // provided as a hidden field on every form so that only form submissions
    // from our own pages are accepted
    char csrfToken[csrf_token_len];
};

/**
 * Combination of user and their _current_ session for the request.
 */ 
struct user_and_session {
    struct game_user game_user;
    struct session session;
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
 * /game_user
 *   the user whose game we're looking for
 * /buffer
 *   pre-allocated struct to fill with game data
 * /error_message
 *   if the user isn't found, it'll contain an error message
 */
struct game_state todays_game(struct storage* storage, struct game_user);

/**
 * Save a new guess in today's game for a user
 * /game_user
 *   the user making the guess
 * /guess
 *   the guess they made
 */ 
void save_guess(struct storage* storage, struct game_user game_user, char guess[wordle_len]);

/**
 * /session_token
 *   Session token by which we'll find the user. If there isn't a user, we'll create one
 *   with a random name for anonymous users
 * /return
 *   either the existing user if they were found, or a new one
 */ 
struct game_user find_or_create_user_by_session(struct storage* storage, char* session_token);

/**
 * /user_name
 *   name of the user we'll look for
 * /error_message
 *   populated if we couldn't find the user, in which case the returned struct should be disregarded.
 * /return
 *   the user, if we found it. 0 struct otherwise.
 */ 
struct game_user find_user_by_name(struct storage* storage, char* user_name, char** error_message);

void random_string(char* string, uint len);