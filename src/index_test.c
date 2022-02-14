#include "munit.h"
#include "index.h"
#include "slog.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/**
 * Load a file into a null terminated string.
 * Caller is responsible for freeing the returned string.
 */ 
static char* load_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    int fd = fileno(file);
    struct stat stat;
    fstat(fd, &stat);
    uint string_len = (stat.st_size / sizeof(char))+1; // Room for null terminator
    char* file_content = malloc(sizeof(char) * string_len+1);
    fread(file_content, sizeof(char), string_len, file);
    fclose(file);
    file_content[string_len-1] = '\0';
    return file_content;
}

#define check_or_update(filename, actual_content_ptr) \
    do { \
        char* env = getenv("EXPECTED_FILE_UPDATE"); \
        if (env != NULL && atoi(env)) { \
            write_file(filename, actual_content_ptr); \
        } else { \
            char* expected = load_file(filename); \
            munit_assert_string_equal(actual_content_ptr, expected); \
            free(expected); \
        } \
    } while(0)

static void write_file(const char* filename, char* content) {
    FILE* file = fopen(filename, "wb");
    fwrite(content, sizeof(char), strlen(content), file);
    fclose(file);
}

static MunitResult test_empty_render(const MunitParameter params[], void* user_data) {
    // GIVEN
    struct game_state game_state = {
        .turns_len = 0
    };
    // WHEN
    char* rendered_index = render_index(game_state);
    
    // THEN
    check_or_update("test_comps/index_empty.html", rendered_index);
    
    free(rendered_index);
    return MUNIT_OK;
}

static MunitResult test_one_turn(const MunitParameter params[], void* user_data) {
    // GIVEN
    struct storage* storage = user_data;

    char* error_message = NULL;
    struct game_user game_user = find_user_by_name(storage, "martin", &error_message);
    munit_assert_null(error_message);
    
    save_guess(storage, game_user, "fffff");

    struct game_state game_state = todays_game(storage, game_user);
    munit_assert_null(error_message);

    // WHEN
    char* rendered_index = render_index(game_state);

    // THEN
    check_or_update("test_comps/index_one.html", rendered_index);
    free(rendered_index);
    return MUNIT_OK;   
}

static MunitResult test_won(const MunitParameter params[], void* user_data) {
    // GIVEN
    struct storage* storage = user_data;
    char* error_message = NULL;
    struct game_user game_user = find_user_by_name(storage, "martin", &error_message);
    munit_assert_null(error_message);
    save_guess(storage, game_user, "cramp");
    
    struct game_state game_state = todays_game(storage, game_user);
    

    // WHEN
    char* rendered_index = render_index(game_state);

    // THEN
    check_or_update("test_comps/index_won.html", rendered_index);
    free(rendered_index);
    return MUNIT_OK;   
}

static MunitResult test_lost(const MunitParameter params[], void* user_data) {
    // GIVEN
    struct storage* storage = user_data;
    char* error_message = NULL;
    struct game_user game_user = find_user_by_name(storage, "martin", &error_message);
    munit_assert_null(error_message);
    save_guess(storage, game_user, "aaaaa");
    save_guess(storage, game_user, "aaaaa");
    save_guess(storage, game_user, "aaaaa");
    save_guess(storage, game_user, "aaaaa");
    save_guess(storage, game_user, "aaaaa");
    save_guess(storage, game_user, "aaaaa");
    
    struct game_state game_state = todays_game(storage, game_user);
    

    // WHEN
    char* rendered_index = render_index(game_state);

    // THEN
    check_or_update("test_comps/index_lost.html", rendered_index);
    free(rendered_index);
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

MunitTest index_tests[] = {
  {
    "test_empty_render",
    test_empty_render,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL,
  },
  {
    "test_one_turn",
    test_one_turn,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL,
  },
  {
    "test_won",
    test_won,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL,
  },
  {
    "test_lost",
    test_lost,
    test_setup,
    test_tear_down,
    MUNIT_TEST_OPTION_NONE,
    NULL,
  },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite index_test_suite = {
  "index_tests",
  index_tests,
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char* argv[]) {
    slog_init(NULL, SLOG_FATAL | SLOG_ERROR | SLOG_WARN | SLOG_NOTE | SLOG_INFO | SLOG_DEBUG /*| SLOG_TRACE*/, 0);
    return munit_suite_main(&index_test_suite, NULL, argc, argv);
}
