#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "powerman.h"

static libusb_device_handle* open_board(
    const char* serial,
    libusb_device** devices
) {
    libusb_device* current_device;

    // Loop over the devices until reaching the NULL sentinel
    while ((current_device = *(devices++))) {
        struct libusb_device_descriptor desc;
        int status = libusb_get_device_descriptor(current_device, &desc);

        if (status < 0) {
            // For now, silently skip.
            continue;
        }

        if (
            desc.idVendor != POWER_VENDOR ||
            desc.idProduct != POWER_PRODUCT
        ) {
            continue;
        }

        // Open the device to get the serial
        libusb_device_handle* handle;
        status = libusb_open(current_device, &handle);

        if (status != 0) {
            // Skip
            continue;
        }

        uint8_t current_serial[128];
        current_serial[0] = '\0';

        libusb_get_string_descriptor_ascii(
            handle,
            desc.iSerialNumber,
            current_serial,
            sizeof(current_serial)
        );

        if (!strcmp((const char*)(&current_serial[0]), serial)) {
            return handle;
        }

        libusb_close(handle);
    }

    return NULL;
}

static bool board_check_firmware_version(libusb_device_handle* board) {
    uint8_t fwver_buffer[4];

    int rq_status = libusb_control_transfer(
        board,
        0x80,                 // bmRequestType
        64,                   // bRequest, shared for all power board requests
        0,                    // wValue
        9,                    // wIndex = 9, get firmware version
        fwver_buffer,         // data
        sizeof(fwver_buffer), // wLength
        3000                  // timeout(ms)
    );

    if (rq_status < 0) {
        fprintf(
            stderr,
            "Could not hit FW version endpoint (errcode %d)\n",
            rq_status
        );
        return false;
    }

    fprintf(
        stderr,
        "Got firmware version: %02x%02x%02x%02x\n",
        fwver_buffer[3],
        fwver_buffer[2],
        fwver_buffer[0],
        fwver_buffer[0]
    );

    return true;
}

static bool board_initialize(libusb_device_handle* board) {
    int rq_status = libusb_control_transfer(
        board,
        0x00, // bmRequestType
        64,   // bRequest, shared for all power board requests
        1,    // wValue
        6,    // wIndex = 6, run LED
        NULL, // data
        0,    // wLength
        3000  // timeout(ms)
    );

    if (rq_status < 0) {
        fprintf(
            stderr,
            "Could not initialize board (errcode %d)\n",
            rq_status
        );
        return false;
    }

    return true;
}

static const int16_t REST = (int16_t)0x8000; // = -32768


static const int16_t TUNE_MARIO[] = {
    4, 4, REST, 4, REST, 0, 4, REST, 7, REST, REST, REST, -5, REST, REST, REST,

    0, REST, REST, -5, REST, REST, -8, REST, REST, -3, REST, -1, REST, -2, -3, REST,
    -5, 4, REST, 7, 9, REST, 5, 7, REST, 4, REST, 0, 2, -1, REST, REST,
    0, REST, REST, -5, REST, REST, -8, REST, REST, -3, REST, -1, REST, -2, -3, REST,
    -5, 4, REST, 7, 9, REST, 5, 7, REST, 4, REST, 0, 2, -1, REST, REST,

    REST, REST, 7, 6, 5, 3, REST, 4, REST, -5, -3, 0, REST, -3, 0, 2,
    REST, REST, 7, 6, 5, 3, REST, 4, REST, 12, REST, 12, 12, REST, REST, REST,
    REST, REST, 7, 6, 5, 3, REST, 4, REST, -5, -3, 0, REST, -3, 0, 2,
    REST, REST, 3, REST, REST, 2, REST, REST, 0, REST, REST, REST, REST, REST, REST, REST,

    REST, REST, 7, 6, 5, 3, REST, 4, REST, -5, -3, 0, REST, -3, 0, 2,
    REST, REST, 7, 6, 5, 3, REST, 4, REST, 12, REST, 12, 12, REST, REST, REST,
    REST, REST, 7, 6, 5, 3, REST, 4, REST, -5, -3, 0, REST, -3, 0, 2,
    REST, REST, 3, REST, REST, 2, REST, REST, 0, REST, REST, REST, REST, REST, REST, REST,

    0, 0, REST, 0, REST, 0, 2, REST, 4, 0, REST, -3, -5, REST, REST, REST,
    0, 0, REST, 0, REST, 0, 2, 4, REST, REST, REST, REST, REST, REST, REST, REST,
    0, 0, REST, 0, REST, 0, 2, REST, 4, 0, REST, -3, -5, REST, REST, REST,
    4, 4, REST, 4, REST, 0, 4, REST, 7, REST, REST, REST, -5, REST, REST, REST,

    0, REST, REST, -5, REST, REST, -8, REST, REST, -3, REST, -1, REST, -2, -3, REST,
    -5, 4, REST, 7, 9, REST, 5, 7, REST, 4, REST, 0, 2, -1, REST, REST,
    0, REST, REST, -5, REST, REST, -8, REST, REST, -3, REST, -1, REST, -2, -3, REST,
    -5, 4, REST, 7, 9, REST, 5, 7, REST, 4, REST, 0, 2, -1, REST, REST,

    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 9, REST, 9, 9, 7, REST, 5, 4, 0, REST, -3, -5, REST, REST, REST,
    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 5, REST, 5, 5, 4, REST, 2, 0, REST, REST, REST, REST, REST, REST, REST,

    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 9, REST, 9, 9, 7, REST, 5, 4, 0, REST, -3, -5, REST, REST, REST,
    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 5, REST, 5, 5, 4, REST, 2, 0, REST, REST, REST, REST, REST, REST, REST,

    0, 0, REST, 0, REST, 0, 2, REST, 4, 0, REST, -3, -5, REST, REST, REST,
    0, 0, REST, 0, REST, 0, 2, 4, REST, REST, REST, REST, REST, REST, REST, REST,
    0, 0, REST, 0, REST, 0, 2, REST, 4, 0, REST, -3, -5, REST, REST, REST,
    4, 4, REST, 4, REST, 0, 4, REST, 7, REST, REST, REST, -5, REST, REST, REST,

    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 9, REST, 9, 9, 7, REST, 5, 4, 0, REST, -3, -5, REST, REST, REST,
    4, 0, REST, -5, REST, REST, -4, REST, -3, 5, REST, 5, -3, REST, REST, REST,
    -1, 5, REST, 5, 5, 4, REST, 2, 0, REST, REST, REST, REST, REST, REST, REST,
};

static const int16_t TUNE_YAKKETY[] = {
    12, REST, 12, REST, 9, 7, 4, 0, 7, 7, 9, 9, 7, 3, 2, 0, 0, REST, 3, 4, REST, 7, 9, 7, 12, REST, REST, REST, REST, 7, 9, 7,
    12, REST, 12, REST, 9, 7, 4, 0, 7, 7, 9, 9, 7, 3, 2, 0, 7, REST, 7, 9, 11, 14, 10, 9, 7, REST, REST, REST, REST, 7, 9, 7,
    12, REST, 12, REST, 12, REST, 12, REST, 12, REST, 12, REST, 9, 7, 4, 0, 5, REST, 5, REST, 5, REST, 5, REST, 9, REST, 12, 14, 15, 12, 9, 5,
    16, 15, 16, 15, 16, 19, REST, REST, 16, 19, 16, 12, REST, REST, REST, REST,
    16, 12, REST, 7, 15, REST, REST, REST, REST, REST, REST, REST, REST, REST, 7, REST
};

static void goomba(
    libusb_device_handle* board,
    const int16_t* tune,
    size_t length
) {
    // Frequency
    int BEATS_PER_MINUTE = 80;
    int TATUMS_PER_BEAT = 8;
    int DELAY = 60000000 / (BEATS_PER_MINUTE * TATUMS_PER_BEAT);
    float fraction = 0.95f;
    uint16_t duration = (fraction * 60000) / (BEATS_PER_MINUTE * TATUMS_PER_BEAT);

    // Send tune
    float freq0 = 440.0f;

    for (int i = 0; true; i = (i + 1) % (length / sizeof(*tune))) {
        struct bees {
            uint16_t frequency;
            uint16_t duration;
        } eyes;
        if (tune[i] == REST) {
            eyes.frequency = 0;
        } else {
            eyes.frequency = (uint16_t)(freq0 * powf(2.0f, (float)(tune[i]) / 12.0f));
        }
        eyes.duration = duration;
        int rq_status = libusb_control_transfer(
            board,
            0x00, // bmRequestType
            64,   // bRequest, shared for all power board requests
            0,    // wValue
            8,    // wIndex = 6, piezo
            (uint8_t*)&eyes, // data
            sizeof(eyes),   // wLength
            3000  // timeout(ms)
        );

        if (rq_status < 0) {
            fprintf(
                stderr,
                "Could not squirt in PWM (errcode %d)\n",
                rq_status
            );
            return;
        }
        usleep(DELAY);
    }
}

static int main_loop_with_handle(
    libusb_device_handle* board,
    const char* controlfile
) {
    if (!board_check_firmware_version(board)) {
        return 2;
    }

    if (!board_initialize(board)) {
        return 3;
    }

    (void)TUNE_YAKKETY;
    (void)TUNE_MARIO;
    goomba(board, TUNE_MARIO, sizeof(TUNE_MARIO));

    return 0;
}

int main_power(const char* serial, const char* controlfile) {
    libusb_device** devices;
    ssize_t n_devices;

    n_devices = libusb_get_device_list(NULL, &devices);

    if (n_devices < 0) {
        fprintf(stderr, "Could not get device list\n");
        // Use the error status as the exit code
        return -(int)n_devices;
    }

    libusb_device_handle* board = open_board(serial, devices);

    if (!board) {
        fprintf(stderr, "Could not board: %s\n", serial);
        libusb_free_device_list(devices, 1);
        return 1;
    }

    int status = main_loop_with_handle(board, controlfile);

    libusb_close(board);
    libusb_free_device_list(devices, 1);

    return status;
}

