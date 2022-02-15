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

// Set to 1 once SIGINT is received.
// Used to control the main loop exit.
static sig_atomic_t interrupted = 0;

// Options for static http content serving.
// Later, the root directory might become configurable
static struct mg_http_serve_opts opts = {.root_dir = "public"};

static void callback(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
static void sighandle(int signal);

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

#define session_len 30
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
            mg_http_reply(c, 200, NULL, "Hello, signup");
        } else if (mg_http_match_uri(hm, "/login")) {
            mg_http_reply(c, 200, NULL, "Hello, login");
        } else if (mg_http_match_uri(hm, "/guess")) {
            // mg_http_get_var will append a null terminator
            // we need to account for that here.
            char the_guess[wordle_len+1] = {0};
            mg_http_get_var(&hm->body, "guess", the_guess, wordle_len+1); 
            char* error_message = NULL;
            if (!guess(storage, game_user, the_guess, &error_message)) {
                // TODO add a flash message, via cookie rather than bomb.
                mg_http_reply(c, 400, NULL, "Guess was invalid! %s", error_message);
            } else {
                mg_http_reply(c, 302, "Location: /\r\n", "redirecting...");
            }
        } else if (mg_http_match_uri(hm, "/")) {
            struct game_state game_state = todays_game(storage, game_user);
            char* rendered_page = render_index(game_state);
            size_t rendered_page_len = strlen(rendered_page);
            mg_printf(c, "HTTP/1.1 200 OK\r\n"
                "Content-Length: %d\r\n"
                "Content-Type: text/html\r\n"
                "Set-Cookie: session=%s; HttpOnly\r\n"
                "\r\n"
                "%.*s", rendered_page_len, session_token, rendered_page_len, rendered_page);
            free(rendered_page);
        } else {
            // serves static content
            mg_http_serve_dir(c, ev_data, &opts);
        }
    }
}
