#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "include/util.h"

void micro_wait(unsigned int n) {
    asm(    "        movs r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n*1000) : "r0", "cc");
}

void tim6_init() {
	// enable system clock for TIM6
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	// configure timer with duration 100μs
	TIM6->PSC = 48-1;
	TIM6->ARR = 100-1;
//	// enable user interrupt
//	TIM6->DIER |= TIM_DIER_UIE;
//	// enable interrupt in NVIC
//	NVIC->ISER |= 1 << TIM6_DAC_IRQn;
}

void tim6_start() {
	// enable TIM6
	TIM6->CR1 |= TIM_CR1_CEN;
}

int tim6_stop() {
	// read TIM6 counter
	int micro_elapsed = TIM6->ARR - TIM6->CNT;
	// disable TIM6
	TIM6->CR1 &= ~TIM_CR1_CEN;
	TIM6->CNT = TIM6->ARR;
	return micro_elapsed;
}

int value_check(int actual, int expected, int tolerance) {
	if(actual < (expected - tolerance))
		return 0;
	if(actual > (expected + tolerance))
		return 0;
	return 1;
}
