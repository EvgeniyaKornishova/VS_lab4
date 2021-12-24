#include "oled.h"
#include "i2c.h"

static uint8_t OLED_Buffer[1024];

static OLED_t OLED;

// write I2C command to OLED display
static void oled_WriteCommand(uint8_t command) {
	HAL_I2C_Mem_Write(&hi2c1,OLED_I2C_ADDR,0x00,1,&command,1,10);
}

// initialize OLED display
uint8_t oled_Init(void) {
	HAL_Delay(100);

	oled_WriteCommand(0xAE); // Disable display
	oled_WriteCommand(0x20); // Memory addressing mode
	oled_WriteCommand(0x10); // ?
	oled_WriteCommand(0xB0); // ?!
	oled_WriteCommand(0xC8); // COM signal mapping inversion (PAGE0: COM63 - COM56)
	oled_WriteCommand(0x00); // ?!
	oled_WriteCommand(0x10); // ?!
	oled_WriteCommand(0x40); // Start string is 0
	oled_WriteCommand(0x81); // Contrast -
	oled_WriteCommand(0xFF); //				256
	oled_WriteCommand(0xA1); // SEG signal mapping inversion (SEG0 = column 127)
	oled_WriteCommand(0xA6); // Normal color (not inverted)
	oled_WriteCommand(0xA8); // COM multiplexor setting
	oled_WriteCommand(0x3F); //							(max ratio = 64)
	oled_WriteCommand(0xA4); // Display video-memory content
	oled_WriteCommand(0xD3); // Vertical shift -
	oled_WriteCommand(0x00); //					0
	oled_WriteCommand(0xD5); // Sync signal setting
	oled_WriteCommand(0xF0); //						(DCLK divide ratio = 1, Fosc = 15)
	oled_WriteCommand(0xD9); // Precharging setting
	oled_WriteCommand(0x22); //						(phase1 = 2, phase2 = 2)
	oled_WriteCommand(0xDA); // COM signal configuration setting
	oled_WriteCommand(0x12); // ?
	oled_WriteCommand(0xDB); // Voltage setting
	oled_WriteCommand(0x20); // 				Vcomh = ~0.77 x Vcc
	oled_WriteCommand(0x8D); // Enable embedded voltage source
	oled_WriteCommand(0x14); //									(charge pump)
	oled_WriteCommand(0xAF); // Enable display

	oled_Fill(Black);	// fill OLED display with black

	oled_UpdateScreen(); // Draw image from video-memory to the screen

	OLED.CurrentX = 0;
	OLED.CurrentY = 0;

	OLED.Initialized = 1;

	return 1;
}

// fill screen with White or Black color
void oled_Fill(OLED_COLOR color) {
	uint32_t i;

	for(i = 0; i < sizeof(OLED_Buffer); i++) {
		OLED_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
	}
}

// draw image from video-memory to the screen
void oled_UpdateScreen(void) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		oled_WriteCommand(0xB0 + i);
		oled_WriteCommand(0x00);
		oled_WriteCommand(0x10);

		HAL_I2C_Mem_Write(&hi2c1,OLED_I2C_ADDR,0x40,1,&OLED_Buffer[OLED_WIDTH * i],OLED_WIDTH,25);
	}
}

// fill a single pixel (x:y) with color (Black or White)
void oled_DrawPixel(uint8_t x, uint8_t y, OLED_COLOR color) {
	if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
		return;
	}

	if (OLED.Inverted) {
		color = (OLED_COLOR)!color;
	}

	if (color == White) {
		OLED_Buffer[x + (y / 8) * OLED_WIDTH] |= 1 << (y % 8);
	} else {
		OLED_Buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
	}
}

// draw a horizontal line (x1:y, x2:y) with color (White or Black)
void oled_DrawHLine(uint8_t x1, uint8_t x2, uint8_t y, OLED_COLOR color) {
	for(int i = x1; i <= x2; i++) {
		oled_DrawPixel(i, y, color);
	}
}

// draw a vertical line (x:y1, x:y2) with color (White or Black)
void oled_DrawVLine(uint8_t y1, uint8_t y2, uint8_t x, OLED_COLOR color) {
	for(int i = y1; i <= y2; i++) {
		oled_DrawPixel(x, i, color);
	}
}

// draw a square (x1:y1, x2:y1, x2:y2, x1:y2) with color (White or Black)
void oled_DrawSquare(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, OLED_COLOR color) {
	oled_DrawHLine(x1, x2, y1, color);
	oled_DrawHLine(x1, x2, y2, color);
	oled_DrawVLine(y1, y2, x1, color);
	oled_DrawVLine(y1, y2, x2, color);
}

// draw a char <ch> with size <Font> and color (White or Black)
char oled_WriteChar(char ch, FontDef Font, OLED_COLOR color) {
	uint32_t i, b, j;

	if (OLED_WIDTH <= (OLED.CurrentX + Font.FontWidth) ||
			OLED_HEIGHT <= (OLED.CurrentY + Font.FontHeight)) {
		return 0;
	}

	for (i = 0; i < Font.FontHeight; i++) {
		b = Font.data[(ch - 32) * Font.FontHeight + i];
		for (j = 0; j < Font.FontWidth; j++) {
			if ((b << j) & 0x8000) {
				oled_DrawPixel(OLED.CurrentX + j, (OLED.CurrentY + i), (OLED_COLOR) color);
			} else {
				oled_DrawPixel(OLED.CurrentX + j, (OLED.CurrentY + i), (OLED_COLOR)!color);
			}
		}
	}

	OLED.CurrentX += Font.FontWidth;

	return ch;
}

// draw a string <str> with size <Font> and color (White or Black)
char oled_WriteString(char* str, FontDef Font, OLED_COLOR color) {
	while (*str) {
		if (oled_WriteChar(*str, Font, color) != *str) {
			return *str;
		}
		str++;
	}
	return *str;
}

// move cursor to point (x:y)
void oled_SetCursor(uint8_t x, uint8_t y) {
	OLED.CurrentX = x;
	OLED.CurrentY = y;
}

// draw a bitmat <bmp> from coordinates (x,y) with size (width:height)
void oled_DrawBitmap(uint8_t* bmp, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
	for (int h = 0; h < height; h++) {
		for (int w = 0; w < width; w++) {
			oled_DrawPixel(w+x, h+y, bmp[w+h*width]);
		}
	}
}
