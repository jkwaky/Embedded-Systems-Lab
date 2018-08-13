// EdgeInterrupt.c
// Runs on LM4F120 or TM4C123
// Request an interrupt on the falling edge of PF4 (when the user
// button is pressed) and increment a counter in the interrupt.  Note
// that button bouncing is not addressed.
// Daniel Valvano
// May 3, 2015

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015
   Volume 1, Section 9.5
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Volume 2, Section 5.5

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

// user button connected to PF4 
// call Fall() on falling edge (touch SW1)
// call Rise() on rising edge (release SW1)

#include <stdint.h>
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"
#include "Switch.h"
#include "MAX5353.h"
#include "SysTick.h"
#include "Song.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
#define PF3                     (*((volatile uint32_t *)0x40025020))
#define PF2                     (*((volatile uint32_t *)0x40025010))
#define PF1                     (*((volatile uint32_t *)0x40025008))
uint32_t RiseCount,FallCount;
uint8_t j = 0; //Where you are in the wave
uint8_t i = 0; //Where you are in the song
uint8_t k = 0; //what song are you on
uint8_t SongState = 0;
extern uint8_t ReverseState;


void (*SongFunction)(void);
void (*NoteFunction)(void);

void Timer2_Init(void(*task)(void), unsigned long period){
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  SongFunction = task;          // user function
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R |= 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

void Timer2A_Handler(void){
  TIMER2_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER2A timeout
  (*SongFunction)();                // execute user task
}

void Timer1_Init(void(*task)(void), uint32_t period){
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  NoteFunction = task;          // user function
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period-1;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  NVIC_EN0_R |= 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}

void Timer1A_Handler(void){
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
  (*NoteFunction)();                // execute user task
}



void SongIncrement(void){ //Goes to the next note in the song
	//TIMER1_IMR_R = 0x00000000;
	//TIMER2_IMR_R = 0x00000000;
	//TIMER1_TAILR_R = Korobeiniki[i].pitch; //sets the reload to the needed frequency
	//TIMER2_TAILR_R = Korobeiniki[i].duration; //sets how long the note needs to play for
	//TIMER1_TAILR_R = SongOfStorms[i].pitch; //sets the reload to the needed frequency
	//TIMER2_TAILR_R = SongOfStorms[i].duration/2; //sets how long the note needs to play for
	//TIMER1_TAILR_R = OdeToJoy[i].pitch; //sets the reload to the needed frequency
	//TIMER2_TAILR_R = OdeToJoy[i].duration/2; //sets how long the note needs to play for
	TIMER1_TAILR_R = SongList[k][i].pitch; //sets the reload to the needed frequency
	TIMER2_TAILR_R = SongList[k][i].duration; //sets how long the note needs to play for
	
	
	i++;
	//i &= 63;
	i = i%SongList[k]->length;
	//TIMER1_IMR_R = 0x00000001;
	//TIMER2_IMR_R = 0x00000001;
}

void NoteIncrement(void){ //Sets DAC to specific voltage from instrument array array
	j++;
	j &= 31;
	DAC_Out(Flute[j]);
}


void Nothing(void){} //A function that does nothing
	
	
	
void Stop(void){
	TIMER2_IMR_R = 0x00000000;
	TIMER1_IMR_R = 0x00000000;
	ReverseState = 0;
}

void Play(void){
	//TIMER1_TAILR_R = OdeToJoy[i].pitch; //sets the reload to the needed frequency
	//TIMER2_TAILR_R = OdeToJoy[i].duration; //sets how long the note needs to play for
	//TIMER1_IMR_R = 0x00000001;
	//TIMER2_IMR_R = 0x00000001;
	ReverseState = 0;
	Timer2_Init(&SongIncrement, 8000000);
	Timer1_Init (&NoteIncrement, 8000000);
}

void Rewind(void){
	TIMER2_IMR_R = 0x00000000;
	TIMER1_IMR_R = 0x00000000;
	j = 0;
	i = 0;
	SongState = 0;
}



//debug code
int main(void){ 
	PLL_Init(Bus80MHz);
	DisableInterrupts();
	SwitchD_Init(&Rewind); //Initialize Rewind Button
	SwitchB_Init(&Play, &Stop); //Initialize Play button
	Timer2_Init(&SongIncrement, 80000000);
	TIMER2_CTL_R = 0x00000000;
	Timer1_Init (&NoteIncrement, 80000000);
	TIMER1_CTL_R = 0x00000000;
	//SysTick_Init();
	DAC_Init(1007);
  EnableInterrupts();           // interrupt after all initialization

  while(1){}
		
	}
