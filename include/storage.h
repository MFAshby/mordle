#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <sodium.h>
#include "models.h"

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

/**
 * Populate the storage with test data.
 */ 
void setup_test_storage(struct storage* storage);

/**
 * Relinquish the storage once it's finished, e.g. by closing postgres connections
 * or closing sqlite file.
 */ 
void free_storage(struct storage* storage);

/**
 * Return today's word from the winning wordlist.
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
 * /user_name
 *   name of the user we'll look for
 * /error_message
 *   populated if we couldn't find the user, in which case the returned struct should be disregarded.
 * /return
 *   the user, if we found it. 0 struct otherwise.
 */ 
struct game_user find_user_by_name(struct storage* storage, char* user_name, char** error_message);

/**
 * /session_token
 * /error_message
 *   populated if we couldn't find the user, in which case the returned struct should be disregarded.
 * /return 
 *   the user, if we found it. 0 struct otherwise.
 */ 
struct game_user find_user_by_session(struct storage* storage, char* session_token, char** error_message);

/**
 * /game_user
 *   The new user to save.
 * /session_token
 *   The session token to save.
 * /return
 *   The user, updated with database assigned ID value.
 * This function has no expected error cases.
 */ 
struct game_user save_user_and_session(struct storage* storage, struct game_user game_user, char* session_token);

/**
 * Re-points a session to the user indicated by game_user.
 * This leaves an orphaned anonymous user, oh well. We'll clean those up.
 * /game_user 
 *   the user to update
 */ 
void update_session_to_user(struct storage* storage, struct game_user game_user, char* session_token);
/**
 * Deletes a session.
 * /session_token
 *   the token of the session.
 */
void delete_session_by_token(struct storage* storage, char* session_token);
/**
 * Update the given user on the database.
 * ID can't change, but everything else can.
 * 
 * /game_user 
 *   The user to update, with new fields set.
 */ 
void update_user(struct storage* storage, struct game_user);

void random_string(char* string, uint len);