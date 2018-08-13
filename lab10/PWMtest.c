// PWMtest.c
// Runs on TM4C123
// Use PWM0/PB6 and PWM1/PB7 to generate pulse-width modulated outputs.
// Daniel Valvano
// March 28, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 6.8, section 6.3.2

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015
   Program 8.4, Section 8.3

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
#include <stdint.h>
#include "PLL.h"
#include "Motor.h"
#include "Tach.h"
#include "button.h"
#include "ST7735.h"

void WaitForInterrupt(void);  // low power mode
extern uint32_t RPSaverage;
int32_t RevPerSec;
int32_t TargetRev = 300;
uint16_t Duty = 3000;
int32_t rateOfChange = 50;
uint32_t localaverage;
char adcConverter[5];

char* adcToString(int m){//returns a string of the 3 digit integer
    adcConverter[4] = 0;

    int one = 0;
    int ten = 0;
    int hundred =  0;
    one = m % 10;
    m = m/10;
    ten = m % 10;
    m = m/10;
    hundred = m % 10;
    if(hundred == 0){
        if(ten == 0){
            adcConverter[0] = (32);
            adcConverter[1] = (32);
            adcConverter[2] = ('.');
            adcConverter[3] = (one + 48);
        }
        else{
            adcConverter[0] = (32);
            adcConverter[1] = (ten + 48);
            adcConverter[2] = ('.');
            adcConverter[3] = (one + 48);
        }
    }
            else{
        adcConverter[0] = (hundred + 48);
        adcConverter[1] = (ten + 48);
        adcConverter[2] = ('.');
        adcConverter[3] = (one + 48);
    }
    return adcConverter;
}



int main(void){
  PLL_Init(Bus80MHz);               // bus clock at 80 MHz
	buttonInit();
	ST7735_InitR(INITR_REDTAB);
	PeriodMeasure_Init();            // initialize 24-bit timer0A in capture mode
  //PWM0A_Init(40000, 30000);         // initialize PWM0, 1000 Hz, 75% duty
  PWM0B_Init(4000, Duty);         // initialize PWM0, 1000 Hz, 25% duty
//  PWM0_Duty(4000);    // 10%
//  PWM0_Duty(10000);   // 25%
//  PWM0_Duty(30000);   // 75%

//  PWM0_Init(4000, 2000);         // initialize PWM0, 10000 Hz, 50% duty
//  PWM0_Init(1000, 900);          // initialize PWM0, 40000 Hz, 90% duty
//  PWM0_Init(1000, 100);          // initialize PWM0, 40000 Hz, 10% duty
//  PWM0_Init(40, 20);             // initialize PWM0, 1 MHz, 50% duty
  while(1){
		localaverage = RPSaverage;
		RevPerSec = 200000000/localaverage;
		rateOfChange = (TargetRev - RevPerSec);
		if(getState()&0x2){
			resetState();
			if(TargetRev<600){
				TargetRev+=50;
			}
		}else{
		if(getState()&0x8){
			resetState();
			if(TargetRev>100){
				TargetRev-=50;
			}
		}}
		
		if((Duty+rateOfChange>1500) && (Duty+rateOfChange<3500)){
	Duty+=rateOfChange;
					PWM0B_Duty(Duty);
		}
		for(int j = 0; j < 4; j++){
					ST7735_SetCursor(1,1);
		//ST7735_DrawString(1,1, "Target Speed:", ST7735_RED);
		char* target = adcToString(TargetRev);	
		ST7735_OutString(target);
		ST7735_SetCursor(1,2);
		char* actual = adcToString(RevPerSec);
		ST7735_OutString(actual);
		for(int i = 0; i <1000000; i++){}
		}
    //WaitForInterrupt();
  }
}
