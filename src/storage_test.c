#include "munit.h"
#include "storage.h"
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

static MunitResult test_todays_answer(const MunitParameter params[], void* user_data) {
    struct storage* storage = user_data;
    struct wordle wordle = todays_answer(storage);
    munit_assert_memory_equal(wordle_len, "cramp", wordle.word);
    return MUNIT_OK;
}

static MunitResult test_todays_game_invalid_user(const MunitParameter params[], void* user_data) {
    struct storage* storage = user_data;
    char* error_message = NULL;
    todays_game(storage, "joe", &error_message);
    munit_assert_string_equal(error_message, "invalid user!");
    return MUNIT_OK;
}

static MunitResult test_todays_game_empty(const MunitParameter params[], void* user_data) {
    struct storage* storage = user_data;
    char* error_message = NULL;
    struct game_state game_state = todays_game(storage, "martin", &error_message);
    munit_assert_null(error_message);
    munit_assert_uint(game_state.turns_len, ==, 0);
    return MUNIT_OK;
}

static MunitResult test_todays_game_1_guess(const MunitParameter params[], void* user_data) {
    struct storage* storage = user_data;
    char* error_message = NULL;
    const char* guess = "spasm"; // answer is "cramp"
    save_guess(storage, "martin", (char*)guess);
    struct game_state game_state = todays_game(storage, "martin", &error_message);
    munit_assert_null(error_message);
    munit_assert_uint(game_state.turns_len, ==, 1);
    struct guess expected = {
        .guess = {{.letter = 's', .state = incorrect},
                {.letter = 'p', .state = present_wrong_pos},
                {.letter = 'a', .state = correct},
                {.letter = 's', .state = incorrect},
                {.letter = 'm', .state = present_wrong_pos}}
    };
    struct guess actual = game_state.turns[0];
    for (uint i=0; i<wordle_len; i++) {
        munit_assert_char(actual.guess[i].letter, ==, expected.guess[i].letter);
        munit_assert_int(actual.guess[i].state, ==, expected.guess[i].state);
    }
    return MUNIT_OK;
}

static void* test_setup(const MunitParameter params[], void* user_data) {
    char* error_message = NULL;
    struct storage* storage = init_storage(0, NULL, &error_message);
    munit_assert_null(error_message);
    setup_test_storage(storage);
    return storage;
}

static void test_tear_down(void* fixture) {
    struct storage* storage = fixture;
    free_storage(storage);
}

MunitTest storage_tests[] = {
  {
    "test_todays_answer",
    test_todays_answer,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL /* parameters */
  },
  {
    "test_todays_game_invalid_user",
    test_todays_game_invalid_user,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL /* parameters */
  },
  {
    "test_todays_game_empty",
    test_todays_game_empty,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL /* parameters */
  },
  {
    "test_todays_game_1_guess",
    test_todays_game_1_guess,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL /* parameters */
  },
  /* null terminator (function pointer must be null) */
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite storage_test_suite = {
  "storage_tests",
  storage_tests,
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&storage_test_suite, NULL, argc, argv);
}
