#include "storage.h"
#include "slog.h"
#include <libpq-fe.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static struct guess make_guess(struct wordle, char guess[wordle_len]);

static char* state_string(enum guess_letter_state ls);

struct storage {
    PGconn* conn;
};

struct storage* init_storage(int argc, char* argv[], char** error_message) {
    struct storage* res = malloc(sizeof(struct storage));
    // TODO config
    res->conn = PQconnectdb("postgresql://mordle:mordle@localhost:5432/mordle");
    return res;
}

void setup_test_storage(struct storage *storage) {
    // Setup today's word as 'cramp'
    // Setup one user called martin
    // Setup a wordlist with 
    PGconn* conn = storage->conn;
    PQexec(conn, "truncate table guess cascade");
    PQexec(conn, "truncate table game_user cascade");
    PQexec(conn, "truncate table answer cascade");
    PQexec(conn, "truncate table wordlist cascade");
    PQexec(conn, "insert into wordlist values ('cramp'), ('clamp'), ('stamp'),('aaaaa'),('spasm'),('fffff')");
    PQexec(conn, "insert into answer values (now()::date, 'cramp')");
    PQexec(conn, "insert into game_user(name) values ('martin')");
}

void free_storage(struct storage* storage) {
    PQfinish(storage->conn);
    free(storage);
}

struct wordle todays_answer(struct storage* storage) {
    PGresult* qr = PQexec(storage->conn, "select word from answer where answer_date = now()::date");
    // TODO check for error response
    char* word = PQgetvalue(qr, 0, 0);
    struct wordle res = {0};
    memcpy(&res.word, word, wordle_len);
    return res;
}

struct game_state todays_game(struct storage* storage, char* user_name, char** error_message) {
    PGresult* qr_user = NULL, *qr_turns = NULL;
    struct game_state game_state = {0};

    const char* param_values_user_name[] = {user_name};
    qr_user = PQexecParams(storage->conn, 
        "select id from game_user where name = $1", 1, NULL, param_values_user_name, NULL, NULL, 0);
    if (PQntuples(qr_user) == 0) {
        *error_message = "invalid user!";
        goto end;
    }
    char* user_id = PQgetvalue(qr_user, 0, 0);

    struct wordle wordle = todays_answer(storage);

    const char* param_values_user_id[] = {user_id};
    qr_turns = PQexecParams(storage->conn, 
        "select g.word "
        "from guess g "
        "where game_user_id = $1 "
        "and g.answer_date = now()::date  "
        "order by g.idx;",
        1, NULL, param_values_user_id,NULL,NULL,0);
    // TODO check for error response
    int turns_len = PQntuples(qr_turns);
    game_state.turns_len = (uint)turns_len;
    for (int i=0; i<turns_len; i++) {
        game_state.turns[i] = make_guess(wordle, PQgetvalue(qr_turns, i, 0));
    }
end:
    PQclear(qr_user);
    PQclear(qr_turns);
    return game_state;
}

void save_guess(struct storage* storage, char* user_name, char guess[wordle_len]) {
    PGresult* qr_user = NULL, *qr_new_idx = NULL;
    PGconn* conn = storage->conn;
    const char* param_values_user_name[] = {user_name};
    qr_user = PQexecParams(conn, 
        "select id from game_user where name = $1", 1, NULL, param_values_user_name, NULL, NULL, 0);
    if (PQntuples(qr_user) == 0) {
        goto end;
    }
    char* user_id = PQgetvalue(qr_user, 0, 0);
    const char* param_values_max_idx[] = {user_id};
    qr_new_idx = PQexecParams(conn, 
        "select coalesce(max(idx), 1)+1 from guess where answer_date = now()::date  and game_user_id = $1", 1, NULL, param_values_max_idx, NULL, NULL, 0);
    char* new_idx_value = PQgetvalue(qr_new_idx, 0, 0);
    const char* param_values_guess_insert[] = {user_id, guess, new_idx_value};
    PQexecParams(conn, 
        "insert into guess (game_user_id, answer_date, word, idx) values ($1, now()::date, $2, $3)", 3, NULL, param_values_guess_insert, NULL, NULL, 0);
end:
    PQclear(qr_user);
    PQclear(qr_new_idx);
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
