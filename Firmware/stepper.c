// COMPENG 2DX3
// This program illustrates the interfacing of the Stepper Motor with the microcontroller
// Written by Ama Simons
// January 18, 2020
// Revised for Winter 2026 Deliverable 1

#include "tm4c1294ncpdt.h"
#include "stepper.h"
#include "SysTick.h"

//-----Enable Port H for Stepper Moter control pins (3:0)-----
void PortH_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};
    GPIO_PORTH_DIR_R |= 0x0F;
    GPIO_PORTH_AFSEL_R &= ~0x0F;
    GPIO_PORTH_DEN_R |= 0x0F;
    GPIO_PORTH_AMSEL_R &= ~0x0F;
}

//----MOTOR SPIN FUNCTIONS----
void spin_step_forward(void){                                                                           
    uint32_t delay = 1;	//1ms at 26MHz
		// Anticlockwise
		GPIO_PORTH_DATA_R = 0b00001001; SysTick_Wait10ms(delay);
		GPIO_PORTH_DATA_R = 0b00001100; SysTick_Wait10ms(delay);
		GPIO_PORTH_DATA_R = 0b00000110; SysTick_Wait10ms(delay);
		GPIO_PORTH_DATA_R = 0b00000011; SysTick_Wait10ms(delay);
}
void spin_step_backward(void){                                                                           
    uint32_t delay = 1;	//1ms at 26MHz
		// Anticlockwise
		GPIO_PORTH_DATA_R = 0b00000011; SysTick_Wait10ms(delay);
    GPIO_PORTH_DATA_R = 0b00000110; SysTick_Wait10ms(delay);
    GPIO_PORTH_DATA_R = 0b00001100; SysTick_Wait10ms(delay);
    GPIO_PORTH_DATA_R = 0b00001001; SysTick_Wait10ms(delay);
}

//----UNWINDING FUNCTION----
void stepper_unwind(uint32_t total_steps){
    for(uint32_t i = 0; i < total_steps; i++){
        spin_step_backward();
    }
}

//==TURN OFF MOTOR==
void MotorOff(void){                                                                           
    uint32_t delay = 1;	//1ms at 26MHz
		// Anticlockwise
		GPIO_PORTH_DATA_R = 0b00000000; SysTick_Wait10ms(delay);
}