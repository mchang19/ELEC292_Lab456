//  pwm_EFM8.c: Controls a motor using two PWM inputs. Both PWM inputs go from 0 to 100, controlling the power. PWM1
//  controls clockwise rotation and PWM2 controls counter-clockwise rotation. The speed is controlled by push buttons
//  and features a welcome/restart screen.
//
//  Authors: Anna Yun and Magan Chang
//  Source code: Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>
#include "pwm_EFM8_header.h"

// ~C51~  

#define SYSCLK 72000000L
#define BAUDRATE 115200L

#define OUT0 P2_0
#define OUT1 P1_7
#define PUSH1 P1_4
#define PUSH2 P1_5

volatile unsigned char pwm_count=0;
volatile unsigned int pwm1 = 0;
volatile unsigned int pwm2 = 0;

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN=0x80;       // enable VDD monitor
	RSTSRC=0x02|0x04;  // Enable reset on missing clock detector and VDD

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif
	
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X00;
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	SCON0 = 0x10;
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	// Initialize timer 2 for periodic interrupts
	TMR2CN0=0x00;   // Stop Timer2; Clear TF2;
	CKCON0|=0b_0001_0000; // Timer 2 uses the system clock
	TMR2RL=(0x10000L-(SYSCLK/10000L)); // Initialize reload value
	TMR2=0xffff;   // Set to reload immediately
	ET2=1;         // Enable Timer2 interrupts
	TR2=1;         // Start Timer2 (TMR2CN is bit addressable)

	EA=1; // Enable interrupts

  	
	return 0;
}

// Timer interrupt
void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count++;
	if(pwm_count>100) pwm_count=0; //Interrupts every 0.1 ms
	
	OUT0=pwm_count>pwm1?0:1; //Generates pwm1% square wave at P2.0
	OUT1=pwm_count>pwm2?0:1; //Generates pwm2% duty cycle wave at P2.1
}

void main (void)
{
	int input = 0;
	int direction = 0;
	xdata char LCD1[16];
	xdata char LCD2[16];
	
	LCD_4BIT();
	
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("Square wave generator for the EFM8LB1.\r\n"
	       "Check pins P2.0 and P2.1 with the oscilloscope.\r\n");
	

		
		

	while(1)
	{	
	
	//FOR PUTTY//	
	//	printf("Enter a number from -100 to 100: ");
	//	scanf("%i\n", &input);
	//FOR SWITCHES//
	
		LCDprint("WELCOME", 1, 1);
		LCDprint("Press start!", 2, 1);
	
	if(P1_3 != 1){
		
		
		
		if(P1_2 != 0){
			
			while(1){
	
				if(P1_5 != 1){
					if(input < 100){
					input = input + 10;
					}
				}
				
				if(P1_4 != 1){
					if(input > -100){
					input = input - 10;
					}
				}
					
				if (input == 0){ //nothing moves
					pwm1 = 0;
					pwm2 = 0;
					direction = 0;
				}
				else if (input > 0){
					pwm1 = input; //cw at pwm1 pin
					pwm2 = 0;
					direction = 1;
				}
				else{
					pwm1 = 0;
					pwm2 = -1 * input; //ccw at pwm2 pin
					direction = -1;
					
			
				}
				
				sprintf(LCD1, "PWM1(CW):  %i", pwm1);
				LCDprint(LCD1, 1, 1);
				sprintf(LCD2, "PWM2(CCW): %i", pwm2);
				LCDprint(LCD2, 2, 1);
				
				printf("\n");
				
				if(P1_2 != 1){
					pwm1 = 0;
					pwm2 = 0;
					input = 0;
					LCDprint("DONE", 1, 1);
					LCDprint("Restarting...", 2, 1);
					waitms(2000);
					break;
					}
			} //inner while loop
		} //P1_2
	
	}
	}//P1_3
}//outer while


