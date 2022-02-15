/**
 * Interface for making changes to users.
 */ 
#include "storage.h"

/**
 * /session_token
 *   Session token by which we'll find the user. If there isn't a user, we'll create one
 *   with a random name for anonymous users. Null terminated string.
 * /return
 *   either the existing user if they were found, or a new one
 */ 
struct game_user find_or_create_user_by_session(struct storage* storage, char* session_token);

