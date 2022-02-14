#pragma once
#include <sys/types.h>
#include <stdbool.h>

#include "storage.h"

/**
 * Gameplay code!
 */
/**
 * Submit a guess in today's game
 * 
 * /storage
 *   Storage handle for game data
 * /game_user
 *   The user who is submitting
 * /guess
 *   Their guess
 * /error_message
 *   Variable is populated with an error message if the guess was invalid
 *   note that an _incorrect_ guess isn't invalid, just wrong :)
 * /return 
 *   true if the guess was valid and accepted,
 *   false if the guess was invalid
 */
bool guess(struct storage* storage, struct game_user game_user, char* guess, char** error_message);

/**
 * Check if a game has been won
 */ 
bool won(struct game_state state);

/**
 * Check if a game has been lost
 */ 
bool lost(struct game_state state);