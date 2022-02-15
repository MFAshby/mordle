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
struct game_state_wrap {
    struct game_state game_state;
    
    // Are we iterating game_state.turns?
    bool iter_turns;
    // Index into game_state.turns
    uint turns_idx;

    // Are we iterating game_state.turns[x].guess?
    bool iter_guess;
    // index into game_state.turns[x].guess
    uint guess_idx;

    bool iter_won;
    bool iter_lost;
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
char* render_index(struct game_state game_state) {
    struct mustach_itf itf = {
        .enter = mustach_itf_game_state_enter,
        .leave = mustach_itf_game_state_leave,
        .get = mustach_itf_game_state_get,
        .next = mustach_itf_game_state_next
    };
    struct game_state_wrap game_state_wrap = {
        .game_state = game_state,
        .iter_turns = false,
        .turns_idx = 0,
        .iter_guess = false,
        .guess_idx = 0,
    };
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
    slogt("mustach_itf_game_state_enter %s", name);
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (strcmp(name, "guess") == 0) {    
            slogt("iter_guess -> true");
            slogt("guess_idx -> 0");
            state_wrapper->iter_guess = true;
            state_wrapper->guess_idx = 0;
            return 1;
        } else {
            sloge("unknown name %s", name);
        }
    } else {
        if (strcmp(name, "turns") == 0) {
            if (state_wrapper->game_state.turns_len > 0){
                slogt("iter_turns -> true");
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
        } else {
            sloge("unknown name %s", name);
        }
    }
    return 0;
}
static int mustach_itf_game_state_leave(void* closure) {
    slogt("mustach_itf_game_state_leave");
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            slogt("left guess");
            state_wrapper->iter_guess = false;
            return 0;
        } else {
            state_wrapper->iter_turns = false;
            slogt("left turns");
            return 0;
        }
    } else if (state_wrapper->iter_won) {
        state_wrapper->iter_won = false;
    } else if (state_wrapper->iter_won) {
        state_wrapper->iter_won = false;
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
    slogt("mustach_itf_game_state_get %s", name);
    struct game_state_wrap* state_wrapper = closure;
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
    }
    return 0;
}

static int mustach_itf_game_state_next(void *closure) {
    slogt("mustach_itf_game_state_next");
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            if ((state_wrapper->guess_idx+1) >= wordle_len) {
                slogt("finished iter_guess at index %d", state_wrapper->guess_idx);
                return 0;
            } else {
                state_wrapper->guess_idx++;
                return 1;
            }
        } else {
            if ((state_wrapper->turns_idx+1) >= (state_wrapper->game_state.turns_len)) {
                slogt("finished iter_turns at index %d", state_wrapper->turns_idx);
                return 0;
            } else {
                state_wrapper->turns_idx++;
                slogt("turns_idx -> %d", state_wrapper->turns_idx);
                return 1;
            }
        }
    } else if (state_wrapper->iter_won) {
        return 0;
    } else if (state_wrapper->iter_lost)  {
        return 0;
    }
    return 0;
}
