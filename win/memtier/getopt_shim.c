#include "getopt.h"

#include <stddef.h>
#include <string.h>

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

static const char *g_short_cursor = NULL;

static const char *find_short_option(const char *optstring, int ch)
{
    const char *cursor = optstring;
    while (cursor != NULL && *cursor != '\0') {
        if (*cursor == ch) {
            return cursor;
        }
        ++cursor;
    }
    return NULL;
}

static int consume_short_option(int argc, char * const argv[], const char *optstring)
{
    const char *spec;
    int ch;

    ch = (unsigned char)*g_short_cursor++;
    spec = find_short_option(optstring, ch);
    if (spec == NULL || ch == ':') {
        optopt = ch;
        if (*g_short_cursor == '\0') {
            g_short_cursor = NULL;
            ++optind;
        }
        return '?';
    }

    if (spec[1] == ':') {
        if (*g_short_cursor != '\0') {
            optarg = (char *)g_short_cursor;
            g_short_cursor = NULL;
            ++optind;
        } else if ((optind + 1) < argc) {
            ++optind;
            optarg = argv[optind];
            g_short_cursor = NULL;
            ++optind;
        } else {
            optopt = ch;
            g_short_cursor = NULL;
            ++optind;
            return '?';
        }
    } else if (*g_short_cursor == '\0') {
        g_short_cursor = NULL;
        ++optind;
    }

    return ch;
}

int getopt_long(int argc, char * const argv[], const char *optstring,
    const struct option *longopts, int *longindex)
{
    char *arg;
    char *value;
    size_t name_len;
    int index;

    optarg = NULL;

    if (g_short_cursor != NULL && *g_short_cursor != '\0') {
        return consume_short_option(argc, argv, optstring);
    }

    if (optind >= argc) {
        return -1;
    }

    arg = argv[optind];
    if (arg == NULL || arg[0] != '-' || arg[1] == '\0') {
        return -1;
    }

    if (strcmp(arg, "--") == 0) {
        ++optind;
        return -1;
    }

    if (arg[1] != '-') {
        g_short_cursor = arg + 1;
        return consume_short_option(argc, argv, optstring);
    }

    value = strchr(arg + 2, '=');
    name_len = (value != NULL) ? (size_t)(value - (arg + 2)) : strlen(arg + 2);

    if (longopts == NULL) {
        ++optind;
        return '?';
    }

    for (index = 0; longopts[index].name != NULL; ++index) {
        if (strlen(longopts[index].name) != name_len) {
            continue;
        }
        if (strncmp(longopts[index].name, arg + 2, name_len) != 0) {
            continue;
        }

        if (longindex != NULL) {
            *longindex = index;
        }

        if (longopts[index].has_arg == required_argument) {
            if (value != NULL) {
                optarg = value + 1;
            } else if ((optind + 1) < argc) {
                ++optind;
                optarg = argv[optind];
            } else {
                ++optind;
                optopt = longopts[index].val;
                return '?';
            }
        } else if (longopts[index].has_arg == optional_argument) {
            if (value != NULL) {
                optarg = value + 1;
            }
        } else if (value != NULL) {
            ++optind;
            optopt = longopts[index].val;
            return '?';
        }

        ++optind;

        if (longopts[index].flag != NULL) {
            *longopts[index].flag = longopts[index].val;
            return 0;
        }
        return longopts[index].val;
    }

    ++optind;
    return '?';
}
