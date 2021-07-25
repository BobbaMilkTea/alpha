#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>

#include "display.h"

// This array will be used with dma_display1() and dma_display2() to mix
// commands that set the cursor location at zero and 64 with characters.
//
uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

void _send_cmd(char b) {
	while((SPI1->SR & SPI_SR_TXE) == 0) {
    	;
    }
	SPI1->DR = b;
}

void _send_data(char b) {
	while((SPI1->SR & SPI_SR_TXE) == 0) {
		;
	}
	SPI1->DR = 0x200 + b;
}

void init_display() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~(GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER7_0);
	GPIOA->MODER |= GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1;
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL5 | GPIO_AFRL_AFRL7);
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_1 | SPI_CR1_BR_0; //highest baud rate is fPCLK/16 or 250 kHz
	SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
	SPI1->CR2 |= SPI_CR2_DS_3 | SPI_CR2_DS_0 | SPI_CR2_SSOE | SPI_CR2_NSSP;
	SPI1->CR1 |= SPI_CR1_SPE;
}

void _dma_display_transfer() {
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	DMA1_Channel5->CCR &= ~DMA_CCR_EN;
	DMA1_Channel5->CMAR = (uint32_t)dispmem;
	DMA1_Channel5->CPAR = &(SPI1->DR);
	DMA1_Channel5->CNDTR = sizeof dispmem;
	DMA1_Channel5->CCR |= DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_DIR;
	DMA1_Channel5->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE | DMA_CCR_PL);
	SPI1->CR2 |= SPI_CR2_TXDMAEN;
	DMA1_Channel5->CCR |= DMA_CCR_EN;
}

void dma_display1(const char *s) {
	_send_cmd(0x80 + 0);
	int x;
	for(x=0; x<16; x+=1)
		if (s[x])
			dispmem[x+1] = s[x] | 0x200;
		else
			break;
	for(   ; x<16; x+=1)
		dispmem[x+1] = 0x220;
	_dma_display_transfer();
}

void dma_display2(const char *s) {
	_send_cmd(0x80 + 0);
	int x;
	for(x=17; x<33; x+=1)
		if (s[x])
			dispmem[x+1] = s[x] | 0x200;
		else
			break;
	for(   ; x<33; x+=1)
		dispmem[x+1] = 0x220;
	_dma_display_transfer();
}
