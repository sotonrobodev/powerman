#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "powerman.h"

static bool init(void) {
    int init_status = libusb_init(NULL);

    return init_status >= 0;
}

static void teardown(void) {
    libusb_exit(NULL);
}

int main(int argc, char** argv) {
    struct options opts = parse_options(argc, argv);

    switch (opts.action) {
    case PROGRAM_ACTION_ARGUMENT_FAIL:
        print_help(argv[0]);
        return 1;

    case PROGRAM_ACTION_PRINT_HELP:
        print_help(argv[0]);
        return 0;

    default:
        break;
    }

    if (!init()) {
        fprintf(stderr, "Could not initialise libusb\n");
        return 1;
    }

    int status;

    if (opts.action == PROGRAM_ACTION_LSUSB) {
        status = main_lsusb();
    } else {
        status = main_power(opts.serial, opts.powerfile_path);
    }

    teardown();
    return status;
}

