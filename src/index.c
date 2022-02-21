#include "index.h"
#include "game.h"
#include "slog.h"
#include "index.html.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <mustach/mustach.h>

/**
 * Wraps up the game_state with couters (_idx) to indicate mustach's progress through the structure.
 * 
 * I'm not completely sure of the best way to indicate indexes in sub-arrays. There doesn't seem to be a 
 * way in mustach to replace the closure with a new one for nested structures.
 */ 
struct index_data_wrap {
    struct game_state game_state;
    struct game_user game_user;
    struct leaderboard_top10 leaderboard;

    struct wordle wordle;
    char flash[max_flash];
    
    // Are we iterating game_state.turns?
    bool iter_turns;
    // Index into game_state.turns
    uint turns_idx;

    // Are we iterating game_state.turns[x].guess?
    bool iter_guess;
    // index into game_state.turns[x].guess
    uint guess_idx;

    // Are we iterating leaderboard.entries?
    bool iter_leaderboard;
    // index into leaderboard.entries
    uint leaderboard_idx;

    bool iter_won;
    bool iter_lost;

    bool iter_user;
    bool iter_anon;
};

static int mustach_itf_game_state_enter(void* closure, const char* name);
static int mustach_itf_game_state_leave(void* closure);
static int mustach_itf_game_state_get(void *closure, const char *name, struct mustach_sbuf *sbuf);
static int mustach_itf_game_state_next(void *closure);
/**
 * Uses mustach (https://gitlab.com/jobol/mustach) to render the template index.html.tpl.
 * 
 * Template is stored in a header file and embedded into the binary, generated with xxd, see Makefile.
 * 
 * String must be freed after use!
 */ 
char* render_index(struct game_state game_state, 
    struct game_user game_user, 
    struct wordle wordle, 
    struct leaderboard_top10 leaderboard,
    char flash[max_flash]) {

    struct mustach_itf itf = {
        .enter = mustach_itf_game_state_enter,
        .leave = mustach_itf_game_state_leave,
        .get = mustach_itf_game_state_get,
        .next = mustach_itf_game_state_next
    };
    struct index_data_wrap game_state_wrap = {
        .game_state = game_state,
        .game_user = game_user,
        .leaderboard = leaderboard,
        .wordle = wordle,
        .iter_turns = false,
        .turns_idx = 0,
        .iter_guess = false,
        .guess_idx = 0,
        .iter_leaderboard = false,
        .leaderboard_idx = 0
    };
    memcpy(&game_state_wrap.flash, flash, sizeof(char) * max_flash); // For some reason the literal init syntax doesn't like it.
    char* rendered_page;
    int err = mustach_mem((const char*)template_index_html_tpl, template_index_html_tpl_len, &itf, &game_state_wrap, 0, &rendered_page, NULL);
    if (err != 0) {
        sloge("error rendering page, %d, see mustach.h", errno);  
        return strdup("error rendering page!");
    } else {
        return rendered_page;
    }
}

/**
 * Mustach callback functions to move through the state
 */ 
static int mustach_itf_game_state_enter(void* closure, const char* name) {
    struct index_data_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (strcmp(name, "guess") == 0) { 
            state_wrapper->iter_guess = true;
            state_wrapper->guess_idx = 0;
            return 1;
        } else {
            sloge("unknown name %s", name);
        }
    } else if (state_wrapper->iter_user) {
        if (strcmp(name, "anon") == 0) {
            if (state_wrapper->game_user.anon) {
                state_wrapper->iter_anon = true;
                return 1;
            }
        } else {
            sloge("unknown name %s", name);
        }
    } else {
        if (strcmp(name, "turns") == 0) {
            if (state_wrapper->game_state.turns_len > 0){
                state_wrapper->iter_turns = true;
                state_wrapper->turns_idx = 0;
                return 1;
            } else {
                return 0;
            }
        } else if (strcmp(name, "won") == 0) {
            if (won(state_wrapper->game_state)) {
                state_wrapper->iter_won = true;
                return 1;
            }
        } else if (strcmp(name, "lost") == 0) {
            if (lost(state_wrapper->game_state)) {
                state_wrapper->iter_lost = true;
                return 1;
            }
        } else if (strcmp(name, "user") == 0) {
            state_wrapper->iter_user = true;
            return 1;
        } else if (strcmp(name, "leaderboard") == 0) {
            state_wrapper->iter_leaderboard = true;
            return 1;
        } else {
            sloge("unknown name %s", name);
        }
    }
    return 0;
}
static int mustach_itf_game_state_leave(void* closure) {
    struct index_data_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            state_wrapper->iter_guess = false;
            return 0;
        } else {
            state_wrapper->iter_turns = false;
            return 0;
        }
    } else if (state_wrapper->iter_won) {
        state_wrapper->iter_won = false;
    } else if (state_wrapper->iter_won) {
        state_wrapper->iter_won = false;
    } else if (state_wrapper->iter_user) {
        if (state_wrapper->iter_anon) {
            state_wrapper->iter_anon = false;
        } else {
            state_wrapper->iter_user = false;
        }
    } else if (state_wrapper->iter_leaderboard) {
        state_wrapper->iter_leaderboard = false;
    }
    return 0;
}

static const char* guess_letter_state_desc(enum letter_state ls) {
    switch (ls) {
        case letter_state_incorrect: 
        return "incorrect";
        case letter_state_wrongpos: 
        return "present_wrong_pos";
        case letter_state_correct:
        return "correct";
        default:
        return "unknown!";
    }
}

static int mustach_itf_game_state_get(void *closure, const char *name, struct mustach_sbuf *sbuf) {
    struct index_data_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            struct guess_letter guess_letter = state_wrapper->game_state.turns[state_wrapper->turns_idx].guess[state_wrapper->guess_idx];
            if (strcmp(name, "state") == 0) {
                enum letter_state s = guess_letter.state;
                const char* sd = guess_letter_state_desc(s);
                sbuf->value = sd;
                return 1;
            } else if (strcmp(name, "letter") == 0) {
                // This is required, because guess_letter is part of a struct that may go out 
                // of scope before the string is actually rendered. Maybe. Unsure about that.
                sbuf->value = strndup(&(guess_letter.letter), 1);
                sbuf->length = 1;
                sbuf->freecb = free;
                return 1;
            }
        }
    } else if (state_wrapper->iter_user) {
        if (strcmp(name, "name") == 0) {
            sbuf->value = strndup(state_wrapper->game_user.name, max_name_len);
            sbuf->freecb = free;
        }
    } else if (state_wrapper->iter_leaderboard) {
        uint idx = state_wrapper->leaderboard_idx;
        struct leaderboard_entry entry = state_wrapper->leaderboard.entries[idx];
        if (strcmp(name, "position") == 0) {
            char* b = malloc(sizeof(char) * 10);
            snprintf(b, 10, "%d", entry.position);
            sbuf->value = b;
            sbuf->freecb = free;
        } else if (strcmp(name, "name") == 0) {
            sbuf->value = strdup(entry.name);
            sbuf->freecb = free;
        } else if (strcmp(name, "average_score") == 0) {
            char* b = malloc(sizeof(char) * 10);
            snprintf(b, 10, "%.2f", entry.average_score);
            sbuf->value = b;
            sbuf->freecb = free;
        }
    } else {
        if (strcmp(name, "flash") == 0) {
            sbuf->value = strndup(state_wrapper->flash, max_flash);
            sbuf->freecb = free;   
        } else if (strcmp(name, "date") == 0) {
            sbuf->value = strndup(state_wrapper->wordle.date, date_len);
            sbuf->freecb = free;
        }
    }
    return 0;
}

static int mustach_itf_game_state_next(void *closure) {
    struct index_data_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            if ((state_wrapper->guess_idx+1) >= wordle_len) {
                return 0;
            } else {
                state_wrapper->guess_idx++;
                return 1;
            }
        } else {
            if ((state_wrapper->turns_idx+1) >= (state_wrapper->game_state.turns_len)) {
                return 0;
            } else {
                state_wrapper->turns_idx++;
                return 1;
            }
        }
    } else if (state_wrapper->iter_won) {
        return 0;
    } else if (state_wrapper->iter_lost)  {
        return 0;
    } else if (state_wrapper->iter_user) {
        if (state_wrapper->iter_anon)  {
            return 0;
        } 
        return 0;
    } else if (state_wrapper->iter_leaderboard) {
        if ((state_wrapper->leaderboard_idx+1) >= 10) {
            return 0;
        } else {
            state_wrapper->leaderboard_idx++;
            return 1;
        }
    }
    return 0;
}
