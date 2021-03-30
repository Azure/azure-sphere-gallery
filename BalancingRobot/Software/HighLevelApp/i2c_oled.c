#include "i2c_oled.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "applibs_versions.h"
#include <errno.h>
#include "utils.h"
#include <string.h>

#include "soc/mt3620_i2cs.h"
#include "applibs/i2c.h"
#include <applibs/log.h>

#define SSD1306_WIDE_LCDWIDTH 128
#define SSD1306_WIDE_LCDHEIGHT 64

#define SSD1306_TALL_LCDWIDTH 128
#define SSD1306_TALL_LCDHEIGHT 32

static uint8_t displayBuffer[1024]; // 128*64 pixels (128/8)*64 bytes - also covers 32*128 (4x128) = 512

static void ssd1306_command(uint8_t c);
static void ssd1306_commands(uint8_t* commands, int numCommands); 
static void i2cSendBytes(uint8_t* data, size_t length);
static bool IsPixel(uint8_t* image, int width, int height, int x, int y);
static void SSD1306_SetPixelInternal(int x, int y, bool turnOn);

static uint8_t oledDisplayAddress = 0x3C;
static int _i2cfd = -1;
static bool _useVerticalDisplay = false;

static int DisplayWidth = 0;
static int DisplayHeight = 0;

bool SSD1306_Init(bool useVerticalDisplay)
{
    _useVerticalDisplay = useVerticalDisplay;

    if (useVerticalDisplay)
    {
        DisplayWidth = SSD1306_TALL_LCDWIDTH;
        DisplayHeight = SSD1306_TALL_LCDHEIGHT;
    }
    else
    {
        DisplayWidth = SSD1306_WIDE_LCDWIDTH;
        DisplayHeight = SSD1306_WIDE_LCDHEIGHT;
    }

    if (_i2cfd > -1)
    {
        return true;
    }

    _i2cfd = I2CMaster_Open(MT3620_I2C_ISU2);
    if (_i2cfd == -1)
    {
        return false;
    }

    int result = I2CMaster_SetBusSpeed(_i2cfd, I2C_BUS_SPEED_FAST);
    if (result != 0)
    {
        return false;
    }

    result = I2CMaster_SetTimeout(_i2cfd, 100);
    if (result != 0) {
        return false;
    }

    result = I2CMaster_SetDefaultTargetAddress(_i2cfd, oledDisplayAddress);
    if (result != 0) {
        Log_Debug("I2CMaster_SetDefaultTargetAddress (1): errno=%d (%s)\n", errno, strerror(errno));
        return false;
    }

    uint8_t data = 0x00;	// I2C OLED Display 'turn off'
    I2C_DeviceAddress _devAddr = 0x00;
    bool deviceFound = false;

    for (int x = 0; x <= 0x7f; x++)
    {
        _devAddr = x;
        ssize_t ret = I2CMaster_Write(_i2cfd, _devAddr, &data, 1);
        if (ret != -1)
        {
            Log_Debug("Found address: 0x%02x\n", x);
            deviceFound = true;
        }
    }

    if (!deviceFound)
    {
        Log_Debug("Didn't find an I2C devices\n");
        return false;
    }

    if (!_useVerticalDisplay)
    {
        // 128x64
        ssd1306_command(0xAE); // display off
        ssd1306_command(0xD5); // clock
        ssd1306_command(0x81); // upper nibble is rate, lower nibble is divisor
        ssd1306_command(0xA8); // mux ratio
        ssd1306_command(0x3F); // rtfm
        ssd1306_command(0xD3); // display offset
        ssd1306_command(0x00); // rtfm
        ssd1306_command(0x00);
        ssd1306_command(0x8D); // charge pump
        ssd1306_command(0x14); // enable
        ssd1306_command(0x20); // memory addr mode
        ssd1306_command(0x00); // horizontal
        ssd1306_command(0xA1); // segment remap
        ssd1306_command(0xA5); // display on
        ssd1306_command(0xC8); // com scan direction
        ssd1306_command(0xDA); // com hardware cfg
        ssd1306_command(0x12); // alt com cfg
        ssd1306_command(0x81); // contrast aka current
        ssd1306_command(0x7F); // 128 is midpoint
        ssd1306_command(0xD9); // precharge
        ssd1306_command(0x11); // rtfm
        ssd1306_command(0xDB); // vcomh deselect level
        ssd1306_command(0x20); // rtfm
        ssd1306_command(0xA6); // non-inverted
        ssd1306_command(0xA4); // display scan on
        ssd1306_command(0x2e);
        ssd1306_command(0xAF); // drivers on
    }
    else
    {
        // 128x32
        ssd1306_command(0xAE); // display off
        ssd1306_command(0xD5); // clock
        ssd1306_command(0x81); // upper nibble is rate, lower nibble is divisor
        ssd1306_command(0xA8); // mux ratio
        ssd1306_command(0x1F); // rtfm
        ssd1306_command(0xD3); // display offset
        ssd1306_command(0x00); // rtfm
        ssd1306_command(0x00);
        ssd1306_command(0x8D); // charge pump
        ssd1306_command(0x14); // enable
        ssd1306_command(0x20); // memory addr mode
        ssd1306_command(0x00); // horizontal
        ssd1306_command(0xA1); // segment remap
        ssd1306_command(0xA5); // display on
        ssd1306_command(0xC8); // com scan direction
        ssd1306_command(0xDA); // com hardware cfg
        ssd1306_command(0x02); // alt com cfg
        ssd1306_command(0x81); // contrast aka current
        ssd1306_command(0x7F); // 128 is midpoint
        ssd1306_command(0xD9); // precharge
        ssd1306_command(0x11); // rtfm
        ssd1306_command(0xDB); // vcomh deselect level
        ssd1306_command(0x20); // rtfm
        ssd1306_command(0xA6); // non-inverted
        ssd1306_command(0xA4); // display scan on
        ssd1306_command(0xAF); // drivers on
    }

    SSD1306_Clear();
    SSD1306_Display();
    

    return true;
}

static void ssd1306_commands(uint8_t* commands, int numCommands)
{
    // need to stick '0x00' on the front to make this a command.
    uint8_t* buffer = (uint8_t*)malloc(numCommands + 1);
    memset(buffer, 0x00, numCommands + 1);
    memcpy(&buffer[1], commands, numCommands);
    i2cSendBytes(buffer, numCommands+1);
    free(buffer);
}

void SSD1306_DrawImage(uint8_t* image, int width, int height, int xOffset, int yOffset)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (IsPixel(image, width, height, x, y))
            {
                SSD1306_SetPixelInternal(x+xOffset, y+yOffset, true);
            }
            else
            {
                SSD1306_SetPixelInternal(x + xOffset, y + yOffset, false);
            }
        }
    }

    //Log_Debug("Dump Full Display\n");
    //DumpDisplayBuffer(displayBuffer, DisplayWidth, DisplayHeight);
}

static void SSD1306_SetPixelInternal(int x, int y, bool turnOn)
{
    if (x >= 0 && y >= 0 && x < DisplayWidth && y < DisplayHeight)
    {
        if (turnOn)
        {
            displayBuffer[x + (y / 8) * DisplayWidth] |= 1 << (y % 8);
        }
        else
        {
            displayBuffer[x + (y / 8) * DisplayWidth] &= ~(1 << (y % 8));
        }
    }
}

void SSD1306_RotateImage(uint8_t* inputImage,uint8_t* outputImage, uint8_t width, uint8_t height, int rotateTo)
{
    if (rotateTo != 90 && rotateTo != 180 && rotateTo != 270)
    {
        Log_Debug("RotateImage, invalid rotation value %d\n",rotateTo);
        return;
    }

    if (width != height)
    {
        Log_Debug("RotateImage: Width/Height must be equal\n");
        return;
    }

    if (rotateTo == 90)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < height; x++)
            {
                if (IsPixel(inputImage, width, height, x, y))
                {
                    SSD1306_SetPixel(outputImage, width, height, height-1-y , x, IsPixel(inputImage, width, height, x, y));
                }
            }
        }
    }

    if (rotateTo == 180)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < height; x++)
            {
                if (IsPixel(inputImage, width, height, x, y))
                {
                    SSD1306_SetPixel(outputImage, width, height, width-1-x , height-1-y , IsPixel(inputImage, width, height, x, y));
                }
            }
        }
    }

    if (rotateTo == 270)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < height; x++)
            {
                if (IsPixel(inputImage, width, height, x, y))
                {
                    SSD1306_SetPixel(outputImage, width, height, y , width-x-1, IsPixel(inputImage, width, height, x, y));
                }
            }
        }
    }
}

static bool IsPixel(uint8_t* image, int width, int height, int x, int y)
{
    uint8_t bMap[8] = { 128,64,32,16,8,4,2,1 };
    int bytesPerLine = width / 8;
    int xOffset = x / 8;
    int xRemain = x % 8;    // bit position
    int byteOffset = (y * bytesPerLine) + xOffset;

    uint8_t val = image[byteOffset];
    if (val & bMap[xRemain])
    {
        return true;
    }

    return false;
}

void SSD1306_Clear(void)
{
    memset(&displayBuffer[0], 0x00, 1024);
}

void SSD1306_SetPixel(uint8_t*image, int width, int height, int x, int y, bool turnOn)
{
    if (x >= 0 && y >= 0 && x < width && y < height)
    {
        static const unsigned int mask[] = { 128,64,32,16,8,4,2,1 };
        int Pos = ((width / 8) * y) + (x / 8);
        int pixel = x % 8;
        int bitMask = mask[pixel];


        if (turnOn)
        {
            image[Pos] |= bitMask;
        }
        else
        {
            image[Pos] &= ~bitMask;
        }
    }
}

static void ssd1306_command(uint8_t c)
{
    uint8_t command[2] = { 0x00, c }; // some use 0X00 other examples use 0X80. I tried both 
    i2cSendBytes(command, 2);
}

void SSD1306_Display(void)
{
    uint8_t pagecol[5] = { 0x22,0x00,0xff,0x21,0x00 };
    ssd1306_commands(pagecol, 5);
    ssd1306_command(0x7f);

    uint8_t dataBuffer[32];
    dataBuffer[0x00] = 0x40;
    int copyPos = 0;
    int copyLen = 31;
    bool haveData = true;
    int totalSent = 0;

    int totalBytes = 1024;

    //DumpDisplayBuffer(displayBuffer,DisplayWidth, DisplayHeight);

    while (haveData)
    {
        memcpy(&dataBuffer[1], &displayBuffer[copyPos], copyLen);
        totalSent += copyLen;
        i2cSendBytes(dataBuffer, copyLen + 1);
        copyPos += copyLen;
        if (totalBytes - copyPos >= 31)
            copyLen = 31;
        else
        {
            copyLen = totalBytes - copyPos;
        }

        if (copyPos >= totalBytes)
            haveData = false;
    }
}

static void i2cSendBytes(uint8_t* data, size_t length)
{
    I2CMaster_Write(_i2cfd, oledDisplayAddress, data, length);
    delay(1);
}

void SSD1306_FillRegion(uint8_t *image, int width, int height, int x, int y, int regionWidth, int regionHeight, bool turnOn)
{
    for (int n = y; n < y + regionHeight; n++)
    {
        for (int o = x; o < x + regionWidth; o++)
        {
            SSD1306_SetPixel(image, width, height, o, n, turnOn);
        }
    }
}