#include "main.h"
#include "pca9538.h"
#include "kb.h"

#define KBRD_RD_ADDR 0xE3
#define KBRD_WR_ADDR 0xE2


uint8_t check_row(uint8_t Nrow) {
 uint8_t buf = 0;
 uint8_t kb_in;

 buf = Nrow;
 PCA9538_Write_Register(KBRD_WR_ADDR, CONFIG, &buf);

 PCA9538_Write_Register(KBRD_WR_ADDR, OUTPUT_PORT, &buf);

 buf = 0;
 PCA9538_Read_Inputs(KBRD_RD_ADDR, &buf);

 kb_in = buf & 0x70;
 return 0x0F ^ (kb_in >> 4);
}
