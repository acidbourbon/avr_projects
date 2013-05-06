#define F_CPU 12e6

#define DISPLAY_ROWS 2
#define DISPLAY_COLS 16

#include <avr/pgmspace.h>

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "lcd.c"
#include "touchpad_franky.c"

void my_uitoa(uint16_t zahl, char* string, char no_digits) {
	int8_t i; // schleifenzähler

	string[no_digits] = '\0'; // String Terminator
	for (i = (no_digits - 1); i >= 0; i--) {
		if (zahl == 0 && i < (no_digits - 1)) {
			string[i] = ' ';
		} else {
			string[i] = (zahl % 10) + '0';
		} // Modulo rechnen, dann den ASCII-Code von '0' addieren
		zahl /= 10;
	}

}

const char welcome_screen_0[] PROGMEM = "U-Bahn-Display v0.1";
const char welcome_screen_1[] PROGMEM = "fuer Station FFM-Niederursel";

int main(void) {

	char stringbuffer[16];
	uint8_t zbyte = 0;
	uint8_t x, y = 0;
	int8_t dx, dy = 0;
	uint8_t field_val = 0;

	/*
	 //##############timer
	 TCCR1A = 0b00000000;	                    //Timer im CTC-Mode; Prescaler 256; CPU-Clock
	 TCCR1B = 0b00001100;	                    //Timer im CTC-Mode; Prescaler 256; CPU-Clock
	 TCCR1C = 0b00000000;	                    //Timer im CTC-Mode; Prescaler 256; CPU-Clock
	 TIMSK  = 0b01000000;	                    //Interrupt bei Compare Match
	 OCR1A  = 62500;
	 */

//###############inits!####################
	LCD_DDR = 0xFF;
	lcd_init();
	lcd_string("lcd initialized!");
	touchpad_init();

	_delay_ms(800);

	sei();
	touchpad_set_abs_mode();

	_delay_ms(2000);
	lcd_clear();

	while (1) {

		_delay_ms(20);
		// Register 0 auslesen
		adb_werte[0] = COM_TALK0;
		zbyte = adb();

		lcd_setcursor(0, 0);
		my_uitoa(x_abs(), stringbuffer, 4);
		lcd_string(stringbuffer);

		lcd_string("x");
		my_uitoa(y_abs(), stringbuffer, 4);
		lcd_string(stringbuffer);
		lcd_string("_");

		//print data length
		/*
		 lcd_setcursor(0,1);
		 my_uitoa(zbyte,stringbuffer,3);
		 lcd_string(stringbuffer);
		 */

		//print z pressure
//		lcd_setcursor(0,1);
		my_uitoa(z_pressure(), stringbuffer, 3);
		lcd_string(stringbuffer);

		field_val = decode_field();
		if (field_val) {
			lcd_setcursor(0, 1);
			my_uitoa(field_val, stringbuffer, 3);
			lcd_string(stringbuffer);
		}

		/*
		 if (zbyte) {

		 if (adb_werte[1] & 0b01000000) {
		 dx = -(128 - adb_werte[1] & 0b00111111);
		 } else {
		 dx = adb_werte[1] & 0b00111111;
		 }

		 if (adb_werte[2] & 0b01000000) {
		 dy = -(128 - adb_werte[2] & 0b00111111);
		 } else {
		 dy = adb_werte[2] & 0b00111111;
		 }

		 if (x + dx > 255) {
		 x = 255;
		 } else if (x + dx < 0) {
		 x = 0;
		 } else {
		 x = (x + dx);
		 }

		 if (y + dy > 255) {
		 y = 255;
		 } else if (y + dy < 0) {
		 y = 0;
		 } else {
		 y = (y + dy);
		 }

		 }
		 lcd_setcursor(0, 1);
		 my_uitoa(x, stringbuffer, 4);
		 lcd_string(stringbuffer);

		 lcd_setcursor(5, 1);
		 my_uitoa(y, stringbuffer, 4);
		 lcd_string(stringbuffer);

		 */

	}

// end of main
	return 0;
}

