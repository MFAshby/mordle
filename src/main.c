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
 * Rendering gameplay pages
 * Routing form submits
 */ 

// Set to 1 once SIGINT is received.
// Used to control the main loop exit.
static sig_atomic_t interrupted = 0;

// Options for static http content serving.
// Later, the root directory might become configurable
static struct mg_http_serve_opts opts = {.root_dir = "public"};

/**
 * Mongoose event loop callback.
 * HTTP requests are received here.
 * See mongoose example code.
 */ 
static void callback(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
    struct storage* storage = fn_data;
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = ev_data;
        if (mg_http_match_uri(hm, "/signup")) {
            // Check method
            // Check form params
            // call handle_signup
            // if err, return err, 
            // else redirect to /
            mg_http_reply(c, 200, NULL, "Hello, signup");
        } else if (mg_http_match_uri(hm, "/login")) {
            // Check method
            // Check form params
            // call handle_login
            // if err, return err, 
            // else redirect to /
            mg_http_reply(c, 200, NULL, "Hello, login");
        } else if (mg_http_match_uri(hm, "/guess")) {
            // mg_http_get_var will append a null terminator
            // we need to account for that here.
            char the_guess[wordle_len+1] = {0};
            mg_http_get_var(&hm->body, "guess", the_guess, wordle_len+1); 

            char* error_message = NULL;
            // TODO get name from login
            if (!guess(storage, "martin", the_guess, &error_message)) {
                // TODO add a flash message, via cookie rather than bomb.
                mg_http_reply(c, 400, NULL, "Guess was invalid! %s", error_message);
            } else {
                mg_http_reply(c, 302, "Location: /\r\n", "redirecting...");
            }
        } else if (mg_http_match_uri(hm, "/")) {
            char* error_message = NULL;
            struct game_state game_state = todays_game(storage, "martin", &error_message);
            game_state_print(game_state);
            if (error_message != NULL) {
                mg_http_reply(c, 400, NULL, "Can't render page! %s", error_message);
            } else {
                char* rendered_page = render_index(game_state);
                size_t rendered_page_len = strlen(rendered_page);
                mg_printf(c, "HTTP/1.1 200 OK\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "%.*s", rendered_page_len, rendered_page_len, rendered_page);
            }
        } else {
            // serves static content
            mg_http_serve_dir(c, ev_data, &opts);
        }
    }
}

static void sighandle(int signal) {
    if (signal == SIGINT) {
        interrupted = 1;
    }
}

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
    return 0;
}
