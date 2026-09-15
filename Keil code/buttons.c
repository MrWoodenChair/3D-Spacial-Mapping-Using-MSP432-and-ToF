#include "tm4c1294ncpdt.h"
#include "buttons.h"

// Initialize scanning flag
volatile uint32_t startScanFlag = 0;

void PortJ_Interrupt_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8; 	//Enable clk
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0){}; 
    
    // 2. Configure PJ0 as digital input with pull-up
    GPIO_PORTJ_DIR_R &= ~0x01;    // Set PJ0 as input
    GPIO_PORTJ_DEN_R |= 0x01;     // Digital enable PJ0
    GPIO_PORTJ_PUR_R |= 0x01;     // Enable weak pull-up resistor
    
    // 3. Configure Interrupt for Falling Edge (Button Press)
    GPIO_PORTJ_IS_R &= ~0x01;     // Edge-sensitive
    GPIO_PORTJ_IBE_R &= ~0x01;    // Interrupt controlled by IEV
    GPIO_PORTJ_IEV_R &= ~0x01;    // Falling edge triggers the interrupt
    GPIO_PORTJ_ICR_R = 0x01;      // Clear any prior interrupt flags
    GPIO_PORTJ_IM_R |= 0x01;      // Unmask (enable) interrupt for PJ0
    
    // 4. Set Priority in NVIC (Priority set to 2)
    // Port J is Interrupt #51. Vector is in PRI12 (bits 29:31). Prioirity of 2 means (010) at the top 3 bits of the byte
    NVIC_PRI12_R = (NVIC_PRI12_R & 0x00FFFFFF) | 0x40000000; 
    
    // 5. Enable Interrupt #51 in NVIC
    NVIC_EN1_R |= 0x00080000;     // Bit 19 of EN1 corresponds to Int #51
}

// The Actual Interrupt Handler
void GPIOJ_IRQHandler(void){
    GPIO_PORTJ_ICR_R = 0x01;      // Acknowledge the interrupt (CRITICAL)
    startScanFlag = 1;            // Set flag for main.c
}