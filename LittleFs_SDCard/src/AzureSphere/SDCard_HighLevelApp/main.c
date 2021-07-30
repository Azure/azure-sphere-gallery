/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ioctl.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

#include "eventloop_timer_utilities.h"


#include "lfs.h"
#include "lfs_util.h"

#include "SDCardViaRtCore.h"

#include "hw/mt3620_rdb.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_Init_EventLoop = 2,
    ExitCode_Init_Connection = 3,
    ExitCode_Init_SetSockOpt = 4,
    ExitCode_Main_EventLoopFail = 5,
    ExitCode_ButtonTimer_Consume = 6,
    ExitCode_Init_Button = 7,
    ExitCode_Init_ButtonPollTimer = 8,
    ExitCode_ButtonTimer_GetButtonState = 9
} ExitCode;

static EventLoop* eventLoop = NULL;
static int buttonA_fd = -1;
static EventLoopTimer* buttonPollTimer = NULL;
static GPIO_Value_Type buttonState = GPIO_Value_High;
static void ButtonTimerEventHandler(EventLoopTimer* timer);

static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void InitSigterm(void);
static ExitCode InitHandlers(void);
static void CloseHandlers(void);

// SD Card uses 512 Byte blocks
// 4MB Card size = 4,194,304 bytes - 8192 blocks
// 2GB Card size = 2,147,483,648 bytes = 4194304 total blocks
// 
// The Project is configured for 4MB Storage (8192 blocks)
#define BLOCK_SIZE     512
#define TOTAL_BLOCKS   8192     // TODO: Modify TOTAL_BLOCKS to match your SD Card configuration (total bytes/512)

char* writeMessage = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua\r\n";

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int storage_erase(const struct lfs_config* c, lfs_block_t block);
static int storage_sync(const struct lfs_config* c);

static lfs_t lfs;

const struct lfs_config g_littlefs_config = {
.read = storage_read,
.prog = storage_program,
.erase = storage_erase,
.sync = storage_sync,
.read_size = BLOCK_SIZE,
.prog_size = BLOCK_SIZE,
.block_size = BLOCK_SIZE,
.block_count = TOTAL_BLOCKS,
.block_cycles = 1000,
.cache_size = BLOCK_SIZE,
.lookahead_size = BLOCK_SIZE,
.name_max = 255
};

/// <summary>
/// Function to exercise LittleFs
/// </summary>
static void DoLittleFswork(void)
{
    Log_Debug("Button A Handler - Do LittleFs work\n");

    if (lfs_mount(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
        Log_Debug("Format and Mount\n");
        assert(lfs_format(&lfs, &g_littlefs_config) == LFS_ERR_OK);
        assert(lfs_mount(&lfs, &g_littlefs_config) == LFS_ERR_OK);
    }

    lfs_file_t datafile;
    // Create a directory, create a file in the directory, write, then read data from the file.
    size_t dataLength = strlen(writeMessage);

    // Create the directory
    Log_Debug("Create Directory '/data'\n");
    assert(lfs_mkdir(&lfs, "/data") == LFS_ERR_OK);

    // Create a file
    Log_Debug("Create File /data/lorem.txt\n");
    assert(lfs_file_open(&lfs, &datafile, "/data/lorem.txt", LFS_O_RDWR | LFS_O_CREAT) == LFS_ERR_OK);

    // Write 'Lorem' data to the file
    Log_Debug("Write to file: %s\n", writeMessage);
    assert(lfs_file_write(&lfs, &datafile, writeMessage, dataLength) == dataLength);

    // Rewind to the start of the file
    Log_Debug("Rewind file pointer\n");
    assert(lfs_file_seek(&lfs, &datafile, 0, LFS_SEEK_SET) == 0);

    // Read from the file
    char buffer[dataLength + 1];
    memset(buffer, 0x00, dataLength + 1);
    Log_Debug("Read from /data/lorem.txt\n");
    assert(lfs_file_read(&lfs, &datafile, buffer, dataLength) == dataLength);

    // Log the data that was read
    Log_Debug("Read data: %s\n", buffer);

    // Close the file
    Log_Debug("Close file\n");
    assert(lfs_file_close(&lfs, &datafile) == LFS_ERR_OK);

    // Get the file size of the directory file.
    struct lfs_info lfsInfo;
    lfs_stat(&lfs, "/data/lorem.txt", &lfsInfo);
    Log_Debug("/data/lorem.txt size (bytes): %u\n", lfsInfo.size);

    // clean up
    Log_Debug("Clean up\n");

    // remove the file first.
    Log_Debug("Delete file\n");
    assert(lfs_remove(&lfs, "/data/lorem.txt") == LFS_ERR_OK);
    
    // remove the directory.
    Log_Debug("Delete directory\n");
    assert(lfs_remove(&lfs, "/data") == LFS_ERR_OK);
}

/// <summary>
/// Littlefs callback function to handle reads from storage
/// </summary>
static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    // just need to read the block number/data over SD/Intercore.
#ifdef SHOW_DEBUG_INFO
    Log_Debug("Read Block %d\n", block);
#endif

    if (SDCard_ReadBlock(block, buffer, size) != LFS_ERR_OK)
        return LFS_ERR_IO;

    return LFS_ERR_OK;
}

/// <summary>
///  Littlefs callback function to handle writes to storage
/// </summary>
static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    if (SDCard_WriteBlock(block, buffer, size) != LFS_ERR_OK)
        return LFS_ERR_IO;

    return LFS_ERR_OK;
}

/// <summary>
/// Littlefs callback function to erase a storage block 
/// </summary>
static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
#ifdef SHOW_DEBUG_INFO
    Log_Debug("Erase Block %d\n", block);
#endif

    uint8_t block_data[c->block_size];
    memset(&block_data, 0x00, c->block_size);
    return storage_program(c, block, 0x00, &block_data, c->block_size);
}

/// <summary>
/// Littlefs callback function to sync storage (not used)
/// </summary>
static int storage_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}

/// <summary>
/// Timer callback to check for 'Button A' press
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(buttonA_fd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonState;
        return;
    }

    // If the button has just been pressed, change the LED blink interval
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {
            DoLittleFswork();
        }
        buttonState = newButtonState;
    }
}

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers for send timer
/// </summary>
static void InitSigterm(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers for send timer
///     and to receive data from real-time capable application.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitHandlers(void)
{
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    Log_Debug("Opening Button A as input.\n");
    buttonA_fd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (buttonA_fd == -1) {
        Log_Debug("ERROR: Could not open Button A: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }
    struct timespec buttonPressCheckPeriod = { .tv_sec = 0, .tv_nsec = 1000000 };
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }


    if (SDCard_Init() == -1)
    {
        Log_Debug("ERROR: Failed to initialize intercore connection\n");
        return ExitCode_Init_Connection;
    }

    return ExitCode_Success;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
/// Write 0x00 to the first two SD Card blocks
/// </summary>
void FormatCard(void)
{
    uint8_t formatBuffer[BLOCK_SIZE];

    memset(&formatBuffer[0], 0x00, BLOCK_SIZE);

    Log_Debug("Formating blocks 0 and 1\n");

    for (uint32_t x = 0; x < 2; x++)
    {
        if (SDCard_WriteBlock(x, &formatBuffer[0], BLOCK_SIZE) != LFS_ERR_OK)
        {
            Log_Debug("\nFaied to write block %d\n", x);
            break;
        }
    }
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);
    SDCard_Cleanup();
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(buttonA_fd, "Button");
}

int main(void)
{
    Log_Debug("Littlefs SD Card project\n");

    InitSigterm();

    exitCode = InitHandlers();

    // WARNING: FormatCard Writes 0x00 to the first two SD Card blocks.
    // useful to test initialization of LittleFs
    // comment the FormatCard line to leave the SD Card intact on the next run of the application
    FormatCard();

    Log_Debug("Press 'Button A' to do LittleFs work\n");

    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    CloseHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}
