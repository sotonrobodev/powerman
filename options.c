#include <string.h>
#include <stdio.h>

#include "powerman.h"

void print_help(const char* progname) {
    printf("Usage: \n");
    printf("    %s <serial> <control-file>\n", progname);
    printf("    %s --lsusb\n", progname);
}

struct options parse_options(int argc, char** argv) {
    if (argc <= 1) {
        // No arguments specified, that's a paddlin'
        return (struct options){
            .action = PROGRAM_ACTION_ARGUMENT_FAIL,
            .powerfile_path = NULL,
            .serial = NULL
        };
    }

    // Detect any use of --help
    for (int i = 1; i < argc; ++i) {
        if (
            !strcmp(argv[i], "--help") ||
            !strcmp(argv[i], "-h")
        ) {
            return (struct options){
                .action = PROGRAM_ACTION_PRINT_HELP,
                .powerfile_path = NULL,
                .serial = NULL
            };
        }
    }

    // Check for lsusb mode
    if (argc == 2) {
        if (!strcmp(argv[1], "--lsusb")) {
            return (struct options){
                .action = PROGRAM_ACTION_LSUSB,
                .powerfile_path = NULL,
                .serial = NULL
            };
        }
    }

    if (argc == 3) {
        return (struct options){
            .action = PROGRAM_ACTION_RUN,
            .powerfile_path = argv[2],
            .serial = argv[1]
        };
    }

    return (struct options){
        .action = PROGRAM_ACTION_ARGUMENT_FAIL,
        .powerfile_path = NULL,
        .serial = NULL
    };
}

