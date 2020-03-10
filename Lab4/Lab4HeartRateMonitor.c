// Lab4HeartRateMonitor.c: Uses a photosensitive finger clip to measure the heartrate and display it on an LCD.
// Authors: Anna Yun and Magan Chang
// Source code: Jesus Calvino-Fraga

#include <EFM8LB1.h>
#include <stdio.h>
#include "Lab4HRMHeader.h"

/* From PeriodEFM8.c*/
#define SYSCLK      72000000L  // SYSCLK frequency in Hz
#define BAUDRATE      115200L  // Baud rate of UART in bps

unsigned char overflow_count;

char _c51_external_startup(void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key

	VDM0CN |= 0x80;
	RSTSRC = 0x02;

#if (SYSCLK == 48000000L)	
	SFRPAGE = 0x10;
	PFE0CN = 0x10; // SYSCLK < 50 MHz.
	SFRPAGE = 0x00;
#elif (SYSCLK == 72000000L)
	SFRPAGE = 0x10;
	PFE0CN = 0x20; // SYSCLK < 75 MHz.
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
	P1MDOUT = 0b_0010_0000;//Configure P1.5 used for LED Blinky

	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0 = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1 = 0X00;
	XBR2 = 0x40; // Enable crossbar and weak pull-ups

#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
#endif
// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100 - ((SYSCLK / BAUDRATE) / (2L * 12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |= 0x20;
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	return 0;
}
/*End from PeriodEFM8.c*/

/*From PeriodEFM8.c*/
void TIMER0_Init(void)
{
	TMOD &= 0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD |= 0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0 = 0; // Stop Timer/Counter 0
}

void main(void)
{
	float period;
	float heartrate;
	char heartrate_string[16];

	LCD_4BIT();
	TIMER0_Init();

	waitms(500); // Give PuTTY a chance to start.
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.

	printf("EFM8 Period measurement at pin P0.1 using Timer 0.\n"
		"File: %s\n"
		"Compiled: %s, %s\n\n",
		__FILE__, __DATE__, __TIME__);

	while (1)
	{
		// Reset the counter
		TL0 = 0;
		TH0 = 0;
		TF0 = 0;
		overflow_count = 0;

		TR0 = 1; // Start the timer
		LCDprint("Heart Rate:      ", 1, 1); // Printing string on LCD
		while (P0_1 != 0) // Wait for the signal to be zero
		{
			if (TF0 == 1) // Did the 16-bit timer overflow?
			{
				TF0 = 0;
				overflow_count++;
			}
		}

		LCDprint("Heart Rate:     <3", 1, 1); // Printing string on LCD

		while (P0_1 != 1) // Wait for the signal to be one
		{
			if (TF0 == 1) // Did the 16-bit timer overflow?
			{
				TF0 = 0;
				overflow_count++;
			}
		}
		TR0 = 0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		period = (overflow_count * 65536.0 + TH0 * 256.0 + TL0) * (14.0 / SYSCLK); // Period is in microseconds
		heartrate = 60 / period; // Heartrate

		if (heartrate < 200 || heartrate > 20){
			sprintf(heartrate_string, "%.1f BPM", heartrate *0.9); //Convert float to string
			LCDprint(heartrate_string, 2, 1);
		}
		
		if (heartrate > 90){
				/* From Blinky.c, LED connected to pin 1.5*/
			P1_5 = !P1_5;
			delay(50000);
			/* End Blinky.c*/
		}
		
		if (heartrate < 40){
			LCDprint("Ded         ", 1, 1);
			LCDprint("            ", 2, 1);
		}

		// Send the period and heartrate to the serial port
		printf("\rT=%f ms, Heart Rate = %f bpm ", period * 1000.0, heartrate);


	}

}
/*End PeriodEFM8.c*/
