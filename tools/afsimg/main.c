#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <getopt.h>

#include "afs.h"

extern unsigned int crc32(const unsigned char *buf, unsigned int len);

static int decode_size(const char *str, uint64_t *value)
{
    uint64_t result = 0;

    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) {
            if (str[i + 1]) {
                return 1;
            }

            switch (str[i]) {
                case 'T':
                    result *= 1024;
                case 'G':
                    result *= 1024;
                case 'M':
                    result *= 1024;
                case 'k':
                    result *= 1024;
                    break;
                default:
                    return 1;
            }
            
            break;
        }

        result *= 10;
        result += str[i] - '0';
    }

    *value = result;
    return 0;
}

static int decode_uint(const char *str, uint64_t *value)
{
    uint64_t result = 0;

    if (str[0] != '0') {
        for (int i = 0; str[i]; i++) {
            if (!isdigit(str[i])) {
                return 1;
            }

            result *= 10;
            result += str[i] - '0';
        }

        *value = result;
        return 0;
    }

    if (str[1] == 'x') {
        for (int i = 2; str[i]; i++) {
            if (!isxdigit(str[i])) {
                return 1;
            }

            result <<= 4;
            result |= str[i] <= '9' ? str[i] - '0' : ((str[i] <= 'F' ? str[i] - 'A' : str[i] - 'a') + 10);
        }

        *value = result;
        return 0;
    }
    
    if (str[1] == 'o' || isdigit(str[1])) {
        for (int i = str[1] == 'o' ? 2 : 1; str[i]; i++) {
            if (!isdigit(str[i]) || str[i] >= '8') {
                return 1;
            }

            result <<= 3;
            result |= str[i] - '0';
        }

        *value = result;
        return 0;
    }
    
    if (str[1] == 'b') {
        for (int i = 2; str[i]; i++) {
            if (str[i] != '0' && str[i] != '1') {
                return 1;
            }

            result <<= 1;
            result |= str[i] - '0';
        }

        *value = result;
        return 0;
    }

    *value = result;
    return 0;
}

static int decode_uuid(const char *str, afs_uuid_t *value)
{
    memset(value, 0, sizeof(afs_uuid_t));

    int digit_idx = 0;
    for (int i = 0; str[i]; i++) {
        if (!isxdigit(str[i]) && str[i] != '-') {
            return 1;
        }

        if (str[i] == '-') {
            if (i != 8 && i != 13 && i != 18 && i != 23) {
                return 1;
            } else {
                continue;
            }
        }

        if (digit_idx >= 32) {
            return 1;
        }

        value->bytes[digit_idx >> 1] <<= 4;
        value->bytes[digit_idx >> 1] |= str[i] <= '9' ? str[i] - '0' : ((str[i] <= 'F' ? str[i] - 'A' : str[i] - 'a') + 10);
        digit_idx++;
    }

    return 0;
}

static const char *argv0;
static int verbose = 0;

static const struct option options[] = {
    { "help", 0, NULL, 'h' },
};

struct subcommand {
    const char *name;
    int (*handler)(int argc, char **argv);
    const char *usage_str;
};

static const struct subcommand subcommand_list[] = {
    {
        .name = "volinfo",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "mkdir",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "copy",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "rename",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "lsfile",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "setattr",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
    {
        .name = "getattr",
        .handler = NULL,
        .usage_str = "[-x]\n",
    },
};

static const struct subcommand *find_subcommand(const char *subcmd_name)
{
    for (int i = 0; i < sizeof(subcommand_list) / sizeof(*subcommand_list); i++) {
        if (strcmp(subcommand_list[i].name, subcmd_name) == 0) {
            return &subcommand_list[i];
        }
    }

    return NULL;
}

static void print_usage(const struct subcommand *subcmd)
{
    if (!subcmd) {
        fprintf(stderr, "usage: %s [-h] subcommand\navailable subcommands: ", argv0);
        for (int i = 0; i < sizeof(subcommand_list) / sizeof(*subcommand_list); i++) {
            fprintf(stderr, "%s ", subcommand_list[i].name);
        }
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "usage: %s %s %s", argv0, subcmd->name, subcmd->usage_str);
    }
}

int main(int argc, char **argv)
{
    int next_option;

    int help = 0;

    argv0 = argv[0];

    do {
        next_option = getopt_long(argc, argv, "h", options, NULL);

        switch (next_option) {
            case 'h':
                help = 1;
                break;
            case '?':
                print_usage(NULL);
                return 1;
            case -1:
                break;
            default:
                abort();
        }
    } while (next_option != -1);

    const char *subcmd_name = NULL;
    if (optind < argc) {
        subcmd_name = argv[optind];
    } else {
        print_usage(NULL);
        return 1;
    }

    if (help) {
        print_usage(subcmd_name ? find_subcommand(subcmd_name) : NULL);
        return 0;
    }

    return 0;
}

