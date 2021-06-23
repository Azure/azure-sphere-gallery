/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// M24SR code based on ST hardware specification - https://www.st.com/resource/en/datasheet/m24sr64-y.pdf
// WiFi NDEF Record code based on WiFi Alliance  Wi-Fi Simple Configuration Technical Specification - https://www.wi-fi.org/

#include "M24SR.h"
#include "utils.h"

#include <string.h>
#include <errno.h>
#include <signal.h>

#include <applibs/gpio.h>
#include <applibs/i2c.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include "eventloop_timer_utilities.h"

#include "hw/avnet_mt3620_sk.h"

static int _i2cFd = -1;
static int _gpioFd = -1;

 struct NDEFMSGHDR {
	uint8_t tnf;
	uint8_t typeLength;
	uint8_t payloadLength;
} NDEFMSGHDR;

 struct NDEFRECORD {
	 uint16_t recordType;
	 uint16_t recordLength;
 } NDEFRECORD;

#define NFC_CLICK_ADDRESS	0x56
#define CMD_GETI2CSESSION	0x26
#define INS_VERIFY			0x20
#define INS_SELECT_FILE		0xa4
#define INS_READ_BINARY		0xb0
#define DESELECT			0xc2
#define KILLRFMODE			0x52

// WiFi Simple Configuration Technical Specification (available from https://www.wi-fi.org/)
// Contents of Android NFCTool WiFi NDEF Record.
/*
0x100e Credential
0x1026 Network Index
0x1045 SSID
0x1003 Authentication Type
0x100f Encryption Type
0x1027 Network Key
0x1020 MAC Address
*/
#define WIFI_CREDENTIAL		0x100e
#define WIFI_NETWORKINDEX	0x1026
#define WIFI_SSID			0x1045
#define WIFI_AUTHTYPE		0x1003
#define WIFI_ENCRYPTIONTYPE 0x100f
#define WIFI_NETWORKKEY		0x1027
#define WIFI_MACADDRESS		0x1020


#define NDEFMSBUFFERLEN		2048		// Max length for NDEF Message
static uint8_t NDefBuffer[NDEFMSBUFFERLEN];	// Need to check spec on max size.

#define BUFFER_SIZE 256			// 256 bytes needed for short record (256 bytes)
static uint8_t writeBuffer[BUFFER_SIZE];
static uint8_t readBuffer[BUFFER_SIZE];

static int WriteCommandAPDU(uint8_t class, uint8_t instruction, uint8_t p1, uint8_t p2, size_t dataLen, uint8_t* data);

// 5.6.7 ReadBinary Command uses APDU without data/length
static int WriteSimpleAPDU(uint8_t class, uint8_t instruction, uint8_t p1, uint8_t p2, size_t length);
static int SendCommand(bool setPCB, uint8_t* data, size_t len);

// Azure Sphere specific code to write/read I2C
static int WriteI2C(uint8_t* data, size_t len);
static int ReadI2C(uint8_t* data, size_t len);

static int M24SR_EnableI2C(void);		// Enable I2C Mode

static uint16_t M24SR_ComputeCrc(uint8_t* Data, uint8_t Length);
static uint16_t M24SR_UpdateCrc(uint8_t ch, uint16_t* lpwCrc);

static int M24SR_SelectFileNdefApp(void);
static int M24SR_SelectFileNdefFile(void);
static uint16_t M24SR_GetMessageLength(void);
static int M24SR_ProcessNDEFMessage(uint8_t* buffer, uint16_t length, struct M24SR_WifiConfig* wifiConfig);
static void M24SR_ShowSystemFile(void);
static int M24SR_VerifyI2cPassword(void);
static int M24SR_GetNdefMessage(uint8_t* buffer, size_t len, struct M24SR_WifiConfig* wifiConfig);
static int M24SR_Deselect(void);

static uint8_t MapAuthType(uint16_t authType);
static void SwapNDEFRecordItems(struct NDEFRECORD* pSource, struct NDEFRECORD* pDest);

static EventLoopTimer* gpoPollTimer = NULL;
static GPIO_Value_Type GpoState = GPIO_Value_High;
static void GpoPollTimerEventHandler(EventLoopTimer* timer);

static void CloseFdAndPrintError(int fd, const char* fdName);

M24SR_GPOCallback _gpoCallback = NULL;

extern volatile sig_atomic_t exitCode;

static bool block=false;		// used in I-Block (section 5.2)

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char* fdName)
{
	if (fd >= 0) {
		int result = close(fd);
		if (result != 0) {
			Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
		}
	}
}

/// <summary>
/// Cleanup resources.
/// </summary>
/// <param name=""></param>
void M24SR_ClosePeripheralsAndHandlers(void)
{
	DisposeEventLoopTimer(gpoPollTimer);
	CloseFdAndPrintError(_i2cFd, "M24SR I2C Interface");
	CloseFdAndPrintError(_gpioFd, "M24SR GPIO Interrupt");
}

/// <summary>
/// Timer handler to check M24SR Event Interrupt GPIO and read WiFi NDEF Record
/// </summary>
static void GpoPollTimerEventHandler(EventLoopTimer* timer)
{
	if (ConsumeEventLoopTimerEvent(timer) != 0) {
		exitCode = ExitCode_GPOTimer_Consume;
		return;
	}

	// Check for a GPIO interrupt
	GPIO_Value_Type newGpoState = GPIO_Value_High;
	int result = GPIO_GetValue(_gpioFd, &newGpoState);
	if (result != 0) {
		Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
		exitCode = ExitCode_GPOTimer_GetState;
		return;
	}

	if (newGpoState == GPIO_Value_High && GpoState == GPIO_Value_Low) {
		GpoState = newGpoState;
		Log_Debug("\nRecevied NFC Interrupt!\n");

		if (_gpoCallback != NULL)
		{
			struct M24SR_WifiConfig wifiConfig;

			delay_ms(200);
			Log_Debug("Process NDEF Record\n");

			if (M24SR_GetNdefMessage(&NDefBuffer[0], NDEFMSBUFFERLEN, &wifiConfig) == 0)
			{
				_gpoCallback(&wifiConfig);
			}
			M24SR_Deselect();
		}
	}

	if (newGpoState != GpoState)
	{
		GpoState = newGpoState;
	}
}


/// <summary>
/// store I2C and GPIO Fd values 
/// </summary>
int M24SR_Init(EventLoop *eventLoop, M24SR_GPOCallback callback)
{
	_gpoCallback = callback;

#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	if (callback == NULL)
	{
		Log_Debug("ERROR: NFC Tap callback is NULL\n");
		exitCode = ExitCode_Init_Failed;
		return exitCode;
	}

	Log_Debug("Opening INT_GPO as input.\n");
	_gpioFd = GPIO_OpenAsInput(AVNET_MT3620_SK_GPIO2);
	if (_gpioFd < 0) {
		Log_Debug("ERROR: Could not open GPIO pin: %s (%d).\n", strerror(errno), errno);
		exitCode = ExitCode_Init_Failed;
		return exitCode;
	}

	_i2cFd = I2CMaster_Open(AVNET_MT3620_SK_ISU2_I2C);
	if (_i2cFd < -1)
	{
		Log_Debug("ERROR: Could not open GPIO pin: %s (%d).\n", strerror(errno), errno);
		exitCode = ExitCode_Init_Failed;
	}

	I2CMaster_SetBusSpeed(_i2cFd, I2C_BUS_SPEED_STANDARD);
	I2CMaster_SetTimeout(_i2cFd, 100);

	struct timespec GpoPollCheckPeriod = { .tv_sec = 0, .tv_nsec = 1000000 };
	gpoPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &GpoPollTimerEventHandler, &GpoPollCheckPeriod);
	if (gpoPollTimer == NULL) {
		exitCode = ExitCode_Init_Failed;
		return exitCode;
	}

	// Optionally display the system file contents.
	M24SR_ShowSystemFile();

	if (M24SR_VerifyI2cPassword() == 0)
	{
		M24SR_Deselect();
	}
	else
	{
		exitCode = ExitCode_Init_Failed;
		return exitCode;
	}


	return ExitCode_Success;
}

// 5.6.3 NDEF Tag Select Application
static int M24SR_SelectFileNdefApp(void)
{
	int ret = 0;

#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t pData[7] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
	WriteCommandAPDU(0x00, 0xa4, 0x04, 0x00, 7, &pData[0]);
	ReadI2C(&readBuffer[0], 5);

	if (readBuffer[1] == 0x90 && readBuffer[2] == 0x00)
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("NDEF Tag Application Select OK\n");
#endif
	}

	if (readBuffer[1] == 0x6a && readBuffer[2] == 0x82)
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("NDEF Tag Application Select - Applicaiton Not Found\n");
#endif
		ret = -1;
	}

	if (readBuffer[1] == 0x6d && readBuffer[2] == 0x00)
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("NDEF Tag Application Select - Class not supported\n");
#endif
		ret = -1;
	}

	return ret;
}

// 5.6.5 NDEF Select command
static int M24SR_SelectFileNdefFile(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t pData[2] = { 0x00, 0x01 };	// file to select (NDEF File [not System or CC file]).

	WriteCommandAPDU(0x00, INS_SELECT_FILE, 0x00, 0x0c, 0x02, &pData[0]);
	return ReadI2C(&readBuffer[0], 5);
}

// Section 5 - I2C and RF command Sets
// ReadBinary
static uint16_t M24SR_GetMessageLength(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint16_t msgLen = 0;
	uint8_t MsgData[7] = { 0 };

	WriteSimpleAPDU(0x00, INS_READ_BINARY, 0x00, 0x00, 0x02);
	ReadI2C(&MsgData[0], 7);

	msgLen = (uint16_t)((MsgData[1] << 8) + (MsgData[2] & 0x00ff));

	// invalid length.
	if (msgLen == 0xfefe)
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("Invalid Message Length\n");
#endif
		msgLen = 0;
	}

	return msgLen;
}

// 5.6.6 System File Select
static void M24SR_SelectSystemFile(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t pData[2] = { 0xe1, 0x01 };

	WriteCommandAPDU(0x00, INS_SELECT_FILE, 0x00, 0x0c, 0x02, pData);
	ReadI2C(&readBuffer[0],5);
	if (readBuffer[1] == 0x90 && readBuffer[2] == 0x00)
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("Select System File OK\n");
#endif
	}
}

// 3.1.4 System file layout 
static void M24SR_ReadSystemFile(uint8_t* buffer, size_t length)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	size_t readLength = (size_t)(length + 5);
	uint8_t data[readLength];
	memset(data, 0x00, readLength);
	WriteSimpleAPDU(0x00, INS_READ_BINARY, 0x00, 0x00, length);
	ReadI2C(data, readLength);
	memcpy(buffer, &data[1], length);	// offset of 1, then length bytes == system file contents.
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	DumpBuffer(data, (uint16_t)readLength);
#endif
}

/// <summary>
/// Read and (optionally) display the System File.
/// </summary>
// 3.1.4 Table 6 defines the System File layout.
void M24SR_ShowSystemFile(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	M24SR_SelectFileNdefApp();
	M24SR_SelectSystemFile();
	uint16_t fileLength = M24SR_GetMessageLength();

	if (fileLength == 0x12)
	{
		memset(readBuffer, 0x00, fileLength);

		M24SR_ReadSystemFile(&readBuffer[0], (fileLength & 0xff));

		if (readBuffer[0x11] == 0x84)
		{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
			Log_Debug("System file read OK\n");
#endif
			// Add code here to display the system file contents.
		}
	}
}

/// <summary>
/// Map NDEF WiFi record values to Azure Sphere WifiConfig values
/// </summary>
static uint8_t MapAuthType(uint16_t authType)
{
	uint8_t ret= WifiConfig_Security_Open;

	if (authType != 0x0001)	// if not open
	{
		ret = WifiConfig_Security_Wpa2_Psk;
	}

	return ret;
}

/// <summary>
/// Process the NDEF Record
/// Ensure that we have a WiFi NDEF Record, that this is:
///   Single Record
///   Short Record type (<= 255 bytes)
///   Has the appropriate MIME type
///   Isn't chunked
///
/// Extract the SSID and Network Key from the WiFi NDEF Record
/// </summary>
static int M24SR_ProcessNDEFMessage(uint8_t* buffer, uint16_t length, struct M24SR_WifiConfig *wifiConfig)
{
	buffer += 2;	// Bytes 0+1 = length of full Message.
	unsigned int idLengthOffset = 0;

	struct NDEFMSGHDR* pHdr = (struct NDEFMSGHDR*)buffer;

	// pull out bit fields
	bool mb = (pHdr->tnf & 0x80) != 0;
	bool me = (pHdr->tnf & 0x40) != 0;
	bool cf = (pHdr->tnf & 0x20) != 0;
	bool sr = (pHdr->tnf & 0x10) != 0;
	bool il = (pHdr->tnf & 0x8) != 0;
	uint8_t tnf = (pHdr->tnf & 0x7);

	if (mb && me)	// only one record.
	{
		Log_Debug("One NDEF Record\n");
	}
	else
	{
		Log_Debug("Error: Only single NDEF record supported.\n");
		return -1;
	}

	if (sr)
	{
		Log_Debug("Short Record <= 255 bytes\n");
	}
	else
	{
		Log_Debug("ERROR: Only short NDEF records are supported\n");
		return -1;
	}

	if (il)
	{
		Log_Debug("Record contains ID Length\n");
		idLengthOffset = 1;
	}

	if (tnf & 0x02)
	{
		Log_Debug("Record contains MIME type\n");
	}
	else
	{
		Log_Debug("ERROR: Only MIME type is supported\n");
		return -1;
	}

	if (!cf)
	{
		Log_Debug("Record is not chunked\n");
	}
	else
	{
		Log_Debug("ERROR: Chunked record not supported\n");
		return -1;
	}

	Log_Debug("Type Length %d (0x%02x)\n", pHdr->typeLength, pHdr->typeLength);
	Log_Debug("Payload Length %d (0x%02x)\n", pHdr->payloadLength, pHdr->payloadLength);
	Log_Debug("ID Length %d (0x%02x)\n", idLengthOffset, idLengthOffset);

	if (strncmp("application/vnd.wfa.wsc", &buffer[sizeof(NDEFMSGHDR) + idLengthOffset], pHdr->typeLength) == 0)
	{
		Log_Debug("WiFi MIME type confirmed\n");
	}
	else
	{
		Log_Debug("ERROR: Not WiFi NDEF Record\n");
		return -1;
	}

#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("NDEF Message Payload:\n");
	DumpBuffer(&buffer[sizeof(NDEFMSGHDR) + pHdr->typeLength], pHdr->payloadLength);
#endif

	// Now walk the NDEF Record tags.
	// three are needed...
	// 0x1045 SSID
	// 0x1027 Network Key
	// 0x1003 Authentication Type(needs to map to Azure Sphere types)

	uint8_t* ptr = &buffer[sizeof(NDEFMSGHDR) + idLengthOffset + pHdr->typeLength + 1];	// point to first record.
	size_t bufferOffset = 0;		// offset into the NDEFMSGHDR buffer.

	uint16_t content16t = 0;

	bool haveSSID = false;
	bool haveAUTHTYPE = false;
	bool haveNETWORKKEY = false;

	wifiConfig->wifiConfigSecurityType = WifiConfig_Security_Open;
	memset(wifiConfig->networkKeyString, 0x00, 32);
	memset(wifiConfig->ssidString, 0x00, 32);

	struct NDEFRECORD ndefRecord;

	while (bufferOffset <= (size_t)pHdr->payloadLength)
	{
		struct NDEFRECORD* pRecord = (struct NDEFRECORD*)&ptr[bufferOffset];
		SwapNDEFRecordItems(pRecord, &ndefRecord);
		bufferOffset += sizeof(NDEFRECORD);		// skip past type and length.

		Log_Debug("recordType: 0x%02x\n", ndefRecord.recordType);

		// Records are defined as 2 byte ID, 2 byte size, then content
		switch (ndefRecord.recordType) {
		case WIFI_CREDENTIAL:		// skip
			Log_Debug("WIFI_CREDENTIAL\n");
			break;
		case WIFI_NETWORKINDEX:		// skip size and content
			Log_Debug("WIFI_NETWORKINDEX\n");
			bufferOffset += ndefRecord.recordLength;
			break;
		case WIFI_SSID:
			Log_Debug("SSID\n");
			if (ndefRecord.recordLength > 0)
			{
				haveSSID = true;
				memcpy(wifiConfig->ssidString, &ptr[bufferOffset], ndefRecord.recordLength);
				bufferOffset += ndefRecord.recordLength;
			}
			break;
		case WIFI_AUTHTYPE:			// two bytes
			Log_Debug("AUTHTYPE\n");
			content16t= (uint16_t)((ptr[bufferOffset] << 8) + (ptr[bufferOffset + 1] & 0x00ff));
			bufferOffset += 2;

			wifiConfig->wifiConfigSecurityType=MapAuthType(content16t);

			Log_Debug("AuthType: 0x%02x\n", wifiConfig->wifiConfigSecurityType);
			haveAUTHTYPE = true;
			break;
		case WIFI_ENCRYPTIONTYPE:
			Log_Debug("WIFI_ENCRYPTIONTYPE\n");
			bufferOffset += ndefRecord.recordLength;
			break;
		case WIFI_NETWORKKEY:
			Log_Debug("WIFI_NETWORKKEY\n");

			if (ndefRecord.recordLength > 0)
			{
				haveNETWORKKEY = true;
				memcpy(wifiConfig->networkKeyString, &ptr[bufferOffset], ndefRecord.recordLength);
				bufferOffset += ndefRecord.recordLength;
			}
			break;
		case WIFI_MACADDRESS:
			bufferOffset += ndefRecord.recordLength;
			break;
		default:
			break;
		}
	}

	if (!haveSSID || !haveAUTHTYPE || !haveNETWORKKEY)
	{
		Log_Debug("Error: missing ssid, auth type, or network key\n");
		return -1;
	}

	Log_Debug("SSID       : %s\n",wifiConfig->ssidString);
	Log_Debug("Network Key: %s\n", wifiConfig->networkKeyString);

	return 0;
}

static void SwapNDEFRecordItems(struct NDEFRECORD* pSource, struct NDEFRECORD* pDest)
{
	pDest->recordLength = (uint16_t)(((pSource->recordLength & 0xff) << 8) | pSource->recordLength >> 8);
	pDest->recordType = (uint16_t)(((pSource->recordType & 0xff) << 8) | pSource->recordType >> 8);
}

/// <summary>
/// Switch from RF Mode to I2C - read and process the NDEF Record
/// </summary>
int M24SR_GetNdefMessage(uint8_t* buffer, size_t length, struct M24SR_WifiConfig* wifiConfig)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	int ret = 0;	// assume success.

	delay(500);
	M24SR_EnableI2C();
	delay(1);
	M24SR_SelectFileNdefApp();
	M24SR_SelectFileNdefFile();
	uint16_t msgLen=M24SR_GetMessageLength();
	if (msgLen != 0)
	{
		Log_Debug("NDEF Message Length %d\n", msgLen);
		M24SR_ReadSystemFile(readBuffer, ((uint8_t)(msgLen & 0xff)));
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("---------------\n");
		Log_Debug("NDEF Message:\n");
		DumpBuffer(readBuffer, msgLen);
		Log_Debug("---------------\n");
#endif
		ret = M24SR_ProcessNDEFMessage(readBuffer, msgLen, wifiConfig);
	}
	else
	{
		ret = -1;	// message length is invalid
	}

	M24SR_Deselect();

	return ret;
}

/// Section 5 - Table 14 - Verify Command (checks access values for passwords)
int M24SR_VerifyI2cPassword(void)
{
	int ret = 0;
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t pPassword[16] = { 0x00 };

	M24SR_SelectFileNdefApp();

	WriteCommandAPDU(0x00, INS_VERIFY, 0x00, 0x03, 0x10, &pPassword[0]);
	ReadI2C(&readBuffer[0], 5);
	if (readBuffer[1] == 0x90 && readBuffer[2] == 0x00)
	{
		Log_Debug("Password Verify OK\n");
	}
	else
	{
		Log_Debug("ERROR: Password NOT verified\n");
		ret = -1;
	}

	return ret;
}

// 5.2.1 C-APDU Payload format
static int WriteCommandAPDU(uint8_t class, uint8_t instruction, uint8_t p1, uint8_t p2, size_t dataLen, uint8_t* data)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	memset(&writeBuffer[0], 0x00, BUFFER_SIZE);

	writeBuffer[1] = class;
	writeBuffer[2] = instruction;
	writeBuffer[3] = p1;
	writeBuffer[4] = p2;
	writeBuffer[5] = (uint8_t)dataLen;
	memcpy(&writeBuffer[6], data, dataLen);

	SendCommand(true, &writeBuffer[0], dataLen+ 6);	// header + size of data

	return 0;
}

// 5.6.7 ReadBinary Command
static int WriteSimpleAPDU(uint8_t class, uint8_t instruction, uint8_t p1, uint8_t p2, size_t length)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	memset(&writeBuffer[0], 0x00, BUFFER_SIZE);

	writeBuffer[1] = class;
	writeBuffer[2] = instruction;
	writeBuffer[3] = p1;
	writeBuffer[4] = p2;
	writeBuffer[5] = (uint8_t)length;
	return SendCommand(true, &writeBuffer[0], 6);
}

/// <summary>
/// Compute CRC and Send command to the M24SR
/// </summary>
static int SendCommand(bool setPCB, uint8_t* data, size_t len)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	if (setPCB) {
		// toggle the block number.
		if (!block) {
			data[0] = 0x02;
		}
		else {
			data[0] = 0x03;
		}
		block = !block;
	}

	uint16_t crc=M24SR_ComputeCrc(data, (uint8_t)len);

	data[len] = ((uint8_t)crc & 0xff);
	data[len+1] = ((uint8_t)((crc >> 8) & 0xff));

	// get a session.
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("Get session\n");
#endif
	uint8_t openSession = CMD_GETI2CSESSION;
	int i2cResult = WriteI2C(&openSession, 1);
	if (i2cResult != 0)
	{
		Log_Debug("Open Session failed %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	delay(30);

#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	DumpBuffer(data, (uint8_t)(len + 2));
#endif

	return WriteI2C(data, (uint8_t)(len + 2));	// len+2 to include the CRC
}

/// <summary>
///  Write data to the M24SR using Azure Sphere APIs
/// </summary>
static int WriteI2C(uint8_t* data, size_t len)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	if (_i2cFd < 0)
		return -1;

	int ret = I2CMaster_Write(_i2cFd, NFC_CLICK_ADDRESS, data, len);

	if (ret == -1 && errno == 6)	// no device or address.
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("WriteI2C Retry (errno 6, no device)\n");
#endif
		// retry the write.
		for (int retryLoop = 0; retryLoop < 5; retryLoop++)
		{
			delay(1);
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
			Log_Debug("WriteI2C Retry %d/5 (errno 6, no device)\n", retryLoop + 1);
#endif
			ret = I2CMaster_Write(_i2cFd, NFC_CLICK_ADDRESS, data, len);
			if (ret == 0)
			{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
				Log_Debug("Retry success\n");
#endif
				break;
			}

		}
	}

	if (ret == -1)
	{
		Log_Debug("Write to I2C failed (ret: %d) - %d(%s)\n", ret, errno, strerror(errno));
		return -1;
	}

	return 0;
}

/// <summary>
/// Read data from the M24SR using Azure Sphere APIs
/// </summary>
static int ReadI2C(uint8_t* data, size_t len)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	int ret = 0;
	delay(1);
	memset(data, 0x00, len);
	int numRead = I2CMaster_Read(_i2cFd, NFC_CLICK_ADDRESS, data, len);

	if (numRead != len)
	{
		Log_Debug("I2C Read, expected %d, got %d\n", len, ret);
		ret = -1;
	}
	else
	{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
		Log_Debug("ReadI2C\n");
		DumpBuffer(data, len);
#endif
	}

	return ret;
}

// KILL RF MODE - 5.10.2
static int M24SR_EnableI2C(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t resp[3];
	static uint8_t data = KILLRFMODE;
	SendCommand(false, &data, 1);
	return ReadI2C(&resp[0], 3);
}

// 5.4 S-Block format - command to deselect I2C (enable RF)
int M24SR_Deselect(void)
{
#ifdef ENABLE_VERBOSE_DEBUG_OUTPUT
	Log_Debug("%s\n", __func__);
#endif

	uint8_t resp[3];

	uint8_t deselectData = DESELECT;
	SendCommand(false, &deselectData, 1);
	delay(10);
	return ReadI2C(&resp[0], 3);
}

// CRC calculations are based on the ST Programmers Application Note:
// https://www.st.com/resource/en/application_note/dm00102751-m24sr-programmers-application-note-based-on-m24srdiscovery-firmware-stmicroelectronics.pdf 

static uint16_t M24SR_UpdateCrc(uint8_t ch, uint16_t* lpwCrc)
{
	ch = (ch ^ (uint8_t)((*lpwCrc) & 0x00FF));
	ch = (ch ^ (ch << 4));
	*lpwCrc = (*lpwCrc >> 8) ^ ((uint16_t)ch <<
		8) ^ ((uint16_t)ch << 3) ^ ((uint16_t)ch >> 4);
	return(*lpwCrc);
}
/**
 * @brief This function returns the CRC 16
 * @param Data : pointer on the data used to compute the CRC16
 * @param Length : number of byte of the data
 * @retval CRC16
 */
static uint16_t M24SR_ComputeCrc(uint8_t* Data, uint8_t Length)
{
	uint8_t chBlock;
	uint16_t wCrc;
	wCrc = 0x6363; // ITU-V.41
	do {
		chBlock = *Data++;
		M24SR_UpdateCrc(chBlock, &wCrc);
	} while (--Length);
	return wCrc;
}

