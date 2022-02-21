#include <sodium/randombytes.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <mongoose.h>

#include "slog.h"
#include "storage.h"
#include "game.h"
#include "user.h"
#include "index.h"

/**
 * main.c handles the web parts of the game:
 * Starting an http server
 * Routing form submits
 */ 

// we'll just use a simple session mechanism
#define session_len 30

// Set to 1 once SIGINT is received.
// Used to control the main loop exit.
static sig_atomic_t interrupted = 0;

// Options for static http content serving.
// Later, the root directory might become configurable
static struct mg_http_serve_opts opts = {.root_dir = "public"};

static void callback(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
static void sighandle(int signal);
static void redirect_to_root_with_flash(struct mg_connection* c,
                                        char error_message[max_flash]);
static void do_signup_or_login(struct storage* storage,
                                struct mg_connection* c,
                                struct mg_http_message* hm,
                                char session_token[session_len],
                                struct game_user game_user,
                                void(*fn)(struct storage* storage, struct game_user game_user, char* user_name, char* password, char* session_token, char** error_message));

int main(int argc, char* argv[]) {
    slog_init(NULL, SLOG_FATAL | SLOG_ERROR | SLOG_WARN | SLOG_NOTE | SLOG_INFO | SLOG_DEBUG, 0);
    slogd("starting");

    signal(SIGINT, sighandle);

    char* error_message = NULL;
    struct storage* storage = init_storage(argc, argv, &error_message);
    if (error_message != NULL) {
        slogf("failed to initialize storage %s", error_message);
        return 1;
    }

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://localhost:8080", callback, storage);
    while (!interrupted) {
        mg_mgr_poll(&mgr, 1000);
    }
    slogd("shutdown");
    mg_mgr_free(&mgr);
    free_storage(storage);
    return 0;
}

static void sighandle(int signal) {
    if (signal == SIGINT) {
        interrupted = 1;
    }
}

/**
 * Mongoose event loop callback.
 * HTTP requests are received here.
 * See mongoose example code.
 */ 
static void callback(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
    struct storage* storage = fn_data;
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = ev_data;

        // Check for / set a session cookie, 30 random chars, plus a null terminator
        struct mg_str* cookie = mg_http_get_header(hm, "Cookie");
        char session_token[session_len+1] = {0};
        session_token[session_len] = '\0';
        if (cookie != NULL) {
            struct mg_str session_var = mg_http_get_header_var(*cookie, mg_str("session"));
            if (session_var.len == session_len) {
                memcpy(session_token, session_var.ptr, session_len);
            }
        } 
        if (strlen(session_token) == 0) {
            random_string(session_token, session_len);
        }
        struct game_user game_user = find_or_create_user_by_session(storage, session_token);
        
        if (mg_http_match_uri(hm, "/signup")) {
            do_signup_or_login(storage, c, hm, session_token, game_user, signup);                       
        } else if (mg_http_match_uri(hm, "/login")) {
            do_signup_or_login(storage, c, hm, session_token, game_user, login);
        } else if (mg_http_match_uri(hm, "/logout")) {
            logout(storage, session_token);
            redirect_to_root_with_flash(c, "logged out");
        } else if (mg_http_match_uri(hm, "/guess")) {
            // mg_http_get_var will append a null terminator
            // we need to account for that here.
            char the_guess[wordle_len+1] = {0};
            mg_http_get_var(&hm->body, "guess", the_guess, wordle_len+1); 
            char* error_message = "";
            guess(storage, game_user, the_guess, &error_message);
            redirect_to_root_with_flash(c, error_message);
        } else if (mg_http_match_uri(hm, "/")) {
            char error_message[max_flash] = {'\0'};
            if (cookie != NULL) {
                struct mg_str flash_var = mg_http_get_header_var(*cookie, mg_str("flash"));
                if (flash_var.len > 0) {
                    mg_url_decode(flash_var.ptr, flash_var.len, error_message, max_flash, 0);
                }
            }
            struct wordle wordle = todays_answer(storage);
            struct game_state game_state = todays_game(storage, game_user);
            struct leaderboard_top10 lb = {0};
            get_leaderboard(storage, lb.entries, 10, -6, 10);
            char* rendered_page = render_index(game_state, game_user, wordle, lb, error_message);
            size_t rendered_page_len = strlen(rendered_page);
            mg_printf(c, "HTTP/1.1 200 OK\r\n"
                "Content-Length: %d\r\n"
                "Content-Type: text/html\r\n"
                "Set-Cookie: session=%s; SameSite=Strict; HttpOnly\r\n"
                "Set-Cookie: flash=; SameSite=Strict; HttpOnly\r\n"
                "\r\n"
                "%.*s", rendered_page_len, session_token, rendered_page_len, rendered_page);
            free(rendered_page);
        } else {
            // serves static content
            mg_http_serve_dir(c, ev_data, &opts);
        }
    }
}

static void redirect_to_root_with_flash(struct mg_connection* c,
                                        char error_message[max_flash]) {
    char flash_cookie[max_flash] = {'\0'};
    if (error_message != NULL) {
        mg_url_encode(error_message, strlen(error_message), flash_cookie, max_flash);
    }
    mg_printf(c, "HTTP/1.1 302 Found\r\n"
        "Content-Length: 0\r\n"
        "Location: /\r\n"
        "Set-Cookie: flash=%s; SameSite=Strict; HttpOnly\r\n"
        "\r\n\r\n", flash_cookie);
}

static void do_signup_or_login(struct storage* storage,
                                struct mg_connection* c,
                                struct mg_http_message* hm,
                                char session_token[session_len],
                                struct game_user game_user,
                                void(*fn)(struct storage* storage, struct game_user game_user, char* user_name, char* password, char* session_token, char** error_message)) {
    char user_name[max_name_len+1] = {0};
    char password[max_pass_len+1] = {0};
    mg_http_get_var(&hm->body, "user_name", user_name, max_name_len+1);
    mg_http_get_var(&hm->body, "password", password, max_pass_len+1);
    char* error_message = NULL;
    fn(storage, game_user, user_name, password, session_token, &error_message);
    redirect_to_root_with_flash(c, error_message);
}