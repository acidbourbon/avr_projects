/*
	Diese Library bassiert auf der Arduino RFM70 Library, ich habe sie lediglich so
	angepasst, dass man sie auf einem AVR und ohne zusätzliche Libs von Arduino
	verwenden kann.

	Es sind nur die wichtigstens Funktionen implementiert!
	Bei Fragen oder Anregungen:
	Mein Blog: http:://projects.web4clans.com
	Meine Email: daniel.weber@web4clans.com

*/

#define UART_BAUD_RATE 9600
#define F_CPU 12e6


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd.h"
#include "lcd.c"



#include "uart.c"
#include "rfm70.c"



int main()
{
	uint8_t test[20]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x78,0x06,0x11,0x99};
	uint8_t buffer[10];

	sei();
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	lcd_init();

	//DDRA |= (1<<PA4);
	_delay_ms(1000);
	uart_puts("System Ready\n");
	lcd_print("System Ready\n");
	Begin();

	setMode(1);// set mode r
	setChannel(8);

	uint8_t textcounter=0;


//testblock
/*
	while(1) {
		lcd_goto(0,0);
		lcd_print("AAAABBBB");
		_delay_ms(1000);
		lcd_goto(0,0);
		lcd_print("YYYYZZZZ");
		_delay_ms(1000);
	}
*/


	while (1)
	{
	
		//sendPayload(test,20, 0);
		//_delay_ms(500);
		/*
		sendPayload("hallo   ",8, 0);
		_delay_ms(200);
		sendPayload("Vera    ",8, 0);
		_delay_ms(200);
		sendPayload("ich     ",8, 0);
		_delay_ms(200);
		sendPayload("habs    ",8, 0);
		_delay_ms(200);
		sendPayload("geschfft",8, 0);
		_delay_ms(200);
		*/


		if (receivePayload(buffer))
		{
			textcounter = (textcounter +1)%16;
			buffer[1] = '\0';
			uart_puts((char*)buffer);
			uart_puts("\r\n");
			if(textcounter == 0)
				lcd_goto(0,0);
			if(textcounter == 8)
				lcd_goto(2,0);
			lcd_print((char*)buffer);
			//lcd_print("\n");
		}


	
	}
}
