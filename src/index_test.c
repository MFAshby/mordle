    #include "munit.h"
#include "index.h"
#include "slog.h"
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
        if (atoi(getenv("EXPECTED_FILE_UPDATE"))) { \
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
    struct game_state game_state = {
        .turns_len = 1,
        .turns = {{
            .guess = {{
                            .letter = 'f',
                            .state = incorrect,
            },{
                            .letter = 'f',
                            .state = incorrect,
            },{
                            .letter = 'f',
                            .state = incorrect,
            },{
                            .letter = 'f',
                            .state = incorrect,
            },{
                            .letter = 'f',
                            .state = incorrect,
            }}
        }},
    };
    // WHEN
    char* rendered_index = render_index(game_state);

    // THEN
    check_or_update("test_comps/index_one.html", rendered_index);
    free(rendered_index);
    return MUNIT_OK;   
}

MunitTest index_tests[] = {
  {
    "test_empty_render",
    test_empty_render,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL,
  },
  {
    "test_one_turn",
    test_one_turn,
    NULL,
    NULL,
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
