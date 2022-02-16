#include "user.h"
#include "storage.h"
#include "slog.h"
#include <sodium/crypto_pwhash.h>
#include <sodium/randombytes.h>
#include <string.h>
#include <sodium.h>

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

void signup(struct storage* storage, struct game_user game_user, char* user_name, char* password, char* session_token, char** error_message) {
    slogd("user %s signing up", user_name);
    strncpy(game_user.name, user_name, max_name_len);
    game_user.anon = false;
    if (crypto_pwhash_str(game_user.password_hash, 
        password, 
        strlen(password),
        crypto_pwhash_opslimit_interactive(), 
        crypto_pwhash_memlimit_interactive()) != 0) {
        sloge("failed sign up!");
        *error_message = "failed to sign up!";
        goto end;
    }
    update_user(storage, game_user);
end:
    return;   
}

void login(struct storage* storage, struct game_user game_user, char* name, char* password, char* session_token, char** error_message) {
    struct game_user found_user = find_user_by_name(storage, name, error_message);
    if (error_message != NULL) {
        *error_message = "incorrect user or password!";
        goto end;
    }
    if (crypto_pwhash_str_verify(found_user.password_hash, password, strlen(password)) != 0) {
        *error_message = "incorrect user or password!";
        goto end;
    }
    // Update the session token to point at the found user.
    update_session_to_user(storage, game_user, session_token);
    // Maybe delete the current anon user for tidyness, they're now inaccessible?
    // Maybe copy over tonight's 
end:
    return;
}
