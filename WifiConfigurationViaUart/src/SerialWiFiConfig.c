//  Copyright(c) Microsoft Corporation.All rights reserved.
//  Licensed under the MIT License.

#include "stdio.h"
#include "stdarg.h"
#include "stdint.h"
#include "stdlib.h"
#include "SerialWiFiConfig.h"

#include <applibs/networking.h>
#include <applibs/uart.h>
#include <applibs/powermanagement.h>
#include <applibs/wificonfig.h>
#include <soc/mt3620_uarts.h>

#include "eventloop_timer_utilities.h"

static void serialPrint(const char* fmt, ...);
static void serialPrintMenu(void);
static void UartEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context);

EventRegistration* uartEventReg = NULL;

enum menuOptionState { TOPLEVEL = 0, SSID = 1, PASSKEY = 2 };
enum menuOptionState menuOption = TOPLEVEL;

#define UART_MESSAGE_SIZE 80
static uint8_t inputBuffer[UART_MESSAGE_SIZE];
static ssize_t inputBufferPointer = 0;
static uint8_t uartMessage[UART_MESSAGE_SIZE];
static int uartFd = -1;

static char ssid[WIFICONFIG_SSID_MAX_LENGTH];

static const char* menu = "\r\n1. Reboot\r\n2. Get stored Wi-Fi networks\r\n3. Add Wi-Fi\r\n\r\nOption>";
static const char* ssidPrompt = "SSID >";
static const char* networkKeyPrompt = "Network Key >";

bool SerialWiFiConfig_Init(EventLoop* eventLoop)
{
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_None;
    uartFd = UART_Open(MT3620_UART_ISU0, &uartConfig);

    if (uartFd < 0)
        return false;

    uartEventReg = EventLoop_RegisterIo(eventLoop, uartFd, EventLoop_Input, UartEventHandler, NULL);
    if (uartEventReg == NULL)
    {
        close(uartFd);
        return false;
    }

    serialPrint("\x1b[2J\x1b[HAzure Sphere UART Wi-Fi Configuration application starting...\r\n");

    serialPrintMenu();

    return true;
}

static void SendUartMessage(int uartFd, const char* dataToSend)
{
    write(uartFd, dataToSend, strlen(dataToSend));
}

static void serialPrintMenu(void)
{
    serialPrint("\r\n%s", menu);
}

static void handleTopLevelMenuSelection(char* message)
{
    int selection = atoi(message);

    if (selection == 3)
    {
        menuOption = SSID;
        serialPrint(ssidPrompt);
    }

    if (selection == 1)
    {
        serialPrint("Rebooting...\n");
        PowerManagement_ForceSystemReboot();
    }

    if (selection == 2)
    {
        serialPrint("\r\nStored Wi-Fi networks:\r\n");

        ssize_t numNetworks = WifiConfig_GetStoredNetworkCount();
        if (numNetworks > 0)
        {
            WifiConfig_StoredNetwork* pStoredNetworks = (WifiConfig_StoredNetwork*)malloc(sizeof(WifiConfig_StoredNetwork) * (size_t)numNetworks);
            WifiConfig_GetStoredNetworks(pStoredNetworks, (size_t)numNetworks);

            for (int x = 0; x < numNetworks; x++)
            {
                serialPrint("%.*s : %s : %s\r\n", pStoredNetworks[x].ssidLength, pStoredNetworks[x].ssid, pStoredNetworks[x].isEnabled == true ? "Enabled" : "Not Enabled", pStoredNetworks[x].isConnected == true ? "Connected" : "Not Connected");
            }

            free(pStoredNetworks);
        }
        else
        {
            serialPrint("No stored Wi-Fi networks\r\n");
        }
        serialPrint("\r\n");
    }
}

static void MessageHandler(char* message)
{
    switch (menuOption)
    {
    case SSID:
        if (strlen(message) == 0)
        {
            // abort setting the SSID/PASSKEY
            menuOption = TOPLEVEL;
        }
        else
        {
            if (strlen(message) < WIFICONFIG_SSID_MAX_LENGTH)
            {
                menuOption = PASSKEY;
                strncpy(ssid, message, WIFICONFIG_SSID_MAX_LENGTH);
                serialPrint(networkKeyPrompt);
            }
            else
            {
                serialPrint("SSID is too long, try again\r\n");
                serialPrint(ssidPrompt);
            }
        }
        break;
    case PASSKEY:
        if (strlen(message) == 0)
        {
            // abort setting the SSID/PASSKEY
            menuOption = TOPLEVEL;
        }
        else
        {
            if (strlen(message) < WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE)
            {
                menuOption = TOPLEVEL;      // set top level menu, and add network
                int networkId = WifiConfig_AddNetwork();
                WifiConfig_SetSSID(networkId, ssid, strlen(ssid));
                WifiConfig_SetSecurityType(networkId, WifiConfig_Security_Wpa2_Psk);
                WifiConfig_SetPSK(networkId, message, strlen(message));
                WifiConfig_SetNetworkEnabled(networkId, true);
                WifiConfig_PersistConfig();
            }
            else
            {
                serialPrint("SSID is too long, try again\r\n");
                serialPrint(ssidPrompt);
            }
        }
        break;
    case TOPLEVEL:
        handleTopLevelMenuSelection(message);

        break;
    default:
        serialPrint(menu);
    }


    if (menuOption == TOPLEVEL)
    {
        serialPrint(menu);
    }
}

static void serialPrint(const char* fmt, ...)
{
    if (uartFd == -1)
        return;

    memset(uartMessage, 0x00, UART_MESSAGE_SIZE);
    va_list args;
    va_start(args, fmt);
    vsnprintf(uartMessage, UART_MESSAGE_SIZE, fmt, args);
    va_end(args);

    SendUartMessage(uartFd, uartMessage);
}

void UartEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    const size_t receiveBufferSize = 16;
    uint8_t receiveBuffer[receiveBufferSize + 1]; // allow extra byte for string termination
    ssize_t bytesRead;

    // Read incoming UART data. It is expected behavior that messages may be received in multiple
    // partial chunks.
    memset(receiveBuffer, 0x00, receiveBufferSize);
    bytesRead = read(uartFd, receiveBuffer, receiveBufferSize);
    if (bytesRead == -1) {
        return;
    }

    // process the bytes from the PC/Terminal.
    for (int x = 0; x < bytesRead; x++)
    {
        uint8_t chr = receiveBuffer[x];

        switch (chr)
        {
        case 0x7f:  // backspace in putty
        case 0x08:
            if (inputBufferPointer-1 > -1)
            {
                inputBufferPointer--;
                inputBuffer[inputBufferPointer] = 0x00;
                serialPrint("%c", chr);   // will update the terminal
            }
            break;
        case 0x0d:
            serialPrint("\r\n");
            MessageHandler(inputBuffer);
            inputBufferPointer = 0x00;  // reset the buffer pointer.
            memset(inputBuffer, 0x00, UART_MESSAGE_SIZE); // clear the buffer.
            break;
        default:
            if (inputBufferPointer + 1 < UART_MESSAGE_SIZE)
            {
                inputBuffer[inputBufferPointer++] = chr;
                inputBuffer[inputBufferPointer] = 0x00;
                serialPrint("%c", chr);   // will update the terminal
            }
            break;
        }
    }
}
