/*
#pragma once
#include <sys/types.h>
#include <stdbool.h>
#include "storage.h"
*/
/**
 * User management code.
 */ 


/**
 * Creates a new user!
 * /storage
 *   Storage handle for game data
 * /name
 *   must be >0 and <=max_name_len characters, and unique per user.
 * /password
 *   no particular restriction here
 * /error_message
 *   Populated with an error message if the login failed for any reason *   
 * /return
 *   new user, if signup was successful. 
 *   Otherwise populates the string at errorMessage with an error.
 */
//struct user_and_session signup(struct storage* storage, char* name, char* password, char** error_message);

/**
 * Authenticates an existing user
 * /storage
 *   Storage handle for game data
 * /name 
 *   previously signed-up user's name
 * /password 
 *   previously supplied user's password
 * /error_message
 *   Populated with an error message if the login failed for any reason
 * /return
 *   the existing user with a new session, if login was successful.
 *   Otherwise error_message is populated
 */
//struct user_and_session login(struct storage* storage, char* name, char* password, char** error_message);
