// main.c
// Runs on LM4F120/TM4C123
// UART runs at 115,200 baud rate 
// Daniel Valvano
// May 3, 2015

/* This example accompanies the books
  "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
  ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

"Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
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


#include <stdint.h> // C99 variable types
#include "ADCT0ATrigger.h"
#include "uart.h"
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

extern uint32_t ADCvalue[100];
extern uint8_t indexholder;
extern uint32_t ADCAverage;
//int32_t ADCTemp[65] = {4100, 4020, 3850, 3780, 3700, 3630, 3560, 3480, 3420, 3350, 3270, 3200, 3130, 3060, 2990, 2930, 2860, 2790, 2720, 2660, 2590, 2520, 2460, 2390, 2330, 2270, 2200, 2140, 2080, 2010, 1950, 1890, 1830, 1770, 1700, 1640, 1580, 1520, 1460, 1400, 1340, 1280, 1230, 1170, 1110, 1050, 990, 940, 880, 820, 770, 710, 660, 600, 540, 490, 430, 370, 320, 270, 220, 160, 110, 50, 0};
int32_t ADCTemp[62] = {4000,
3930,		//1
3860,		//2
3790,		//3
3720,		//4
3650,		//5
3580,		//6
3520,		//7
3450,		//8
3390,		//9
3330,		//10
3270,		//11
3210,		//12
3150, 	//13
3090,		//14
2030,		//15
2970,		//16
2920,		//17
2860,		//18
2810,		//19
2750,		//20
2700,		//21
2650,		//22
2600,		//23
2550,		//24
2500,		//25
2450,		//26
2400,		//27
2350,		//28
2300,		//29
2260,		//30
2210,		//31
2160,		//32
2120,		//33
2070,		//34
2030,		//35
1987, 	//36
1940,		//37
1899,		//38
1860,		//39
1815,		//40
1770,		//41
1730,		//42
1690,		//43
1650,		//44
1610,		//45
1570,		//46
1535,		//47
1497,		//48
1455,		//49
1420,		//50
1380,		//51
1345,		//52
1310,		//53
1270,		//54
1235,		//55
1199,		//56
1164,		//57
1130,		//58
1093,		//59
1060,		//60
1000,		//61
};
	char adcConverter[5];

char* adcToString(int m){//returns a string of the 3 digit integer
    adcConverter[3] = 0;

    int one = 0;
    int ten = 0;
    int hundred =  0;
    one = m % 100;
		one = one/10;
    m = m/100;
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


	
uint32_t interpret(uint32_t input){
	uint32_t placeholder = input/64;
	uint32_t leftover;
	uint32_t temperature;
	uint32_t sloperesult;
	if(placeholder>= 61){
		return 1000;
	}
	else{
	leftover = input%64;
	temperature = ADCTemp[placeholder];
	sloperesult = leftover*(temperature - ADCTemp[placeholder+1])/64;
	temperature = temperature - sloperesult;
	temperature += 5;
	return temperature;
	}
}

int main(void){ 
	//int32_t data;
  PLL_Init(Bus80MHz);   // 80 MHz
	ST7735_InitR(INITR_REDTAB);
  //UART_Init();              // initialize UART device
  SYSCTL_RCGCGPIO_R |= 0x00000020;         // activate port F
  ADC0_InitTimer0ATriggerSeq3(0, 799999); // ADC channel 0, 100 Hz sampling
  GPIO_PORTF_DIR_R |= 0x04;                // make PF2 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x04;             // disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x04;                // enable digital I/O on PF2
                                           // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;                  // disable analog functionality on PF
  GPIO_PORTF_DATA_R &= ~0x04;              // turn off LED
  EnableInterrupts();
  while(indexholder != 9){}
		ST7735_FillScreen(ST7735_BLACK);
		ST7735_SetCursor(1,1);
		uint32_t overall = 0;
		uint32_t tempout = 0;
		uint32_t comparetemp = 0;
		
		while(1){
			overall = ADCAverage/10;		//takes the average of last 10 adc values
			comparetemp = interpret(overall);		//uses the average to look up index in ADCTemp array to find temperature
			for(int i = 0; i<16000000; i++){}
			if(tempout != comparetemp){ //if the temperature is the same as before then leave the lcd alone
				tempout = comparetemp;
				char *stringADC = adcToString(tempout);		//converts int to char array to print to lcd
				ST7735_OutString(stringADC);
				ST7735_SetCursor(1,1);			
			}
  }
}


