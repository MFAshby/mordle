#include "storage.h"
#include "slog.h"
#include <libpq-fe.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

// in-memory storage impl
// only works for me
static struct game_state the_state;

static struct guess make_guess(struct wordle, char guess[wordle_len]);

static char* state_string(enum guess_letter_state ls);

struct storage* init_storage(int argc, char* argv[], char** error_message) {
    the_state.turns_len = 0;
    return NULL;
}

void free_storage(struct storage* storage) {}

struct wordle todays_answer(struct storage* storage) {
    struct wordle res = {
        .word = "cramp"
    };
    return res;
}

struct game_state todays_game(struct storage* storage, char* user_name, char** error_message) {
    if (strcmp(user_name, "martin") != 0) {
        *error_message = "invalid user!";
    }
    return the_state;
}

void save_guess(struct storage* storage, char* user_name, char guess[wordle_len]) {
    struct wordle wordle = todays_answer(storage);
    the_state.turns[the_state.turns_len++] = make_guess(wordle, guess);
}

/**
 * Compare the winning word to a guess and return the matching letters
 */ 
static struct guess make_guess(struct wordle wordle, char guess[wordle_len]) {
    struct guess res = {0};
    // Since we do two passes, we need to avoid overwriting the value from the first pass in the second
    // track which letter states we already set.
    bool state_is_set[wordle_len] = {false};

    // Use two passes, first pass finds exact matches
    for (uint i=0; i<wordle_len; i++) {
        enum guess_letter_state s = incorrect;
        if (wordle.word[i] == guess[i]) {
            s = correct;
            wordle.word[i] = '\0';
            state_is_set[i] = true;
        }
        struct guess_letter l = {
            .letter = guess[i],
            .state = s,
        };
        slogt("make_guess pass 1: letter %d state %s", i, state_string(l.state));
        res.guess[i] = l;
    }

    // Second pass finds misplaced letters
    for (uint i=0; i<wordle_len; i++) {
        // Avoid clobbering first pass results
        if (state_is_set[i]) {
            continue;
        }
        enum guess_letter_state s = incorrect;
        for (uint j=0; j<wordle_len; j++) {
            if (wordle.word[j] == guess[i]) {
                s = present_wrong_pos,
                wordle.word[j] = '\0';
            }
        }
        struct guess_letter l = {
            .letter = guess[i],
            .state = s,
        };
        slogt("make_guess pass 2: letter %d state %s", i, state_string(l.state));
        res.guess[i] = l;
    }
    return res;
}

static char* state_string(enum guess_letter_state ls) {
    switch (ls) {
        case incorrect:
            return "i";
        case present_wrong_pos:
            return "p";
        case correct:
            return "c";
    }
    return "unknown!";
}

void game_state_print(struct game_state state) {
    /*
    printf("%d turns\n", state.turns_len);
    for (uint i=0; i<state.turns_len; i++) {
        printf("turn %d: ", i+1);
        for (uint j=0; j<wordle_len; j++) {
            printf("[%c ", state.turns[i].guess[j].letter);
            printf("(%s)]", state_string(state.turns[i].guess[j].state));
        }
        printf("\n");
    }*/
}