#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage_exit(void) {
    printf("supervisor [-n limit] [-w delay]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    printf("Hello World\n");

    // delay is infinite if it remains a NULL pointer
    int limit = 0, *delay = NULL;
    static int temp_delay;
    int c;

    char *endptr;

    while ((c = getopt(argc, argv, "n:w:")) != -1) {
        switch (c) {
            case 'n':
                limit = strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0' || limit < 0) {
                    usage_exit();
                }
                break;
            case 'w':
                temp_delay = strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0' || temp_delay < 0) {
                    usage_exit();
                }
                delay = &temp_delay;
                break;
            case '?': usage_exit();
        }
    }

    return EXIT_SUCCESS;
}
