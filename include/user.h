#pragma once
#include "storage.h"

/**
 * User management code.
 */ 

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
struct user {
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
    struct user user;
    struct session session;
};

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
struct user_and_session signup(struct storage* storage, char* name, char* password, char** error_message);

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
struct user_and_session login(struct storage* storage, char* name, char* password, char** error_message);
