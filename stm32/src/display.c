#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>
#include "display.h"

#define DISP_RS GPIO_ODR_1
#define DISP_RW GPIO_ODR_2
#define DISP_EN GPIO_ODR_3

static void _micro_wait(unsigned int n) {
    asm(    "        movs r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n*1000) : "r0", "cc");
}

static void _send_parallel_four_bit(char b, int mode) {
	GPIOA->ODR &= ~(DISP_RS);
	GPIOA->ODR = (mode << 1) | ((b & 0x0f) << 4);
	GPIOA->ODR |= DISP_EN;
	_micro_wait(1000);
	GPIOA->ODR &= ~DISP_EN;
}

static void _send_cmd(char b) {
	_send_parallel_four_bit(b>>4, 0);
	_send_parallel_four_bit(b, 0);
	_micro_wait(10000);
}

static void _send_data(char b) {
	_send_parallel_four_bit(b>>4, 1);
	_send_parallel_four_bit(b, 1);
	_micro_wait(100);
}

static void _shift_cursor(int row, int col)
{
    if (row == 0)
    	col |= 0x80;
    if (row == 1)
        col |= 0xC0;
    _send_cmd(col);
}


static void _send_init_display_cmd() {
	// configure 4-bit mode
	_micro_wait(50000);
	_send_parallel_four_bit(0x3, 0);
	_send_parallel_four_bit(0x3, 0);
	_send_parallel_four_bit(0x3, 0);
	_send_parallel_four_bit(0x2, 0);
	// configure control bits
	_send_cmd(0x6);
	_send_cmd(0x2);
	_send_cmd(0xc);
}

void display_init() {
	// enable clock for GPIOA
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	// set PA0-7 to output mode
	GPIOA->MODER &= ~(GPIO_MODER_MODER1_1 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);
	GPIOA->MODER |= GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;
	// set to write mode: 0
	GPIOA->ODR &= ~DISP_RW;
	_send_init_display_cmd();
}

void _clear_row(int row) {
	_shift_cursor(row, 0);
	for (int i = 0; i < 16; i++) {
		_send_data(' ');
	}
}

void display_msg(const char* text, int row) {
	_clear_row(row);
	_shift_cursor(row, 0);

	int i = 0;
	while ((*text != '\0') && (i++ < 16)) {
		_send_data(*(text++));
	}
}
