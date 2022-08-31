#include "r503_uart.h"


#include "driver/uart.h"

// base write and receive functions
int r503_init(uint32_t adder);
int __r503_write(uint8_t* data, uint16_t len);
int __r503_read(uint8_t* data, uint16_t* len);
