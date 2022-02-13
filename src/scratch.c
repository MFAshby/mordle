#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (atoi(getenv("EXPECTED_FILE_UPDATE"))) {
        printf("EXPECTED_FILE_UPDATE is set\n");
    } else {
        printf("EXPECTED_FILE_UPDATE is NOT set\n");
    }
}