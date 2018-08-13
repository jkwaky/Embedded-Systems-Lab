// Switch.c
// Runs on TMC4C123
// Use GPIO in edge time mode to request interrupts on any
// edge of PF4 and start Timer0B. In Timer0B one-shot
// interrupts, record the state of the switch once it has stopped
// bouncing.
// Daniel Valvano
// May 3, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// PF4 connected to a negative logic switch using internal pull-up (trigger on both edges)
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#define PF4                     (*((volatile uint32_t *)0x40025040))
#define PB0 										(*((volatile uint32_t *)0x40005004))
#define PD0										  (*((volatile uint32_t *)0x40007004))
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

extern uint8_t SongState; //keeps track of if song is playing or not
extern uint8_t k; //keeps track of which song we are on
uint8_t ReverseState = 0; //keeps track of if the reverse button was the most recent button pressed

volatile static unsigned long Touch;     // true on touch
volatile static unsigned long Release;   // true on release
volatile static unsigned long Last;      // previous
volatile static unsigned long LastB;
volatile static unsigned long LastD;
void (*TouchTask)(void);    // user function to be executed on touch
void (*ReleaseTask)(void);  // user function to be executed on release
void (*UselessTask)(void);
void (*PlayTask)(void);
void (*ReverseTask)(void);
void (*PauseTask)(void);
static void Timer0Arm(void){
  TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x0000001;    // 3) 1-SHOT mode
  TIMER0_TAILR_R = 160000;      // 4) 10ms reload value
  TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // 8) priority 4
  NVIC_EN0_R |= 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
}

static void GPIOBArm(void){
  GPIO_PORTB_ICR_R = 0x1;      // (e) clear flag4
  GPIO_PORTB_IM_R |= 0x1;      // (f) arm interrupt on PF4 and PF0*** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFF0FFF)|0x0000A000; // (g) priority 5
  NVIC_EN0_R |= 0x00000002;      // (h) enable interrupt 1 in NVIC  
}

static void GPIODArm(void){
  GPIO_PORTD_ICR_R = 0x1;      // (e) clear flag4
  GPIO_PORTD_IM_R |= 0x1;      // (f) arm interrupt on PF4 and PF0*** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0x1FFFFFFF)|0xA0000000; // (g) priority 5
  NVIC_EN0_R |= 0x00000008;      // (h) enable interrupt 1 in NVIC  
}




 void SwitchB_Init(void(*touchtask)(void), void(*releasetask)(void)){
  // **** general initialization ****
  SYSCTL_RCGCGPIO_R |= 0x00000002; // (a) activate clock for port B
  while((SYSCTL_PRGPIO_R & 0x00000002) == 0){};
  GPIO_PORTB_DIR_R &= ~0x1;    // (c) make PB0 in (built-in button)
  GPIO_PORTB_AFSEL_R &= ~0x1;  //     disable alt funct on PB0
  GPIO_PORTB_DEN_R |= 0x1;     //     enable digital I/O on PF4  and PF0 
  GPIO_PORTB_PCTL_R &= ~0x0000000F; // configure PF4 and PF0 as GPIO
  GPIO_PORTB_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTB_IS_R &= ~0x1;     // (d) PF4 and PF0 is edge-sensitive
	GPIO_PORTB_IBE_R &= ~0x1;    //     PF4 is not both edges
  GPIO_PORTB_IEV_R &= ~0x1;    //     PF4 falling edge event
  GPIOBArm();

  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  PlayTask = touchtask;      // user function 
	PauseTask = releasetask;

	//LastB2 = PB2;		// initial switch state
 }

 void SwitchD_Init(void(*touchtask)(void)){
  // **** general initialization ****
  SYSCTL_RCGCGPIO_R |= 0x00000008; // (a) activate clock for port B
  while((SYSCTL_PRGPIO_R & 0x00000008) == 0){};
  GPIO_PORTD_DIR_R &= ~0x1;    // (c) make PB0 in (built-in button)
  GPIO_PORTD_AFSEL_R &= ~0x1;  //     disable alt funct on PB0
  GPIO_PORTD_DEN_R |= 0x1;     //     enable digital I/O on PF4  and PF0 
  GPIO_PORTD_PCTL_R &= ~0x000F000F; // configure PF4 and PF0 as GPIO
  GPIO_PORTD_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTD_IS_R &= ~0x1;     // (d) PF4 and PF0 is edge-sensitive
	GPIO_PORTD_IBE_R &= ~0x1;    //     PF4 is not both edges
  GPIO_PORTD_IEV_R &= ~0x1;    //     PF4 falling edge event
	SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  GPIODArm();
  ReverseTask = touchtask;      // user function 
 }
 
 

void GPIOPortB_Handler(void){
  GPIO_PORTB_IM_R &= ~0x1;     // disarm interrupt on PF4 
  if(SongState){    // 0x10 means it was previously released
		Timer0Arm();
    (*PauseTask)();  // execute user task
		SongState = 0;
  }
	else{
		Timer0Arm();
		(*PlayTask)();
		SongState = 1;
}
}


//checks if reverse button was the most recent button 
//pressed and if it was then it goes to next song and if it wasnt then it restarts the song
void GPIOPortD_Handler(void){ 
  GPIO_PORTD_IM_R &= ~0x1;     // disarm interrupt on PF4 
if(!ReverseState){
		(*ReverseTask)();

  Timer0Arm(); // start one shot
	ReverseState = 1;
}
else{
	
	k++;
	k = k%3;
}
}



void Timer0A_Handler(void){
  TIMER0_IMR_R = 0x00000000;    // disarm timeout interrupt
	GPIOBArm();
	GPIODArm();
}
