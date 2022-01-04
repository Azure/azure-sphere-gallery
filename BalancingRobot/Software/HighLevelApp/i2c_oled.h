#ifndef _SPI_OLED_DISPLAY
#define _SPI_OLED_DISPLAY

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool SSD1306_Init(bool useVerticalDisplay);
void SSD1306_Display(void);
void SSD1306_Clear(void);
void SSD1306_FillRegion(uint8_t* image, int width, int height, int x, int y, int regionWidth, int regionHeight, bool turnOn);
void SSD1306_SetPixel(uint8_t *image, int width, int height, int x, int y, bool turnOn);
void SSD1306_DrawImage(uint8_t* image, int width, int height, int xOffset, int yOffset);
void SSD1306_RotateImage(uint8_t* inputImage, uint8_t* outputImage, uint8_t width, uint8_t height, int rotateTo);

#endif