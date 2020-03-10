/*From EFM8_LCD_4Bit.h*/
#include <EFM8LB1.h>
#include <stdio.h>

#define SYSCLK    72000000L // SYSCLK frequency in Hz
#define BAUDRATE    115200L // Baud rate of UART in bps
/*End EFM8_LCD_4Bit.h*/

/*From lcd.h*/
#define LCD_RS P2_6
//#define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E  P2_5
#define LCD_D4 P2_4
#define LCD_D5 P2_3
#define LCD_D6 P2_2
#define LCD_D7 P2_1
#define CHARS_PER_LINE 16

void Timer3us(unsigned char us);
void waitms(unsigned int ms);
void LCD_pulse(void);
void LCD_byte(unsigned char x);
void WriteData(unsigned char x);
void WriteCommand(unsigned char x);
void LCD_4BIT(void);
void LCDprint(char* string, unsigned char line, bit clear);
int getsn(char* buff, int len);
void delay(unsigned int x);

/*End from lcd.h*/

/*From PeriodEFM8.c*/
// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter

	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON0:
	CKCON0 |= 0b_0100_0000;

	TMR3RL = (-(SYSCLK) / 1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow

	TMR3CN0 = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN0 & 0x80));  // Wait for overflow
		TMR3CN0 &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN0 = 0;                   // Stop Timer3 and clear overflow flag
}

void waitms(unsigned int ms)
{
	unsigned int j;
	for (j = ms; j != 0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}
/*End PeriodEFM8.c*/

/*From EFM8_LCD_4bit.c*/
void LCD_pulse(void)
{
	LCD_E = 1;
	Timer3us(40);
	LCD_E = 0;
}

void LCD_byte(unsigned char x)
{
	// The accumulator in the C8051Fxxx is bit addressable!
	ACC = x; //Send high nible
	LCD_D7 = ACC_7;
	LCD_D6 = ACC_6;
	LCD_D5 = ACC_5;
	LCD_D4 = ACC_4;
	LCD_pulse();
	Timer3us(40);
	ACC = x; //Send low nible
	LCD_D7 = ACC_3;
	LCD_D6 = ACC_2;
	LCD_D5 = ACC_1;
	LCD_D4 = ACC_0;
	LCD_pulse();
}

void WriteData(unsigned char x)
{
	LCD_RS = 1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand(unsigned char x)
{
	LCD_RS = 0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT(void)
{
	LCD_E = 0; // Resting state of LCD's enable is zero
	// LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char* string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line == 2 ? 0xc0 : 0x80);
	waitms(5);
	for (j = 0; string[j] != 0; j++)	WriteData(string[j]);// Write the message
	if (clear) for (; j < CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

int getsn(char* buff, int len)
{
	int j;
	char c;

	for (j = 0; j < (len - 1); j++)
	{
		c = getchar();
		if ((c == '\n') || (c == '\r'))
		{
			buff[j] = 0;
			return j;
		}
		else
		{
			buff[j] = c;
		}
	}
	buff[j] = 0;
	return len;
}
/*End EFM8_LCD_4bit.c*/

/*From Blinky.c*/
void delay(unsigned int x)
{
	unsigned char j;
	while (--x)
	{
		for (j = 0; j < 100; j++);
	}
}
/*End Blinky.c*/
