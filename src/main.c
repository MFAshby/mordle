#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <mustach/mustach.h>
#include <mongoose.h>

#include "slog.h"
#include "storage.h"
#include "game.h"
#include "user.h"
#include "index.html.h"

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
 * Structures and functions for iterating game_state via mustach template.
 */ 
struct game_state_wrap {
    struct game_state game_state;
    
    // Index into game_state.guesses
    bool iter_turns;
    uint turns_idx;

    // Index into game_state.guesses[guesses_idx].guess
    bool iter_guess;
    uint guess_idx;
};
static int mustach_itf_game_state_enter(void* closure, const char* name) {
    slogd("mustach_itf_game_state_enter %s", name);
    struct game_state_wrap* state_wrapper = closure;
    // Handle nested elements first
    if (state_wrapper->iter_turns) {
        if (strcmp(name, "guess") == 0) {
            state_wrapper->iter_guess = true;
            state_wrapper->guess_idx = 0;
            return 1;
        }
    } else {
        // Top level elements can be entered
        if (strcmp(name, "turns") == 0) {
            if (state_wrapper->game_state.turns_len > 0) {
                state_wrapper->iter_turns = true;
                state_wrapper->turns_idx = 0;
            }
            return 1;
        }
    }
    return 0;
}
static int mustach_itf_game_state_leave(void* closure) {
    slogd("mustach_itf_game_state_leave");
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            state_wrapper->iter_guess = false;
            return 1;
        } else {
            state_wrapper->iter_turns = false;
            return 1;
        }
    }
    return 0;
}

static const char* _guess_letter_state_desc(enum guess_letter_state ls) {
    switch (ls) {
        case incorrect: 
        return "incorrect";
        case present_wrong_pos: 
        return "present_wrong_pos";
        case correct:
        return "correct";
        default:
        return "unknown!";
    }
}

static int mustach_itf_game_state_get(void *closure, const char *name, struct mustach_sbuf *sbuf) {
    slogd("mustach_itf_game_state_get %s", name);
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            struct guess_letter guess_letter = state_wrapper->game_state.turns[state_wrapper->turns_idx].guess[state_wrapper->guess_idx];
            if (strcmp(name, "state") == 0) {
                enum guess_letter_state s = guess_letter.state;
                const char* sd = _guess_letter_state_desc(s);
                sbuf->value = sd;
                slogd("get state, returning %s", sbuf->value);
                return 1;
            } else if (strcmp(name, "letter") == 0) {
                sbuf->value = &(guess_letter.letter);
                sbuf->length = 1;
                slogd("get letter, returning %.*s", sbuf->length, sbuf->value);
                return 1;
            }
        }
    }
    return 0;
}
static int mustach_itf_game_state_next(void *closure) {
    slogd("mustach_itf_game_state_next");
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            if (state_wrapper->guess_idx < (wordle_len-1)) {
                state_wrapper->guess_idx++;
                return 1;
            } else {
                return 0;
            }
        } else {
            if (state_wrapper->turns_idx < (state_wrapper->game_state.turns_len-1)) {
                state_wrapper->turns_idx++;
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
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
                struct mustach_itf itf = {
                    .enter = mustach_itf_game_state_enter,
                    .leave = mustach_itf_game_state_leave,
                    .get = mustach_itf_game_state_get,
                    .next = mustach_itf_game_state_next
                };
                char* rendered_page;
                // size_t rendered_page_len;
                int err = mustach_mem((const char*)template_index_html_tpl, template_index_html_tpl_len, &itf, &game_state, 0, &rendered_page, NULL);
                if (err) {
                    mg_http_reply(c, 500, NULL, "error rendering template %d", err);
                } else {
                    size_t rendered_page_len = strlen(rendered_page);
                    mg_printf(c, "HTTP/1.1 200 OK\r\n"
                        "Content-Length: %d\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "%.*s", rendered_page_len, rendered_page_len, rendered_page);
                }
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
