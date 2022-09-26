# Real-time capable application

This real-time app runs on the MT3620 real-time cores to read and write the modbus messages through UART.
* Handle IPC_OPEN_UART command from the high-level application (HLApp) and open the UART with the configuration parameters.
* Handle IPC_WRITE_UART command from the high-level application (HLApp) and write the modbus request to UART.
* When there are bytes available on UART, RTApp will send bytes back to HLApp.
* Handle IPC_CLOSE_UART command from the high-level application (HLApp) and close UART.

**Note:** Before you run this sample, see [Communicate with a high-level application](https://learn.microsoft.com/azure-sphere/app-development/inter-app-communication). It describes how real-time capable applications communicate with high-level applications on the MT3620.

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A TTL-to-RS485 converter which connect ISU0 UART RX/TX on MT3620 to the industrial device through RS485.
1. An optional USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time capable core UART to a USB port on your PC.
1. An optional terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/.)) to display the output.

## Prep your device

To prep your device on Windows:

1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More>Run as administrator**.

   The `--enablertcoredebugging` parameter requires administrator privilege because it installs USB drivers for the debugger.

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

1. Close the window after the command completes because administrator privilege is no longer required. As a best practice, you should always use the lowest privilege that can accomplish a task.

To prep your device on Linux:

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

## Set up hardware to display output

To prepare your hardware to display output from the real-time app, see [Set up hardware to display output](https://learn.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) for Windows or [Set up hardware to display output](https://learn.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output) for Linux.

## Build and run the real-time app

The real-time app run as partners. Make sure that they're designated as partners, as described in [Mark applications as partners](https://learn.microsoft.com/azure-sphere/app-development/sideload-app#mark-applications-as-partners), so that sideloading one doesn't delete the other.

If you're using Visual Studio, you will need to deploy and debug both apps simutaneously. See the following instructions for building and running
the real-time app with Visual Studio:

### Build and run the real-time app with Visual Studio

1. On the **File** menu, select **Open > Folder**.
1. Navigate to the sourcecode directory, select MT3620_IDC_RTApp, and click **Select Folder**.
1. On the **Select Startup Item** menu, select **GDB Debugger (All Cores)**.
1. On the **Build** menu, select **Build All**.
1. On the **Debug** menu, select **Start**, or press **F5**.

## Observe the output

The real-time capable application output will be sent to the serial terminal for display.

```sh
--------------------------------
MT3620_IDC_RTApp
App built on: Oct 12 2021 11:40:54
Message received: command 0 seq_num 1 length 6
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
Message received: command 2 seq_num 2 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 6 bytes: '121fe18c'.
Message received: command 2 seq_num 3 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 12 bytes: '142212a12b12c12d12'.
UART received 12 bytes: 'e12f121012111212121312'.
UART received 12 bytes: '141215121612171218121912'.
UART received 3 bytes: '1a4c2d'.
Message received: command 2 seq_num 4 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 7 bytes: '132065786f'.
Message received: command 2 seq_num 5 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 6 bytes: '121fe18c'.
Message received: command 2 seq_num 6 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 12 bytes: '142212a12b12c12d12'.
UART received 12 bytes: 'e12f121012111212121312'.
UART received 12 bytes: '141215121612171218121912'.
UART received 3 bytes: '1a4c2d'.
Message received: command 2 seq_num 7 length 8
Sender: 77c1568c-bae1-470d-abe7-eb3fef9b6b0
UART received 7 bytes: '132065786f'.
```