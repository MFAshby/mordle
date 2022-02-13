    #include "munit.h"
#include "index.h"
#include <stdio.h>
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

static MunitResult test_empty_render(const MunitParameter params[], void* user_data) {
    // GIVEN
    struct game_state game_state = {
        .turns_len = 0
    };
    // WHEN
    char* rendered_index = render_index(game_state);
    
    // THEN
    char* expected = load_file("test_comps/index_empty.html");
    munit_assert_string_equal(rendered_index, expected);
    free(expected);
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
  return munit_suite_main(&index_test_suite, NULL, argc, argv);
}
