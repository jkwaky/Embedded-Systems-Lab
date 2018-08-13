#include "button.h"
#include "../inc/tm4c123gh6pm.h"
#include <stdint.h>

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
//#define PF3                     (*((volatile uint32_t *)0x40025020))
//#define PF2                     (*((volatile uint32_t *)0x40025010))
//#define PF1                     (*((volatile uint32_t *)0x40025008))

int buttonStates = 0;
static void Timer3Arm(void){
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x0000001;    // 3) 1-SHOT mode
  TIMER3_TAILR_R = 320000;      // 4) 10ms reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN1_R |= 1<<3;           // 9) enable IRQ 19 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER0A
}

static void GPIOArm(void){
  GPIO_PORTF_ICR_R = 0xA;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0xA;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC  
}

void buttonInit(void){
	SYSCTL_RCGCGPIO_R |= 0x00000020; // activate clock for port F
  while((SYSCTL_PRGPIO_R & 0x00000020) == 0){};
	GPIO_PORTF_LOCK_R = 0x4C4F434B;
		GPIO_PORTF_CR_R = 0x1F;
  GPIO_PORTF_DIR_R &= ~0x0A;     // PF3,PF2,PF1,PF0 inputs
  GPIO_PORTF_DEN_R |= 0xA;     // enable digital I/O on PF1,PF2
	GPIO_PORTF_PUR_R = 0;
  GPIO_PORTF_AFSEL_R &= ~0x0A;  //     disable alt funct on PF1,PF2 
  GPIO_PORTF_PCTL_R &= ~0x0000F0F0; // configure PF1, PF0 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  //GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x0A;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R |= 0x0A;     //     PF4 is both edges
  GPIOArm();

  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
//  TouchTask = touchtask;           // user function 
  //ReleaseTask = releasetask;       // user function
}

void GPIOPortF_Handler(void){
	GPIO_PORTF_IM_R &= ~0xA; //add and operations
	
	Timer3Arm(); //start debounce timer
	
}

int getState(void){
	return buttonStates;
}

void resetState(void){
	buttonStates = 0;
}

void Timer3A_Handler(void){
	TIMER3_IMR_R = 0x00000000;    // disarm timeout interrupt
	buttonStates |= GPIO_PORTF_DATA_R;
  GPIOArm();   // start GPIO
}
