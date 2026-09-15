#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "stepper.h"
#include "buttons.h"


#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up
#define FULL_ROTATION_STEPS     512		//Every motor rotation func takes 4 steps (2048=Total # of steps)(2048/4=512)
#define MEASUREMENTS_PER_TURN   32
#define STEPS_PER_MEASUREMENT   (FULL_ROTATION_STEPS / MEASUREMENTS_PER_TURN)
#define NUM_OF_SCAN_LAYERS 3

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//======ENABLE INTERRUPTS=====
void EnableInt(void)
{    __asm("    cpsie   i\n");
}

// Disable interrupts
void DisableInt(void)
{    __asm("    cpsid   i\n");
}

// Low power wait
void WaitForInt(void)
{    __asm("    wfi\n");
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}


//*********************************************************************************************************
//*********************************************************************************************************
//***********					MAIN Function				*****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void) {

	//initialize
	PLL_Init();			//Set clk to 26MHz
	SysTick_Init();		//Initalize delay func
	onboardLEDs_Init();		//Inialize onboard LED funcs
	I2C_Init();			//Inialize UART for PC communication
	UART_Init();		
	PortH_Init();		//Initialize stepper motor data pins
	PortJ_Interrupt_Init();		//Initialize Button PJ0 with Interrupt
	EnableInt();			//Enables global interrupt
	PortG_Init();
	VL53L1X_XSHUT();   // HARD RESET SENSOR
	
	//======AD3 TESTING======
	//COMMENT OUT WHEN SCANNING!!!!!!
//	while(1)
//	{
//		GPIO_PORTF_DATA_R^=0x02;	//toggle PF1
//		SysTick_Wait(200000);
//	}
	
	
	//ToF Sensor Initialize
	uint8_t sensorState = 0;
	uint16_t wordData;    // Used once to check Sensor ID
	uint16_t Distance;    // The actual measurement in mm
	uint8_t dataReady;    // Flag for the Polling loop
	
	//Wait for device to boot
	while(sensorState == 0){
        VL53L1X_BootState(dev, &sensorState);
        SysTick_Wait10ms(10);
		}
	status = VL53L1X_SensorInit(dev);
	status = VL53L1X_StartRanging(dev);
		
	//SYSTEM READY INDICATOR (Additional Status)
	UART_printf("System Ready\r\n");
	FlashLED1(); // PN1(LED 1) Flash: Indicates button is ready to press
	SysTick_Wait10ms(50);
	FlashLED1();
		
	while(1) {
		// start scan flag from interrupt handler in buttons.c
		//Triggered by PJ0 (BUTTON 0)
		if(startScanFlag) 
			{
				UART_printf("START\r\n");
				SysTick_Wait10ms(50);
				uint32_t virtual_z = 0; //Initial z displacement
				
				// Doing 3 full rotations
				for(int ring = 0; ring < NUM_OF_SCAN_LAYERS; ring++) {
						
						//2048 steps = 360 degrees (full-step + gear ratio)
						//i=1 means 4 steps =0.703deg
						for(int i = 0; i < FULL_ROTATION_STEPS; i++) {
								
							//1st Step Motor Forward
								spin_step_forward();
							
							//2nd Measure occasionally (Steps per measurement)
							if(i % STEPS_PER_MEASUREMENT == 0) {
		
								// Timeout/non-blocking wait
								int timeout = 1000;
								dataReady = 0;
								
								//Wait for ToF Data
								while(dataReady == 0 && timeout--){
										VL53L1X_CheckForDataReady(dev, &dataReady);
								}
								
								//3rd COllect and print data
								if(dataReady)
								{
									//Read Distance(PF0(LED4) toggles during measurement)
									VL53L1X_GetDistance(dev, &Distance);
									FlashLED4();
									VL53L1X_ClearInterrupt(dev);
									
									// Compute angle
									float angle = ((float)i / FULL_ROTATION_STEPS) * 360.0;
						
									//Transmit Data(PN0(LED2) toggles during UART)
									sprintf(printf_buffer, "%d, %.2f, %u\r\n", virtual_z, angle, Distance);
									FlashLED2();
									UART_printf(printf_buffer);
								}
							}
						}
						
						// --- UNWIND WIRES(360 degrees back) ---
            stepper_unwind(FULL_ROTATION_STEPS); // Spin 512 steps backward to return to 0 deg
            
            // --- INCREMENT z VALUE AND PREP FOR NEXT SCAN ---
            virtual_z += 10;            // Increment pretend height
            GPIO_PORTN_DATA_R &= ~0x02; // PN1(LED 0) OFF
            SysTick_Wait10ms(50);       // Short 0.5s pause for stability
				}
				startScanFlag = 0; // Reset flag
				MotorOff();		//Turn motor off
				UART_printf("END\r\n");
			}
		}
	}