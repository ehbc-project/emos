#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "afs.h"

static const struct option options[] = {
    { "image", 1, NULL, 'i' },
    { 0, 0, 0, 0 },
};

int volinfo_handler(int argc, char **argv)
{
    int next_option;
    const char *image_path;

    do {
        next_option = getopt_long(argc, argv, "i:", options, NULL);

        switch (next_option) {
            case 'i':
                image_path = optarg;
                break;
            case '?':
                return 1;
            case -1:
                break;
            default:
                abort();
        }
    } while (next_option != -1);

    return 0;
}
