// Lab5Sine.c: Measures the magnitude of amplitude of two sinusoidal signals and
// measures the phase difference between them.
// Authors: Anna Yun and Magan Chang
// Source code by:  Jesus Calvino-Fraga (c) 2008-2018
//
// The next line clears the "C51 command line options:" field when compiling with CrossIDE
//  ~C51~  

#include <EFM8LB1.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Lab5SineHeader.h"

#define SYSCLK      72000000L  // SYSCLK frequency in Hz
#define BAUDRATE      115200L  // Baud rate of UART in bps
#define VDD 3.3035 // The measured value of VDD in volts


unsigned char overflow_count;

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN |= 0x80;
	RSTSRC = 0x02;

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

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
	
	return 0;
}

void InitADC (void)
{
	SFRPAGE = 0x00;
	ADC0CN1 = 0b_10_000_000; //14-bit,  Right justified no shifting applied, perform and Accumulate 1 conversion.
	ADC0CF0 = 0b_11111_0_00; // SYSCLK/32
	ADC0CF1 = 0b_0_0_011110; // Same as default for now
	ADC0CN0 = 0b_0_0_0_0_0_00_0; // Same as default for now
	ADC0CF2 = 0b_0_01_11111 ; // GND pin, Vref=VDD
	ADC0CN2 = 0b_0_000_0000;  // Same as default for now. ADC0 conversion initiated on write of 1 to ADBUSY.
	ADEN=1; // Enable ADC
}

void InitPinADC (unsigned char portno, unsigned char pin_num)
{
	unsigned char mask;
	
	mask=1<<pin_num;

	SFRPAGE = 0x20;
	switch (portno)
	{
		case 0:
			P0MDIN &= (~mask); // Set pin as analog input
			P0SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 1:
			P1MDIN &= (~mask); // Set pin as analog input
			P1SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 2:
			P2MDIN &= (~mask); // Set pin as analog input
			P2SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		default:
		break;
	}
	SFRPAGE = 0x00;
}

unsigned int ADC_at_Pin(unsigned char pin)
{
	ADC0MX = pin;   // Select input from pin
	ADBUSY=1;       // Dummy conversion first to select new pin
	while (ADBUSY); // Wait for dummy conversion to finish
	ADBUSY = 1;     // Convert voltage at the pin
	while (ADBUSY); // Wait for conversion to complete
	return (ADC0);
}

float Volts_at_Pin(unsigned char pin)
{
	 return ((ADC_at_Pin(pin)*VDD)/16383.0);
}
void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0=0; // Stop Timer/Counter 0
}

void main (void) 
{
	float period;
	float half_period;
	float v[4];
	float time_diff;
	float phase;
	float vrms1, vrms2;
	int sign = 1;
	float offset;

	xdata char LCD1[16];
	xdata char LCD2[16];
	
	LCD_4BIT();
	TIMER0_Init();

	waitms(500); // Give PuTTY a chance to start.
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.

	printf ("EFM8 Period measurement at pin P0.1 using Timer 0.\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);
	        
	InitPinADC(1, 0); // Configure P1.0 as analog input (analog for test)
	InitPinADC(1, 1); // Configure P1.1 as analog input (analog for reference)
	InitADC();
	
	//1.2 - test, 1.3 - reference
	
    while (1)
    {
    	// Reset the counter
		TL0=0; 
		TH0=0;
		TF0=0;
		overflow_count=0;
		
		while(P1_3!=0); // Wait for the signal to be zero
		while(P1_3!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		while(P1_3!=0) // Wait for the signal to be zero
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		while(P1_3!=1) // Wait for the signal to be one
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
				
			}
			
		}
		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		period=(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
		half_period = period/2.0;
		
		// Send the period to the serial port
		printf( "\rT=%f ms    ", period*1000.0);
		
		
		//wait till period/4//
		overflow_count = 0; 
		TL0=0; 
		TH0=0;
		TF0=0;
		while(P1_3!=0); // Wait for the signal to be zero
		while(P1_3!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		while((overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK) < period/4.0) // Wait for the signal to be zero
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		while(P1_3!=1) // Wait for the signal to be one
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
				
			}
			
		}
		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		v[1] = Volts_at_Pin(QFP32_MUX_P1_1);
		printf("Reference peak voltage %.2F ", v[0]);
		//do the whole fucking thing again for the other signal
		
		
		//wait till period/4//
		overflow_count = 0; 
		TL0=0; 
		TH0=0;
		TF0=0;
		while(P1_2!=0); // Wait for the signal to be zero
		while(P1_2!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		while((overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK) < period/4.0) // Wait for the signal to be zero
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		while(P1_2!=1) // Wait for the signal to be one
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
				
			}
			
		}
		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		v[0] = Volts_at_Pin(QFP32_MUX_P1_0);
		printf("Test peak voltage %.2F  ", v[1]);
		
		
		
		//find time difference between zero crosses//
		overflow_count = 0; 
		TL0=0; 
		TH0=0;
		TF0=0;
		
		while(P1_3!=0); // Wait for the signal to be zero
		while(P1_3!=1){
			if (P1_2 == 1)
				sign = 1;
			else
				sign = -1;
		}
		
		while(P1_3!=0); // Wait for the signal to be zero
		while(P1_3!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		while(P1_2!=0) // Wait for the signal to be zero
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		offset = (overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
		while(P1_2!=1) // Wait for the signal to be one
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
				
			}	
		}
		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		time_diff = (overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
		printf("Time diff %.5F ms", time_diff*1000.0);
		
		if (sign == -1)
			phase = (time_diff-offset)*(360.0/period);
		else
			phase = sign*time_diff*(360.0/period) - 360;
		
		vrms1 = v[0] / sqrtf(2.0);
		vrms2 = v[1] / sqrtf(2.0);
		
		sprintf(LCD1, "V1:%.2fV V2:%.2fV", vrms1, vrms2);
		LCDprint(LCD1, 1, 1);
		
		sprintf(LCD2, "Phase: %.2fdeg", phase);
		LCDprint(LCD2, 2, 1);
		
		waitms(500);
    }
	
}


 
