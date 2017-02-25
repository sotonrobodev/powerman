#include <stdio.h>

#include "powerman.h"

static void print_devices(libusb_device** devices) {
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

        printf(
            "Found: bus %d, device %d\n",
            libusb_get_bus_number(current_device),
            libusb_get_device_address(current_device)
        );

        // Open the device to get the serial
        libusb_device_handle* handle;
        status = libusb_open(current_device, &handle);

        if (status != 0) {
            // Skip
            continue;
        }

        uint8_t serial[128];
        serial[0] = '\0';

        libusb_get_string_descriptor_ascii(
            handle,
            desc.iSerialNumber,
            serial,
            sizeof(serial)
        );

        printf(
            "Serial: %s\n",
            serial
        );

        libusb_close(handle);

        uint8_t path[8];

        status = libusb_get_port_numbers(current_device, path, sizeof(path));

        if (status > 0) {
            printf("  path: %d", path[0]);

            for (int i = 0; i < status; ++i) {
                printf(".%d", path[i]);
            }

            printf("\n");
        }
    }
}

int main_lsusb(void) {
    libusb_device** devices;
    ssize_t n_devices;

    n_devices = libusb_get_device_list(NULL, &devices);

    if (n_devices < 0) {
        fprintf(stderr, "Could not get device list\n");
        // Use the error status as the exit code
        return -(int)n_devices;
    }

    print_devices(devices);

    libusb_free_device_list(devices, 1);

    return 0;
}

