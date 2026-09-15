#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

// Global var to start scanning
extern volatile uint32_t startScanFlag;

// Initializes PJ0 with a falling-edge interrupt
void PortJ_Interrupt_Init(void);

// The Interrupt Service Routine (ISR)
void GPIOJ_IRQHandler(void);

#endif