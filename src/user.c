#include "user.h"
#include "storage.h"
#include <string.h>

struct game_user find_or_create_user_by_session(struct storage* storage, char* session_token) {
    char* error_message = NULL;
    struct game_user game_user = find_user_by_session(storage, session_token, &error_message);
    if (error_message == NULL) {
        return game_user;
    }
    char random_name[max_name_len] = {0};
    random_string(random_name, 5);
    sprintf(random_name+5, "-anon");
    struct game_user new_user = {.id = 0};
    strcpy(new_user.name, random_name);
    return save_user_and_session(storage, new_user, session_token);
}