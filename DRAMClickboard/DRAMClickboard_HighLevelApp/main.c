/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// The following #include imports a "sample appliance" hardware definition. This provides a set of
// named constants such as SAMPLE_BUTTON_1 which are used when opening the peripherals, rather
// that using the underlying pin names. This enables the same code to target different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio. To target different hardware, you'll
// need to update the TARGET_HARDWARE variable in CMakeLists.txt - see instructions in that file.
//
// You can also use hardware definitions related to all other peripherals on your dev board because
// the sample_appliance header file recursively includes underlying hardware definition headers.
// See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
#include <hw/sample_appliance.h>

// MikroSDK DRAM Clickboard library import
#include "dram.h"

// Example dummy values
#define DEMO_TEXT_MESSAGE_1 "MikroE"
#define DEMO_TEXT_MESSAGE_2 "DRAM Click board"

uint8_t *send_buf;
uint8_t *retr_buf;

// Use timespec struct to slow down output
// const struct timespec sleepTime = {.tv_sec = 3, .tv_nsec = 50};

int application_task(unsigned long starting_address)
{
    // Allocate memory for Buffers to hold data for memory transfers
    send_buf = (uint8_t *)malloc(strlen(DEMO_TEXT_MESSAGE_1));
    retr_buf = (uint8_t *)malloc(strlen(DEMO_TEXT_MESSAGE_1));

    Log_Debug("Accessing %#08X\r\n", (uint32_t)starting_address);

    // Copy string into send_buf
    memcpy(send_buf, DEMO_TEXT_MESSAGE_1, strlen(DEMO_TEXT_MESSAGE_1));

    // Write send_buf into ram chip starting at specified address
    int exitCode = dram_memory_write(starting_address, send_buf, strlen(DEMO_TEXT_MESSAGE_1));
    if (exitCode != 0) {
        return exitCode;
    }

    // Read from memory location
    exitCode = dram_memory_read(starting_address, retr_buf, strlen(DEMO_TEXT_MESSAGE_1));
    if (exitCode != 0) {
        return exitCode;
    }

    // Print written data
    // Log_Debug(" Write data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_1), send_buf);

    // Show received data
    // Log_Debug(" Read data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_1), retr_buf);

    // Code check that information is error free
    int a = memcmp(send_buf, retr_buf, strlen(DEMO_TEXT_MESSAGE_1));
    if (a != 0) {
        Log_Debug(" ERROR\n");
        // Print written data
        Log_Debug(" Write data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_1), send_buf);

        // Report the data that was read
        Log_Debug(" Read data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_1), retr_buf);

        // Deallocate memory from buffers
        free(send_buf);
        free(retr_buf);
        return -1;
    }

    // Uncomment to slow down output
    // nanosleep(&sleepTime, NULL);

    //---------------Second Write/Read Command--------------------//

    // Reallocate memory for the 2nd demo text message
    send_buf = realloc(send_buf, strlen(DEMO_TEXT_MESSAGE_2));
    retr_buf = realloc(retr_buf, strlen(DEMO_TEXT_MESSAGE_2));
    memset(send_buf, 0, strlen(DEMO_TEXT_MESSAGE_2)); // Clear send_buf for next message
    memset(retr_buf, 0, strlen(DEMO_TEXT_MESSAGE_2)); // Clear retr_buf for next message

    // Copy string into send_buf
    memcpy(send_buf, DEMO_TEXT_MESSAGE_2, strlen(DEMO_TEXT_MESSAGE_2));

    // Write send_buf into ram chip starting at specified address
    exitCode = dram_memory_write(starting_address, send_buf, strlen(DEMO_TEXT_MESSAGE_2));
    if (exitCode != 0) {
        return exitCode;
    }

    // Read from memory location
    exitCode = dram_memory_read_fast(starting_address, retr_buf, strlen(DEMO_TEXT_MESSAGE_2));
    if (exitCode != 0) {
        return exitCode;
    }

    // Print written data
    // Log_Debug(" Write data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_2), send_buf);

    // Report the data that was read
    // Log_Debug(" Fast Read data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_2), retr_buf);

    // Check if data is the same
    a = memcmp(send_buf, retr_buf, (int)strlen(DEMO_TEXT_MESSAGE_2));
    if (a != 0) {
        Log_Debug(" ERROR\n");
        // Print written data
        Log_Debug(" Write data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_2), send_buf);

        // Report the data that was read
        Log_Debug(" Fast Read data: %.*s\r\n", (int)strlen(DEMO_TEXT_MESSAGE_2), retr_buf);

        free(send_buf);
        free(retr_buf);
        return -1;
    }

    // Uncomment to slow down output
    // nanosleep(&sleepTime, NULL);

    free(send_buf);
    free(retr_buf);

    return 0;
}

int main(int argc, char *argv[])
{
    Log_Debug("DRAM Clickboard application starting\n");

    // Initialize DRAM click set up
    int exitCode =
        dram_init(SAMPLE_DRAM_SPI, SAMPLE_DRAM_CS_A, SAMPLE_DRAM_SPI_IO3A, SAMPLE_DRAM_SPI_IO2A);
    if (exitCode != 0) {
        return exitCode;
    }

    unsigned long starting_address = DRAM_MIN_ADDRESS;

    // This task will write to the entire dram click until the max address.
    // After every write, it also reads the data to check if it matches with
    // what is written. It reads using dram read and read fast cmds.

    // The last addr that you can access is 0x7FFFF0 (since we also need to
    // consider the size of the data used in this app to write to the chip).
    // But we are letting this app access the addresses after 0x7FFFF0 until
    // max - 0x7FFFFF to show that we are covering the overflow case. From
    // 0x7FFFF1 you should see overflow writes when you run this app.
    while (exitCode == 0 && starting_address <= DRAM_MAX_ADDRESS) {
        exitCode = application_task(starting_address);
        starting_address++;
    }

    Log_Debug("Application exiting.\n");
    return exitCode;
}