#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>

// Initializes Port H (PH0-PH3)
void PortH_Init(void);

// Basic step functions
void spin_step_forward(void);
void spin_step_backward(void);

// The "Tangle-Free" function: reverses the total rotation
void stepper_unwind(uint32_t total_steps);

//Turn off motor
void MotorOff(void);
#endif