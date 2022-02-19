/**
 * Common models used through the application
 * 
 */
#include <sys/types.h>
#include <stdbool.h>
#include <sodium.h>

// Length of words in the game
#define wordle_len 5

// Maximum guesses per round
#define max_turns 6

// Length of a date in format dd/mm/yyyy
#define date_len 10

/**
 * The word of the day! The thing that everyone wants to know :)
 * There's one wordle per day that players try to guess.
 */ 
struct wordle {
    char word[wordle_len];
    char date[date_len];
};

/**
 * State of a letter in a guess
 */ 
enum letter_state {
    // The letter is not present in the correct answertyp
    letter_state_incorrect,
    // The letter is in the correct answer, but not in it's current position
    letter_state_wrongpos,
    // The letter is present in this position in the correct answer
    letter_state_correct,
};

/**
 * An individual guess at a letter
 */ 
struct guess_letter {
    char letter;
    enum letter_state state;
};

/**
 * An indvidual guess at a word
 */ 
struct guess {
    struct guess_letter guess[wordle_len];
};

/**
 * Game state for a player's current day.
 */
struct game_state {
    // How many guesses have been submitted?
    uint turns_len;
    // The guesses that have been submitted already
    struct guess turns[max_turns];
};

#define max_name_len 30
#define max_pass_len 100
// defined by sodium.h
#define password_hash_len crypto_pwhash_STRBYTES
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

    // True for anonymous users, used to exclude from stats and control some features.
    bool anon;

    // argon2 hashed password for the user, and salt.
    char password_hash[password_hash_len];
};

/**
 * Represents a user logged into a browser.
 * Same user could be logged into multiple browsers, and those would have different sessions (I think?)
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

