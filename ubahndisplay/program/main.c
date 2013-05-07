// source code for an automatic subway schedule viewer



#define DCF_PIN PINA
#define DCF_PORT PORTA
#define DCF_DDR DDRA
#define DCF_DATA 1
#define DCF_PON 0


#define F_CPU 16e6


#define DISPLAY_ROWS 2
#define DISPLAY_COLS 40

#define DISPLAY_CLOCK_COUNTDOWN 10;




#include <avr/pgmspace.h>
#include "lcd.c"
#include <util/delay.h>
#include <avr/interrupt.h>

#include "train_data.h" // contains the train schedules

const char mon[] PROGMEM = "Mon";
const char die[] PROGMEM = "Die";
const char mit[] PROGMEM = "Mit";
const char don[] PROGMEM = "Don";
const char fre[] PROGMEM = "Fre";
const char sam[] PROGMEM = "Sam";
const char son[] PROGMEM = "Son";
 
const char *     const daysoftheweek[] PROGMEM = 
{
mon,
die,
mit,
don,
fre,
sam,
son,
};


const char welcome_screen_0[] PROGMEM = "U-Bahn-Display v0.3 (2013)";
const char welcome_screen_1[] PROGMEM = "fuer Station FFM-Niederursel";


//global variables
volatile int32_t seconds=0;
volatile int16_t min_pz = 0;
volatile uint8_t state = 0;
uint8_t h_;
uint8_t h; 
uint8_t m_;
uint8_t m;
uint8_t s_;
uint8_t s;
uint8_t d=0;			// tag
volatile uint8_t global_test_var;
uint8_t display_clock_counter = DISPLAY_CLOCK_COUNTDOWN;
uint8_t mode;
uint8_t highlight;
volatile uint8_t dcfbuffer[8];
volatile uint8_t decodable = 0;
volatile uint8_t new_bit, new_bit_flag = 0;





ISR(TIMER1_COMPA_vect)			       //Bei Compare-Match mit OCR1A (31250) wird seconds um eins erhöht.
{
	seconds++;
	if (seconds >= 86400) {		       //Wenn 24h vorbei sind, wird wieder auf 0 gesetzt
		seconds = seconds % 86400;
		if (d==7) {
			d=0;
		} else {
			d++;
		}
	}
	min_pz = seconds/60;				// berenchne Minuten nach Mitternacht


	if (mode == 2) { // update setting clock every second in case countdown still running
		
		if(display_clock_counter > 0) {
			display_clock_counter --;
			if (display_clock_counter ==0) { // when countdown's over, return to schedule business
				state =1;
			} else {
				state = mode;
			}
		}
	} else {
		if ( seconds%60 ==0) {
			state = 1; // update schedule every minute
		}
	}
	
}



ISR(TIMER2_OVF_vect)			       //Das DCF77 signal abtasten
{
	static uint8_t locounter = 0;	
	static uint8_t hicounter = 0;
	static uint8_t state = 0;
	static uint8_t counter = 0;

	

	if(DCF_PIN & 1<<DCF_DATA) {
		if(state == 1) {
			//lcd_string("L");
			state = 0;
		}
		//what to do when dcf input is lo
		locounter++;
	} else {
			//what to do when dcf input is hi
		if(state == 0) {
			//lcd_string("H");
			state = 1;
			new_bit_flag=1;
			//decode
			if(counter%8 == 0) {
				dcfbuffer[counter/8] = 0;
			}
			if(locounter >=4 && locounter <=8){
				new_bit=0;
			} else if (locounter >=11 && locounter <=14) {
				new_bit=1;
				dcfbuffer[counter/8] |= 1<<(counter%8);
			}
			counter++;
			locounter = 0;
			hicounter = 0;
		}
		hicounter++;
		if (hicounter > 100) { // ende von der Minute erreicht
			if(counter == 59) {
				decodable = 1;
			}
			new_bit=2;
			new_bit_flag=1;
			hicounter = 0;
			counter = 0;
		}
	}




}


void my_uitoa(uint32_t zahl, char* string, uint8_t no_digits) {

  int8_t i;                             // schleifenzähler
  string[no_digits]='\0';                       // String Terminator
  for(i=(no_digits-1); i>=0; i--) {
    	if(zahl == 0 && i<(no_digits-1)) {
	string[i] = ' ';
	}
	else {
	string[i]=(zahl % 10) +'0';
	}        // Modulo rechnen, dann den ASCII-Code von '0' addieren
    zahl /=10;
  }
	if(zahl >0) {
		for(i=(no_digits-1); i>=0; i--) {
		string[i] = '+'; // falls Zahl nicht mit der gewünschten Zeichenzahl dargestellt
		}		// werden kann, dann fülle die Ausgabe mit Plusen
	}

}







void calc_time (void){
h_ = (seconds / 3600) / 10;
h = (seconds / 3600) - h_*10;
m_    = ((seconds % 3600) / 60) / 10;
m     = ((seconds % 3600) / 60) - m_*10;
s_	= (seconds % 60) /10;
s	= (seconds % 60) - s_*10;
}






void make_highlight(uint8_t x, uint8_t input) {; // only called in "show_clock_at", makes one item pop out
	if(highlight==input) {
		lcd_setcursor(x,0);
	} else {
		lcd_setcursor(x,1);
	}
}






void show_clock_at (uint8_t x) { // displays time and weekday at specified x position

static char string[3];

string[2] = '\0';

make_highlight(x,1);
lcd_string_P(pgm_read_word(&daysoftheweek[d]));
x+=4;

make_highlight(x,2);
string[0] = h_ + '0';
string[1] = h + '0';
lcd_string(string);
x+=2;

lcd_setcursor(x,1);
string[0] = ':';
string[1] = '\0';
lcd_string(string);
x+=1;

make_highlight(x,3);
string[0] = m_ + '0';
string[1] = m + '0';
lcd_string(string);
x+=2;

lcd_setcursor(x,1);
string[0] = ':';
string[1] = '\0';
lcd_string(string);
x+=1;

make_highlight(x,4);
string[0] = s_ + '0';
string[1] = s + '0';
lcd_string(string);
}






void lcd_number(uint16_t number,uint8_t no_digits) {
	static 	char stringbuffer[16];
	my_uitoa(number,stringbuffer,no_digits);
	lcd_string(stringbuffer);
	}






uint8_t today() { //returns todays bit pattern
	return 1<<d;
}






uint8_t tomorrow() { //returns bit pattern for tomorrow
	if (d == 6) {
		return 1;
	} else {
		return 1<<(d+1);
	}
}






uint8_t next_day(uint8_t day) {
	if (day == 0b01000000) {
		return 1;
	} else {
		return (day<<1);
	}
}


uint8_t parity(uint8_t input) {

	uint8_t parity_accumulator = 0;

	for (uint8_t i=0;i<8;i++) {	
		parity_accumulator ^= (input>>i)&1;
	}
	return parity_accumulator;
}

void decode_dcf() {

uint32_t hour = 0,min = 0;
uint8_t dow = 0;
uint8_t check_sum =0;




//parity check DOW
check_sum += parity(dcfbuffer[4]&0b11110000) ^ parity(dcfbuffer[5])^ parity(dcfbuffer[6])^ parity(dcfbuffer[7]&0b00000111);
//parity check HOURS
check_sum += parity(dcfbuffer[3]&0b11100000) ^ parity(dcfbuffer[4]&0b00001111);
//parity check MIN
check_sum += parity(dcfbuffer[2]&0b11100000) ^ parity(dcfbuffer[3]&0b00011111);

if (check_sum) {
	return;
}



//decode day of week
				if( dcfbuffer[5]&(1<<2) )  {
					dow += 1;
				}
				if( dcfbuffer[5]&(1<<3) )  {
					dow += 2;
				}
				if( dcfbuffer[5]&(1<<4) )  {
					dow += 4;
				}


//decode hours
				if( dcfbuffer[3]&(1<<5) )  {
					hour += 1;
				}
				if( dcfbuffer[3]&(1<<6) )  {
					hour += 2;
				}
				if( dcfbuffer[3]&(1<<7) )  {
					hour += 4;
				}
				if( dcfbuffer[4]&(1<<0) )  {
					hour += 8;
				}
				if( dcfbuffer[4]&(1<<1) )  {
					hour += 10;
				}
				if( dcfbuffer[4]&(1<<2) )  {
					hour += 20;
				}

//decode minutes
				if( dcfbuffer[2]&(1<<5) )  {
					min += 1;
				}
				if( dcfbuffer[2]&(1<<6) )  {
					min += 2;
				}
				if( dcfbuffer[2]&(1<<7) )  {
					min += 4;
				}
				if( dcfbuffer[3]&(1<<0) )  {
					min += 8;
				}
				if( dcfbuffer[3]&(1<<1) )  {
					min += 10;
				}
				if( dcfbuffer[3]&(1<<2) )  {
					min += 20;
				}
				if( dcfbuffer[3]&(1<<3) )  {
					min += 40;
				}





				cli();
				d=dow-1;
				seconds=hour*3600+min*60;
				sei();



/*
				lcd_setcursor(5,1);

				lcd_number(dow,1);
				lcd_string(" ");
				//lcd_number((dcfbuffer[7]>>2)&1,1);
				lcd_number( parity(dcfbuffer[4]&0b11110000) ^ parity(dcfbuffer[5])^ parity(dcfbuffer[6])^ parity(dcfbuffer[7]&0b00000111) , 1);
				lcd_string(" | ");
				lcd_number(hour,3);
				lcd_string(" ");
				//lcd_number((dcfbuffer[4]>>3)&1,1);
				lcd_number( parity(dcfbuffer[3]&0b11100000) ^ parity(dcfbuffer[4]&0b00001111) , 1);
				lcd_string(" | ");
				lcd_number(min,3);
				lcd_string(" ");
				//lcd_number((dcfbuffer[3]>>3)&1,1);
				lcd_number( parity(dcfbuffer[2]&0b11100000) ^ parity(dcfbuffer[3]&0b00011111) , 1);
				lcd_string(" | ");

*/



}




//########### debouncing routines


volatile uint8_t key_signal=0;
// debounce counter length = debounce time / (F_CPU /(prescaler * 256))
#define DB_CL 3

#define KEYPORT PORTA
#define KEYPIN	PINA
#define KEYDDR DDRA
#define ACTIVE_KEYS 0b11100000


#define RIGHT_KEY 1<<7
#define CENTER_KEY 1<<6
#define LEFT_KEY 1<<5


uint8_t key_pressed(uint8_t key_mask)
{
	/* returns 1 if key pattern is pressed */

	if(key_signal & key_mask)
	{
		key_signal &= ~(key_mask);
		return 1;
	}
	return 0;
}






uint8_t read_keys(uint8_t key_mask)
{
	/* returns byte with keys pressed
	if not omitted by mask */

	if(key_signal & key_mask)
	{
		uint8_t temp = key_signal;
		key_signal &= ~(key_mask);
		return temp;
	}
	return 0;
}






void update_key_state()
{
	/* surveillance of the input port,
	last states are buffered, searching for
	pattern match*/
	static uint8_t key_state[DB_CL];
	
	for(uint8_t i=(DB_CL-1); i>0;i--)
	{
		key_state[i] = key_state[i-1];
	}
	key_state[0] = ~(KEYPIN);

	key_signal = (key_state[DB_CL-1] ^ key_state[DB_CL-2]) & key_state[0]; 		 
}






// ############# \ debouncing routines






void key_action(void) {
	static uint8_t keys_pressed;
	update_key_state();
	static int8_t increment;

	keys_pressed = read_keys(ACTIVE_KEYS);
	
	if(keys_pressed) { // keys were pressed do something!
		display_clock_counter = DISPLAY_CLOCK_COUNTDOWN;
		state = 2; // if not already switch to clock showing
		
		if (mode == 2) {

			if(keys_pressed == CENTER_KEY ){ // roll selection
				highlight = (highlight + 1) % 5;
			} else {

				if(keys_pressed == LEFT_KEY ){ // decrease value
					increment = -1;
				}
				if(keys_pressed == RIGHT_KEY ){ // increase value
					increment = 1;
				}
						cli();
				switch(highlight) {
					case 1: // day
						d += increment;
						if (d < 0) {
							d+=7;
						} else {
							d = d % 7;
						}
					break;
					case 2: // hour
						seconds += increment * 3600;
						min_pz += increment * 60;
					break;
					case 3: // minutes
						seconds += increment * 60;
						min_pz += increment;
					break;
					case 4: // seconds

						TCNT1H =0;
						TCNT1L =0;
						seconds = seconds / 60 *60;

					break;
				}
				if ( seconds < 0 ) {
					seconds += 86400;
					d--;
					if (d < 0) {
						d+=7;
					}
				}
						sei();
			}
		}
	}
}





uint16_t find_next_train(struct departure * schedule, uint8_t find_again) {




	static uint16_t i;
	uint16_t result = 0;
	static uint8_t day;
	static uint16_t time;
	static uint8_t shift;


	if (find_again ==0) {
		time = min_pz;
		day = today();
		i = 1;
		shift = 0;
	}

	//lcd_setcursor(20,global_test_var);
	//lcd_number(i,5);

	while(result == 0) {
		// search forward, till you find the next train that has not departed yet, dont get out of borders!!!
		while(    (pgm_read_word(&schedule[i].depart_time) + (shift*1440) <= time || (pgm_read_byte(&schedule[i].weekly_schedule) & day )==0)  
				&& i <= pgm_read_word(&schedule[0].depart_time)) {
			i++;
		}
		// is condition met?
		if (i <= pgm_read_word(&schedule[0].depart_time)) { // schedule[0].depart_time equals number of entries in schedule
			result = i + shift * pgm_read_word(&schedule[0].depart_time);
			time = pgm_read_word(&schedule[i].depart_time); // remember time of last match
			i++; //don't look the same place twice, go on!

		} else { // result not yet available
			day = next_day(day);
			shift += 1;
			i-= pgm_read_word(&schedule[0].depart_time);
		}
	}
	

	//lcd_setcursor(25,global_test_var);
	//lcd_number(result,5);
	return result;
}



void display_train_at(uint8_t x, uint8_t y, struct departure * schedule, uint16_t index) {
	static uint8_t line,endofline;

	lcd_setcursor(x,y);
	if ( index <= pgm_read_word(&schedule[0].depart_time) )  { // train will arrive today
		lcd_number(pgm_read_word(&schedule[index].depart_time)-min_pz,2);
	} else { //train will arrive tomorrow, e.g. after 24:00
		index-=pgm_read_word(&schedule[0].depart_time); // remove the offset
		lcd_number(pgm_read_word(&schedule[index].depart_time)+1440-min_pz,2);
	}

	line = pgm_read_byte(&schedule[index].line_and_endofline);
	endofline = line & 0x0F; // keep only lower nibble
	line = line>>4; //decode line information, keep only upper nibble, and shift it to proper position

	lcd_setcursor(x+3,y);
	lcd_string_P((char*) pgm_read_word(&line_names[line]));
	lcd_string_P((char*) pgm_read_word(&endofline_names[endofline]) );


}





//###########################################  MAIN  ############################################

int main(void) {




	//##############timer

	TCCR1A = 0b00000000;	                    //Timer im CTC-state; Prescaler 256; CPU-Clock
	TCCR1B = 0b00001100;	                    //Timer im CTC-state; Prescaler 256; CPU-Clock
	TIMSK  = 0b01010000;	                    //Interrupt bei Compare Match für Timer 1, Overflow für Timer 2
	OCR1A  = 62500;  	

	TCCR2 = (0<<COM21) | (0<<COM20) | (0<<WGM21) | (0<<WGM20) | (1<<CS22) | (1<<CS21) | (1<<CS20);





	//###############inits!####################
	LCD_DDR= 0xFF;
	lcd_init();

	// for keys

	KEYDDR &= ~(ACTIVE_KEYS);
	KEYPORT = ACTIVE_KEYS;

	// for DCF77 Receiver

	// DCF DATA is nothing but INPUT
	DCF_DDR &= ~(1<<DCF_DATA);
	DCF_PORT &= ~(1<<DCF_DATA);
	
	// DCF_PON input at first, let pullup resistor do its job
	DCF_DDR &= ~(1<<DCF_PON);
	DCF_PORT &= ~(1<<DCF_PON);

	_delay_ms(2300);
	DCF_DDR |= 1<<DCF_PON; // then pull down!



	sei();


	state = 0; // set initial state
	




	
	// test block for dcf77 receiver
	/*
	while(1) {
		//lcd_setcursor(0,0);
		_delay_ms(1000);
		lcd_number(testitest,5);
		testitest = 0;
				if(DCF_PIN & 1<<DCF_DATA) {
					lcd_string("H");
				} else {
					lcd_string("L");
				}
		//lcd_number(testitest,3);
	}
	*/


	/* //elaborate testblock for dcf77 receiver
	uint8_t dispcounter = 0;
	while(1) {
		_delay_ms(100);
		if(new_bit_flag) {
			new_bit_flag = 0;
			lcd_setcursor(dispcounter,0);
			dispcounter++;
			dispcounter = dispcounter %10;
			lcd_number(new_bit,1);
			if(dispcounter == 0) {
				lcd_setcursor(0,0);
				lcd_string("----------");
			}
			if(new_bit == 2){
				//lcd_clear();
				//lcd_setcursor(0,0);
				decode_dcf();
				
				
			}
		}
				lcd_clear();
				calc_time();
				show_clock_at((DISPLAY_COLS)/2-6);
				
	}
	*/
	

	while(1)
	{	
	
		if (state != 255) {
			mode = state; //indicator of last action
		}	
		switch (state) {
			case 0 : // show welcome screen
				lcd_clear();
				lcd_setcursor(10,0);
				lcd_string_P(welcome_screen_0);
				lcd_setcursor(3,1);
				lcd_string_P(welcome_screen_1);	
				_delay_ms(1000);
				lcd_clear();

			case 1 : // update and show schedule
				state = 255; // update clock afterwards
				lcd_clear();
				
				//global_test_var = 0;


				display_train_at(DISPLAY_COLS/2,0,south_schedule,  find_next_train(south_schedule,0)  );
				display_train_at(DISPLAY_COLS/2,1,south_schedule,  find_next_train(south_schedule,1)  );
				display_train_at(0,0,north_schedule,  find_next_train(north_schedule,0)  );
				display_train_at(0,1,north_schedule,  find_next_train(north_schedule,1)  );


				//global_test_var = 1;
			break;
			case 2 : // update and show clock
				state = 255;
				lcd_clear();
				calc_time();
				show_clock_at((DISPLAY_COLS)/2-6);
				
				
			break;

			default : // idle and key command recognition
				key_action();
				_delay_ms(20);

				if (decodable) {
					decodable = 0;
					decode_dcf();
				}

			break;
		
		}

	

	}


	// end of main
	return 0;
}

















