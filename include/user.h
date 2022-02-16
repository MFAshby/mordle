/**
 * Interface for making changes to users.
 */ 
#include "storage.h"

/**
 * Everyone on the site gets a game_user record created automatically and a session_token set.
 * If you don't already have one, the first page load gives you an anonymous user.
 * 
 * /session_token
 *   Session token by which we'll find the user. If there isn't a user, we'll create one
 *   with a random name for anonymous users. Null terminated string.
 * /return
 *   either the existing user if they were found, or a new one with a generated name
 */ 
struct game_user find_or_create_user_by_session(struct storage* storage, char* session_token);

/**
 * TODO should you distinguish between anonymous users and named ones? 
 * A. yes, because 
 * 1. anons shouldn't appear in the leaderboard and 
 * 2. you should only show 'login' for anon users, and 'logout' for named ones.
 * 
 * Turn an anonymous user to a named one
 * /game_user
 *   The current & possibly anonymous user
 */ 
void signup(struct storage* storage, struct game_user game_user, char* name, char* password, char* session_token, char** error_message);

/**
 * Re-associates the session with the desired user, if the supplied credentials are correct.
 * 
 * /game_user 
 *   The current (possibly anonymous) user.
 * /name
 *   The name of the user we're trying to login as 
 * /password
 *   The password of the user we're trying to login as
 * /session_token
 *   The current session token
 */ 
void login(struct storage* storage, struct game_user game_user, char* name, char* password, char* session_token, char** error_message);


