#ifndef __POWERMAN
#define __POWERMAN

#include <libusb.h>
#include <stdint.h>
#include <stdbool.h>

enum program_action {
    PROGRAM_ACTION_ARGUMENT_FAIL,
    PROGRAM_ACTION_PRINT_HELP,
    PROGRAM_ACTION_RUN,
    PROGRAM_ACTION_LSUSB
};

struct options {
    enum program_action action;
    const char* powerfile_path;
    const char* serial;
};

struct options parse_options(int argc, char** argv);
void print_help(const char* progname);

// General, useful constants
static const uint16_t POWER_VENDOR  = 0x1BDA;
static const uint16_t POWER_PRODUCT = 0x0011;

// Main entry points
int main_lsusb(void);
int main_power(const char* serial, const char* powerfile);

#endif

