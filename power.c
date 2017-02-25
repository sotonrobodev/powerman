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
        0,    // wValue
        12,   // wIndex = 12, initialize
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

