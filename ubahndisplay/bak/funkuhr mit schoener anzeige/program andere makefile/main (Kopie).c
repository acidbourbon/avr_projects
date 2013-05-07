#define F_CPU 16e6


#define DISPLAY_ROWS 2
#define DISPLAY_COLS 40

#include <avr/pgmspace.h>
#include "lcd.c"
#include <util/delay.h>
#include <avr/interrupt.h>

#include "train_data.h"

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


const char welcome_screen_0[] PROGMEM = "U-Bahn-Display v0.1";
const char welcome_screen_1[] PROGMEM = "fuer Station FFM-Niederursel";


//Globale Variablen
volatile uint32_t Sekunden=85800;	        //32Bit Variable; 24Bit würdens auch tun
volatile uint16_t min_pz = 0;
volatile uint8_t state = 0;
unsigned char h_;
unsigned char h; 
unsigned char m_;
unsigned char m;
unsigned char s_;
unsigned char s;
volatile unsigned char d=0;			// tag
volatile uint8_t global_test_var;



ISR(TIMER1_COMPA_vect)			       //Bei Compare-Match mit OCR1A (31250) wird Sekunden um eins erhöht.
{

	Sekunden++;
	if (Sekunden >= 86400) {		       //Wenn 24h vorbei sind, wird wieder auf 0 gesetzt
		Sekunden = Sekunden % 86400;
		if (d==7) {
			d=0;
		} else {
			d++;
		}
	}
	min_pz = Sekunden/60;				// berenchne Minuten nach Mitternacht

	if ( Sekunden%60 ==0) {
		state = 1; // update schedule every minute
	} else {
		state = 2; // update clock every second
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

 
 
void calc_time (void){			       //Sekunden zu BCD
h_ = (Sekunden / 3600) / 10;
h = (Sekunden / 3600) - h_*10;
m_    = ((Sekunden % 3600) / 60) / 10;
m     = ((Sekunden % 3600) / 60) - m_*10;
s_	= (Sekunden % 60) /10;
s	= (Sekunden % 60) - s_*10;
}



void show_clock_at (uint8_t x, uint8_t y) {

static char string[9];

string[8] = '\0';
string[0] = h_ + '0';
string[1] = h + '0';
string[2] = ':';
string[3] = m_ + '0';
string[4] = m + '0';
string[5] = ':';
string[6] = s_ + '0';
string[7] = s + '0';
lcd_setcursor(x,y);
lcd_string(string);

}

void show_date_at (uint8_t x, uint8_t y) {

lcd_setcursor(x,y);
lcd_string_P(pgm_read_word(&daysoftheweek[d]));

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




//########### Entprellzeugs


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
	if(key_signal & key_mask)
	/* returns 1 if key pattern is pressed */
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

// ############# \ Entprellzeugs

void Tastenabfrage(void) {
	update_key_state();
	if(key_pressed(LEFT_KEY)){
		Sekunden +=3600;
		min_pz +=60;
		state = 1;
	}
	if(key_pressed(CENTER_KEY)){
		Sekunden +=60;
		min_pz +=1;
		state = 1;
	}

	if(key_pressed(RIGHT_KEY)){
		cli();
		TCNT1H =0;
		TCNT1L =0;
		Sekunden = Sekunden / 60 *60;
		sei();
		state = 1;
	}
}



uint16_t find_next_train(struct departure * schedule, uint16_t start_pos, uint8_t ignore) {



	cli();
	uint16_t i = 1;
	uint16_t result;

	// remove possible offset



	if (start_pos > pgm_read_word(&schedule[0].depart_time)  && ) {
		i = 1;
	} else {
		i = start_pos;
	}


	lcd_setcursor(20,global_test_var);
	lcd_number(i,5);

	

	for(uint8_t j = 0; j<= ignore; j++) { // ignore #ignore matches
		

		



		// search forward, till you find the next train that has not departed yet, dont get out of borders!!!
		while(    (pgm_read_word(&schedule[i].depart_time) <= min_pz || (pgm_read_byte(&schedule[i].weekly_schedule) & today() )==0)  
				&& i <= pgm_read_word(&schedule[0].depart_time)) {
			i++;
		}


		if (i <= pgm_read_word(&schedule[0].depart_time)) { // schedule[0].depart_time equals number of entries in schedule
			result = i;
		} else { // found nothing till 24:00, try next day
			i = i- pgm_read_word(&schedule[0].depart_time);

			while((      pgm_read_byte(&schedule[i].weekly_schedule)   &   tomorrow()    )==0  ) {
				i++;
			}
			result = i+pgm_read_word(&schedule[0].depart_time);
			i = result;
		}


	
		i++; // move on, so next round will ignore the last result
	}
	
	sei();
				lcd_setcursor(25,global_test_var);
		lcd_number(result,5);
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
	TIMSK  = 0b00010000;	                    //Interrupt bei Compare Match
	OCR1A  = 62500;  	




	//###############inits!####################
	LCD_DDR= 0xFF;
	lcd_init();

	// for keys

	KEYDDR &= ~(ACTIVE_KEYS);
	KEYPORT = ACTIVE_KEYS;


	sei();


	state = 0; // set initial state

	uint16_t current_north_train = 1; //serves as memory for faster query operation
	uint16_t current_south_train = 1; //serves as memory for faster query operation

	global_test_var = 0;

	


	while(1)
	{	
	
		
		switch (state) {
			case 0 : // show welcome screen
				lcd_clear();
				lcd_setcursor(18,0);
				lcd_string_P(welcome_screen_0);
				lcd_setcursor(3,1);
				lcd_string_P(welcome_screen_1);	
				_delay_ms(1000);
				lcd_clear();
			case 1 : // update schedule
				state = 2; // update clock afterwards
				lcd_clear();
				
				global_test_var = 0;
				current_north_train = find_next_train(north_schedule,current_north_train,0);
				display_train_at(0,0,north_schedule,current_north_train);
				/*
				current_south_train = find_next_train(south_schedule,current_south_train,0);
				display_train_at(DISPLAY_COLS/2,0,south_schedule,current_south_train);
				*/

				global_test_var = 1;
				display_train_at(0,1,north_schedule,    find_next_train(north_schedule,current_north_train,1)    );
				/*
				display_train_at(DISPLAY_COLS/2,1,south_schedule,    find_next_train(south_schedule,current_south_train,1)    );
				*/


			case 2 : // update clock
				calc_time();
				show_clock_at((DISPLAY_COLS-8),0);
				//show_date_at(DISPLAY_COLS/2-1,1);
				state = 255;
			default : // idle and key command recognition
				Tastenabfrage();
				_delay_ms(20);
			break;
		
		}

	

	}


	// end of main
	return 0;
}

















