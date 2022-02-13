#include "index.h"
#include "slog.h"
#include "index.html.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <mustach/mustach.h>

struct game_state_wrap;
static int mustach_itf_game_state_enter(void* closure, const char* name);
static int mustach_itf_game_state_leave(void* closure);
static int mustach_itf_game_state_get(void *closure, const char *name, struct mustach_sbuf *sbuf);
static int mustach_itf_game_state_next(void *closure);
/**
 * Uses mustach (https://gitlab.com/jobol/mustach) to render the template index.html.tpl.
 * 
 * Template is stored in a header file and embedded into the binary, generated with xxd, see Makefile.
 */ 
char* render_index(struct game_state game_state) {
    struct mustach_itf itf = {
        .enter = mustach_itf_game_state_enter,
        .leave = mustach_itf_game_state_leave,
        .get = mustach_itf_game_state_get,
        .next = mustach_itf_game_state_next
    };
    char* rendered_page;
    int err = mustach_mem((const char*)template_index_html_tpl, template_index_html_tpl_len, &itf, &game_state, 0, &rendered_page, NULL);
    if (err != 0) {
        sloge("error rendering page, %d, see mustach.h", errno);  
        return strdup("error rendering page!");
    } else {
        return rendered_page;
    }
}

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
};
static int mustach_itf_game_state_enter(void* closure, const char* name) {
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (strcmp(name, "guess") == 0) {
            state_wrapper->iter_guess = true;
            state_wrapper->guess_idx = 0;
            return 1;
        }
    } else {
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

static const char* guess_letter_state_desc(enum guess_letter_state ls) {
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
    struct game_state_wrap* state_wrapper = closure;
    if (state_wrapper->iter_turns) {
        if (state_wrapper->iter_guess) {
            struct guess_letter guess_letter = state_wrapper->game_state.turns[state_wrapper->turns_idx].guess[state_wrapper->guess_idx];
            if (strcmp(name, "state") == 0) {
                enum guess_letter_state s = guess_letter.state;
                const char* sd = guess_letter_state_desc(s);
                sbuf->value = sd;
                return 1;
            } else if (strcmp(name, "letter") == 0) {
                sbuf->value = &(guess_letter.letter);
                sbuf->length = 1;
                return 1;
            }
        }
    }
    return 0;
}

static int mustach_itf_game_state_next(void *closure) {
    // Careful! Check mustach.h for @next function. This should return 0 if there are  no more members to iterate, 
    // which means it's called _after_ we've finished with the current member. So, if we're on the highest index
    // then we should return false. this is why it compares (x-1)
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
