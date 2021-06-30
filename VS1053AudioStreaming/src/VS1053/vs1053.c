/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "vs1053.h"
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <errno.h>

#include "hw/audioplayback_hw.h"

#include <applibs/spi.h>
#include <applibs/gpio.h>

#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_CLOCKF 0x03
#define SCI_VOL 0x0B

#define VS1053_SCI_READ 0x03
#define VS1053_SCI_WRITE 0x02

#define VS1053_REG_MODE  0x00
#define VS1053_REG_STATUS 0x01
#define VS1053_REG_CLOCKF 0x03
#define VS1053_REG_VOLUME 0x0B

static int WaitOnDREQHigh(void);

static void controlModeOn(void);
static void controlModeOff(void);
static void dataModeOn(void);
static void dataModeOff(void);
static void setDCSHigh(void);
static void setDCSLow(void);
static void EnableCS(void);
static void DisableCS(void);
static void ResetVs1053(void);

uint16_t sciRead(uint8_t addr);
void sciWrite(uint8_t addr, uint16_t data);

void dumpRegs(void);

static int vs1053_fd = -1;
static int vs1053_CS = -1;
static int vs1053_xDCS = -1;
static int vs1053_DREQ = -1;
static int vs1053_RESET = -1;

static bool vs1053_initCompleted = false;

static void delay(int ms);
void timespec_diff(struct timespec* start, struct timespec* stop, struct timespec* result);
static void CloseFdAndPrintError(int fd, const char* fdName);

void VS1053_Cleanup(void)
{
	CloseFdAndPrintError(vs1053_fd, "SPI");
	CloseFdAndPrintError(vs1053_CS, "GPIO: Chip Select");
	CloseFdAndPrintError(vs1053_xDCS, "GPIO: xDCS");
	CloseFdAndPrintError(vs1053_RESET, "GPIO: RESET");
	CloseFdAndPrintError(vs1053_DREQ, "GPIO: DREQ");
}

static void CloseFdAndPrintError(int fd, const char* fdName)
{
	if (fd >= 0) {
		int result = close(fd);
		if (result != 0) {
			Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
		}
	}
}

// InitVS1053 needs to call DataModeOn
int VS1053_Init(void)
{
	SPIMaster_Config config;
	int ret = SPIMaster_InitConfig(&config);
	if (ret != 0) {
		Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
			errno);
		return -1;
	}

	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	// Setup SPI
	vs1053_fd = SPIMaster_Open(VS1053_SPI, VS1053_SPICS, &config);
	if (vs1053_fd == -1) {
		Log_Debug("Failed to open VS1053 - Error: %s\n", strerror(errno));
		return -1;
	}

	SPIMaster_SetBitOrder(vs1053_fd, SPI_BitOrder_MsbFirst);
	SPIMaster_SetMode(vs1053_fd, SPI_Mode_0);
	SPIMaster_SetBusSpeed(vs1053_fd, 1000000);		// 1MHz

	vs1053_CS = GPIO_OpenAsOutput(VS1053_CS, GPIO_OutputMode_PushPull, GPIO_Value_High);

	vs1053_xDCS = GPIO_OpenAsOutput(VS1053_DCS, GPIO_OutputMode_PushPull, GPIO_Value_High);
	vs1053_RESET = GPIO_OpenAsOutput(VS1053_RST, GPIO_OutputMode_PushPull, GPIO_Value_High);
	vs1053_DREQ = GPIO_OpenAsInput(VS1053_DREQ);

	if (vs1053_CS == -1 || vs1053_xDCS == -1 || vs1053_RESET == -1 || vs1053_DREQ == -1)
	{
		VS1053_Cleanup();
		return -1;
	}

	// wait for the IC to show ready.
	if (WaitOnDREQHigh() == -1)
	{
		Log_Debug("VS1053 not responding (or not connected)\n");
		VS1053_Cleanup();
		return -1;
	}

	ResetVs1053();

	uint16_t status = sciRead(SCI_STATUS);
	int vs1053Version = (status >> 4) & 0x000F; //Mask out only the four version bits
	Log_Debug("vs1053 version: %d\n", vs1053Version);
	if (vs1053Version != 4)	// from VS1053 VLSI manual, page 41 SCI_STATUS - http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf
	{
		Log_Debug("ERROR: This is not a VS1053 board!\n");
		return false;
	}

	vs1053_initCompleted = true;
	return 0;
}

static void controlModeOn(void)
{
	EnableCS();
	setDCSHigh();
}

static void controlModeOff(void)
{
	DisableCS();
	setDCSLow();
}

static void dataModeOn(void)
{
	EnableCS();
	setDCSLow();
}

static void dataModeOff(void)
{
	DisableCS();
	setDCSHigh();
}

static void EnableCS(void)
{
	GPIO_SetValue(vs1053_CS, GPIO_Value_Low);
}

static void DisableCS(void)
{
	GPIO_SetValue(vs1053_CS, GPIO_Value_High);
}

static void setDCSHigh(void)
{
	GPIO_SetValue(vs1053_xDCS, GPIO_Value_High);
}

static void setDCSLow(void)
{
	GPIO_SetValue(vs1053_xDCS, GPIO_Value_Low);
}

uint16_t sciRead(uint8_t addr)
{
	uint16_t data;

	controlModeOn();
	uint8_t txBuffer[2] = { 0 };
	uint8_t rxBuffer[4] = { 0 };

	txBuffer[0] = VS1053_SCI_READ;
	txBuffer[1] = addr;

	write(vs1053_fd, txBuffer, 2);
	delay(10);
	read(vs1053_fd, rxBuffer, 2);

	data = (uint16_t)((rxBuffer[0] * 0x100) + rxBuffer[1]);

	WaitOnDREQHigh();
	controlModeOff();
	return data;
}

void sciWrite(uint8_t addr, uint16_t data)
{
	controlModeOn();
	uint8_t buffer[4] = { 0 };
	buffer[0] = VS1053_SCI_WRITE;
	buffer[1] = addr;
	buffer[2] = (uint8_t)(data >> 8);
	buffer[3] = (uint8_t)(data & 0xff);

	write(vs1053_fd, buffer, 4);
	WaitOnDREQHigh();
	controlModeOff();
}

static const uint8_t modeVal[] = { 0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f };
static const unsigned char *Modes[] = { "SCI_MODE","SCI_STATUS","SCI_BASS","SCI_CLOCKF","SCI_AUDATA","SCI_WRAM","SCI_WRAMADDR","SCI_AIADDR","SCI_VOL","SCI_AICTRL0","SCI_AICTRL1","SCI_num_registers" };

void dumpRegs(void)
{
	Log_Debug("%s = 0x", Modes[0x00]); Log_Debug("%04x\n", sciRead(modeVal[0x00]));
	Log_Debug("%s = 0x", Modes[0x01]); Log_Debug("%04x\n", sciRead(modeVal[0x01]));
	Log_Debug("%s = 0x", Modes[0x08]); Log_Debug("%04x\n", sciRead(modeVal[0x08]));
}

void VS1053_PlayByte(uint8_t data)
{
	dataModeOn();
	WaitOnDREQHigh();

	write(vs1053_fd, &data, 1);
	dataModeOff();
}

void VS1053_SetVolume(uint16_t volume)
{
	uint16_t vol = (uint16_t)((volume * 0x100) + volume);
	sciWrite(SCI_VOL, vol);
}

static int WaitOnDREQHigh(void)
{
	int ret=0;
	GPIO_Value_Type dReq;
	struct timespec ts_beg, ts_end, ts_diff;

	clock_gettime(CLOCK_MONOTONIC, &ts_beg);

	while (true)
	{
		GPIO_GetValue(vs1053_DREQ, &dReq);
		if (dReq == GPIO_Value_High)
		{
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &ts_end);
		timespec_diff(&ts_beg, &ts_end, &ts_diff);
		long ms = (ts_diff.tv_sec * 1000) + (ts_diff.tv_nsec / 1000000);
		if (ms > 1000)	// 
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

static void ResetVs1053(void)
{
	// RESET the Vs1053 into default state.
	GPIO_SetValue(vs1053_RESET, GPIO_Value_Low);
	delay(1000);
	GPIO_SetValue(vs1053_RESET, GPIO_Value_High);
	delay(1000);
}


static void delay(int ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void timespec_diff(struct timespec* start, struct timespec* stop,
	struct timespec* result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	}
	else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}
