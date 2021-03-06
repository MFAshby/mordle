#include "storage.h"
#include "slog.h"
#include <libpq-fe.h>
#include <postgres_ext.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

static struct guess make_guess(struct wordle, char guess[wordle_len]);

static char* state_string(enum letter_state ls);

static PGresult* PQexec_retry(PGconn *conn, const char *query);

static PGresult* PQexecParams_retry(PGconn *conn,
                              const char *command,
                              int nParams,
                              const Oid *paramTypes,
                              const char *const *paramValues,
                              const int *paramLengths,
                              const int *paramFormats,
                              int resultFormat);

struct storage {
    PGconn* conn;
};

struct storage* init_storage(int argc, char* argv[], char** error_message) {
    struct storage* res = malloc(sizeof(struct storage));
    // TODO config
    res->conn = PQconnectdb("postgresql://mordle:mordle@localhost:5432/mordle");
    if (PQstatus(res->conn) != CONNECTION_OK) {
        slogf("failed connecting to postgres %s", PQerrorMessage(res->conn));
        *error_message = PQerrorMessage(res->conn);
    }
    return res;
}

void setup_test_storage(struct storage *storage) {
    // Setup today's word as 'cramp'
    // Setup one user called martin
    // Setup a wordlist with a few words
    PGconn* conn = storage->conn;
    PQclear(PQexec_retry(conn, "truncate table session cascade"));
    PQclear(PQexec_retry(conn, "truncate table guess cascade"));
    PQclear(PQexec_retry(conn, "truncate table game_user cascade"));
    PQclear(PQexec_retry(conn, "truncate table answer cascade"));
    PQclear(PQexec_retry(conn, "truncate table wordlist cascade"));
    PQclear(PQexec_retry(conn, "insert into wordlist values ('cramp'), ('clamp'), ('stamp'),('aaaaa'),('spasm'),('fffff')"));
    PQclear(PQexec_retry(conn, "insert into answer values (now()::date, 'cramp')"));
    PQclear(PQexec_retry(conn, "insert into game_user(name) values ('martin')"));
}

void free_storage(struct storage* storage) {
    PQfinish(storage->conn);
    free(storage);
}

bool in_wordlist(struct storage* storage, char* word) {
    bool res = false;
    const char* param_values[] = {word};
    PGresult* qr = PQexecParams_retry(storage->conn, "select 1 from wordlist where word = $1", 
        1, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("error checking wordlist %s", PQresultErrorMessage(qr));
        goto end;
    }
    res = PQntuples(qr) > 0;
end:
    PQclear(qr);
    return res;
}

struct wordle todays_answer(struct storage* storage) {
    struct wordle res = {0};
    PGresult* qr = PQexec_retry(storage->conn, "select word, to_char(answer_date,'DD/MM/YYYY') from answer where answer_date = now()::date");
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("error selecting user %s", PQresultErrorMessage(qr));
        goto end;
    }

    if (PQntuples(qr) == 0) {
        sloge("No word of the day found!");
        memcpy(&res.word, "cramp", wordle_len);
        memcpy(&res.date, "19/02/2021", date_len);
    } else {
        memcpy(&res.word, PQgetvalue(qr, 0, 0), wordle_len);
        memcpy(&res.date, PQgetvalue(qr, 0, 1), date_len);
    }

end:
    PQclear(qr);
    return res;
}

struct game_state todays_game(struct storage* storage, struct game_user game_user) {
    struct game_state game_state = {0};

    // Track results we need to close later
    PGresult *qr;

    struct wordle wordle = todays_answer(storage);

    // TODO calculate max length of bigint on the database and rsultig string in base 10
    char game_user_id_str[10];
    snprintf(game_user_id_str, 10, "%d", game_user.id);

    const char* param_values_user_id[] = {game_user_id_str};
    qr = PQexecParams_retry(storage->conn, 
        "select g.word "
        "from guess g "
        "where game_user_id = $1 "
        "and g.answer_date = now()::date  "
        "order by g.idx;",
        1, NULL, param_values_user_id,NULL,NULL,0);
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("error selecting user %s", PQresultErrorMessage(qr));
        goto end;
    }
    int turns_len = PQntuples(qr);
    game_state.turns_len = (uint)turns_len;
    for (int i=0; i<turns_len; i++) {
        game_state.turns[i] = make_guess(wordle, PQgetvalue(qr, i, 0));
    }
end:
    PQclear(qr);
    return game_state;
}

void save_guess(struct storage* storage, struct game_user game_user, char guess[wordle_len]) {
    PGresult* qr = NULL, * to_clear[5] = {0};
    uint to_clear_idx = 0;
    PGconn* conn = storage->conn;
    char user_id_str[10];
    snprintf(user_id_str, 10, "%d", game_user.id);
    const char* param_values_max_idx[] = {user_id_str};
    qr = PQexecParams_retry(conn, 
        "select coalesce(max(idx), 1)+1 from guess "
        "where answer_date = now()::date "
        "and game_user_id = $1", 1, NULL, param_values_max_idx, NULL, NULL, 0);
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("failed to get next guess index %s", PQresultErrorMessage(qr));
        goto end;
    }
    
    char* new_idx_value = PQgetvalue(qr, 0, 0);
    const char* param_values_guess_insert[] = {user_id_str, guess, new_idx_value};
    qr = PQexecParams_retry(conn, 
        "insert into guess (game_user_id, answer_date, word, idx) "
        "values ($1, now()::date, $2, $3)", 3, NULL, param_values_guess_insert, NULL, NULL, 0);
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("failed to save guess %s", PQresultErrorMessage(qr));
        goto end;
    }
end:
    for (uint i=0; i<to_clear_idx; i++) {
        PQclear(to_clear[i]);
    }
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
        enum letter_state s = letter_state_incorrect;
        if (wordle.word[i] == guess[i]) {
            s = letter_state_correct;
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
        enum letter_state s = letter_state_incorrect;
        for (uint j=0; j<wordle_len; j++) {
            if (wordle.word[j] == guess[i]) {
                s = letter_state_wrongpos,
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

static char* state_string(enum letter_state ls) {
    switch (ls) {
        case letter_state_incorrect:
            return "i";
        case letter_state_wrongpos:
            return "p";
        case letter_state_correct:
            return "c";
    }
    return "unknown!";
}


struct game_user find_user_by_session(struct storage* storage, char* session_token, char** error_message) {
    struct game_user res = {0};
    PGresult* qr = NULL;
    PGconn* conn = storage->conn;
    const char* param_values[] = {session_token};
    qr = PQexecParams_retry(conn, 
        "select id, name, anon, password_hash "
        "from game_user g "
            "join session s on s.game_user_id = g.id "
            "and s.session_token = $1", 1, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("error selecting user %s", PQresultErrorMessage(qr));
        goto end;
    }
    if (PQntuples(qr) == 1) {
        res.id = atoi(PQgetvalue(qr, 0, 0));
        snprintf(res.name, max_name_len, "%s", PQgetvalue(qr, 0, 1));
        res.anon = *PQgetvalue(qr, 0, 2) == 't';
        snprintf(res.password_hash, password_hash_len, "%s", PQgetvalue(qr, 0, 3));
    } else {
        *error_message = "user not found for session";
    }
end:
    PQclear(qr);
    return res;
}

struct game_user save_user_and_session(struct storage* storage, struct game_user game_user, char* session_token) {
    PGresult* qr, * to_clear[10] = {0};
    uint to_clear_idx = 0;
    PGconn* conn = storage->conn;
    bool did_commit = false;

    qr = PQexec_retry(conn, "begin");
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("failed to begin %s", PQresultErrorMessage(qr));
        goto end;
    }
    const char* param_values_insert_user[] = {game_user.name};
    qr = PQexecParams_retry(conn, 
        "insert into game_user (name) "
        "values ($1) returning id", 1, NULL, param_values_insert_user, NULL, NULL, 0);
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("failed to insert new user %s", PQresultErrorMessage(qr));
        goto end;
    }

    char* id_str = PQgetvalue(qr, 0, 0);
    const char* param_values_insert_sess[] = {id_str, session_token};
    qr = PQexecParams_retry(conn, 
        "insert into session (game_user_id, session_token) "
        "values ($1, $2)", 2, NULL, param_values_insert_sess, NULL, NULL, 0);
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("failed to insert new session %s", PQresultErrorMessage(qr));
        goto end;
    }
    qr = PQexec_retry(conn, "commit");
    to_clear[to_clear_idx++] = qr;
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("failed to commit %s", PQresultErrorMessage(qr));
        goto end;
    }
    // everything was good, let's return our updated user.
    did_commit = true;
    game_user.id = atoi(id_str);
end:
    if (!did_commit) {
        PQclear(PQexec_retry(conn, "rollback"));
    }
    for (uint i=0; i<to_clear_idx; i++) {
        PQclear(to_clear[i]);
    }
    return game_user;
}

struct game_user find_user_by_name(struct storage* storage, char* user_name, char** error_message) {
    struct game_user res = {0};
    PGconn* conn = storage->conn;
    const char* param_values[] = {user_name};
    PGresult* qr = PQexecParams_retry(conn, 
        "select id, name, anon, password_hash "
        "from game_user g "
        "where name = $1", 1, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("failed to find user by name %s", PQresultErrorMessage(qr));
        *error_message = "error finding user!";
        goto end;
    }
    if (PQntuples(qr)) {
        res.id = atoi(PQgetvalue(qr, 0, 0));
        snprintf(res.name, max_name_len, "%s", PQgetvalue(qr, 0, 1));
        res.anon = *PQgetvalue(qr, 0, 2) == 't';
        snprintf(res.password_hash, password_hash_len, "%s", PQgetvalue(qr, 0, 3));
    } else {
        slogi("user %s not found", user_name);
        *error_message = "invalid user!";
    }
end:
    PQclear(qr);
    return res;
}

/**
 * Used for mapping random bytes -> ascii string.
 * Could use more characters if bothered.
 */ 
static const char* charset = "abcdefghijklmnopqrstuvwxyzABCEDFGHIJKLMNOPQRSTUVWXYZ";
static uint charset_len = 52;

void random_string(char* string, uint len) {
    char buf[len];
    randombytes_buf(buf, len * (sizeof(char)));
    for (uint i=0; i<len; i++) {
        string[i] = charset[buf[i] % charset_len];
    }
}

void update_session_to_user(struct storage* storage, struct game_user game_user, char* session_token) {
    PGconn* conn = storage->conn;
    char id_str[10];
    snprintf(id_str, 10, "%d", game_user.id);
    const char* param_values[] = {id_str, session_token};
    PGresult* qr = PQexecParams_retry(conn, "update session set game_user_id = $1 where session_token = $2", 2, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("error updating session to user %s", PQresultErrorMessage(qr));
    }
    PQclear(qr);
}

void update_user(struct storage* storage, struct game_user game_user) {
    PGconn* conn = storage->conn;
    char id_str[10], anon_str[10];
    snprintf(id_str, 10, "%d", game_user.id);
    snprintf(anon_str, 10, "%d", game_user.anon);
    const char* param_values[] = {game_user.name, game_user.password_hash, anon_str, id_str};
    PGresult* qr = PQexecParams_retry(conn, "update game_user set name = $1, password_hash = $2, anon = $3 where id = $4", 4, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("error updating user %s", PQresultErrorMessage(qr));
    }
    PQclear(qr);    
}

void delete_session_by_token(struct storage* storage, char* session_token) {
    const char* param_values[] = {session_token};
    PGconn* conn = storage->conn;
    PGresult* qr = PQexecParams_retry(conn, "delete from session where session_token = $1", 1, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("error updating session to user %s", PQresultErrorMessage(qr));
    }
    PQclear(qr);
}

void save_game_result(struct storage* storage, struct game_user game_user, bool won, uint score) {
    PGconn* conn = storage->conn;
    char user_id_buf[10], score_buf[10];
    snprintf(user_id_buf, 10, "%d", game_user.id);
    snprintf(score_buf, 10, "%d", score);
    char* won_buf = won ? "t" : "f";
    const char* param_values[] = {user_id_buf, won_buf, score_buf};
    PGresult* qr = PQexecParams_retry(conn, "insert into game_result (game_user_id, answer_date, win, score) "
        "values ($1, now()::date, $2, $3)", 
        3, NULL, param_values, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_COMMAND_OK) {
        sloge("error saving game result %s", PQresultErrorMessage(qr));
    }
    PQclear(qr);
}

void get_leaderboard(struct storage* storage, 
    struct leaderboard_entry* fill, 
    uint leaderboard_count, 
    int days_back,
    uint incomplete_score) {
    PGconn* conn = storage->conn;
    char b1[10], b2[10], b3[10];
    snprintf(b1, 10, "%d", incomplete_score);
    snprintf(b2, 10, "%d", days_back);
    snprintf(b3, 10, "%d", leaderboard_count);
    const char* params[] = {b1, b2, b3};
    PGresult* qr = PQexecParams_retry(conn, 
        "with scores as ( "
        "select "
            "gu.id as game_user_id, "
            "gu.name as name, "
            "(now()::date+g.day) as answer_date, "
            "coalesce(gr.score,$1) as score "
        "from game_user gu "
        "cross join generate_series($2, 0, 1) as g(day) "
        "left outer join game_result gr "
            "on gr.game_user_id = gu.id "
            "and gr.answer_date = (now()::date+g.day)) "
        "select row_number() over(order by avg(score)) as position, "
            "game_user_id, name, "
            "avg(score) as avg_score "
        "from scores "
        "group by game_user_id, name "
        "order by position "
        "limit $3; ",
        3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(qr) != PGRES_TUPLES_OK) {
        sloge("error fetching leaderboard %s", PQresultErrorMessage(qr));
        goto end;
    }
    int n = PQntuples(qr);
    for (uint i=0; i<n; i++) {
        fill[i].position = atoi(PQgetvalue(qr, i, 0));
        fill[i].game_user_id = atoi(PQgetvalue(qr, i, 1));
        strncpy(fill[i].name, PQgetvalue(qr, i, 2), max_name_len);
        fill[i].average_score = atof(PQgetvalue(qr, i, 3));
    }
end:    
    PQclear(qr);
}

/**
 * LibPQ won't automatically retry a query itself if a connection went bad.
 * Wrap the exec functions so that we retry in case we lost connection to postgresql.
 * 
 * This happens if e.g. postgresql server restarts.
 */ 
static PGresult* PQexec_retry(PGconn *conn, const char *query) {
    PGresult* qr = PQexec(conn, query);
    ExecStatusType result_status = PQresultStatus(qr);
    if (result_status == PGRES_TUPLES_OK || result_status == PGRES_COMMAND_OK) {
        return qr;
    }
    ConnStatusType status = PQstatus(conn);
    if (status != CONNECTION_BAD) {
        return qr;
    }
    slogd("connection was bad, retrying");
    PQclear(qr);
    PQreset(conn);
    usleep(100000);
    return PQexec_retry(conn, query);
}

static PGresult* PQexecParams_retry(PGconn *conn,
                              const char *command,
                              int nParams,
                              const Oid *paramTypes,
                              const char *const *paramValues,
                              const int *paramLengths,
                              const int *paramFormats,
                              int resultFormat) {
    PGresult* qr = PQexecParams(conn, command, nParams, paramTypes, paramValues, paramLengths, paramFormats, resultFormat);
    ExecStatusType result_status = PQresultStatus(qr);
    if (result_status == PGRES_TUPLES_OK || result_status == PGRES_COMMAND_OK) {
        return qr;
    }
    ConnStatusType status = PQstatus(conn);
    if (status != CONNECTION_BAD) {
        return qr;
    }
    slogd("connection was bad, retrying");
    PQclear(qr);
    PQreset(conn);
    usleep(100000);
    return PQexecParams_retry(conn, command, nParams, paramTypes, paramValues, paramLengths, paramFormats, resultFormat);
}
