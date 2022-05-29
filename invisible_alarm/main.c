/*
 * invisible_alarm.c
 *
 * Created: 9/28/2020 5:15:21 PM
 * Author : kenkr
 */ 

// 
#define F_CPU 15989750
#define NULL 0

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "keypad.h"

typedef uint8_t bool;
#define FALSE 0
#define TRUE !FALSE

// hh-mm-A/P
char time_buffer[6] = "0000A";

int alarm_time = -1;
int current_time = -1;

#define NOTE_FREQUENCIES_LENGTH 12
int NOTE_FREQUENCIES[NOTE_FREQUENCIES_LENGTH] = 
	{0, 784, 880, 988, 1047, 1175, 1319, 1397, 1568, 1760, 1976, 2093};
//      G    A    B    C     D     E     F     G     A     B     C

#define NOTE_RS 0
#define NOTE_G4 1
#define NOTE_A4 2
#define NOTE_B4 3
#define NOTE_C5 4
#define NOTE_D5 5
#define NOTE_E5 6
#define NOTE_F5 7
#define NOTE_G5 8
#define NOTE_A5 9
#define NOTE_B5 10
#define NOTE_C6 11

#define MAX_TUNE_LENGTH 64
int tune_length = 22;
uint8_t tune[MAX_TUNE_LENGTH] = {
	NOTE_C5, NOTE_RS, NOTE_A4, NOTE_C5,
	NOTE_RS, NOTE_C5, NOTE_C5, NOTE_A4,
	NOTE_C5, NOTE_RS, NOTE_D5, NOTE_C5,
	NOTE_RS, NOTE_A4, NOTE_A4, NOTE_A4,
	NOTE_RS, NOTE_RS, NOTE_RS, NOTE_RS
};
int tune_counter = -1;

void print_keypad(uint16_t keypad) {
	for (int i = 15; i >=0; i--) {
		lcd_print_char((1 & (keypad >> i)) ? KEYS[i] : ' ');
	}
}

void print_key(int key) {
	lcd_clear();
	lcd_print_char(KEYS[key]);
}

void writeTimeBuffer(int key) {
	char c = KEYS[key];
	if (c >= '0' && c <= '9') {
		time_buffer[0] = time_buffer[1];
		time_buffer[1] = time_buffer[2];
		time_buffer[2] = time_buffer[3];
		time_buffer[3] = c;
	} else if (c == 'A') {
		if (time_buffer[4] == 'A') {
			time_buffer[4] = 'P';
		} else {
			time_buffer[4] = 'A';
		}
	}
}

int32_t readTimeBuffer() {
	int32_t minutes =
		(time_buffer[3] - '0') +
		(time_buffer[2] - '0') * 10 +
		(time_buffer[1] - '0') * 60 +
		(time_buffer[0] - '0') * 600;
		if (time_buffer[4] == 'P') {
			minutes += 12 * 60;
		}
	return minutes * 60;
}

void printTimeBuffer() {
	lcd_print_char(time_buffer[0]);
	lcd_print_char(time_buffer[1]);
	lcd_print_char(':');
	lcd_print_char(time_buffer[2]);
	lcd_print_char(time_buffer[3]);
	if (time_buffer[4] == 'A') {
		lcd_print(" AM");
	} else {
		lcd_print(" PM");
	}
}

// Returns the time buffer as a number of seconds
void printTime(int32_t s) {
	if (s < 0) {
		lcd_print_int(s);
		return;
	}
	int seconds = s % 60;
	int minutes = (s / 60) % 60;
	int hours = s / 3600;
	lcd_print_char('0' + hours / 10);
	lcd_print_char('0' + hours % 10);
	lcd_print_char(':');
	lcd_print_char('0' + minutes / 10);
	lcd_print_char('0' + minutes % 10);
	lcd_print_char(':');
	lcd_print_char('0' + seconds / 10);
	lcd_print_char('0' + seconds % 10);
}

uint32_t speaker_delay = 0;
uint32_t speaker_counter = 0;

// set the sound in hz
void setSound(int hz) {
	if (hz == 0) {
		speaker_delay = 0; // This plays no sound
	} else {
		speaker_delay = (F_CPU / hz) / 256;
	}
}

// get the sound in hz
int getSound() {
	if (speaker_delay == 0) {
		return 0;
	} else {
		return F_CPU / (speaker_delay * 256);
	}
}

void clock_init() {
	cli();
	TCCR0B = 0b00000001;
	TIMSK0 |= OCF0A;
	DDRB |= 0b00000100;
	sei();
}

int32_t timer_ticks = 0;
ISR (TIMER0_OVF_vect) {
	timer_ticks += 256;
	if (timer_ticks > F_CPU) {
		current_time++;
		timer_ticks -= F_CPU;
	}
	speaker_counter ++;
	if (speaker_delay > 0 && speaker_counter > speaker_delay) {
		speaker_counter -= speaker_delay;
		PORTB ^= 0b00100000;
	}
}

// Gets current time in milliseconds
uint32_t get_ms() {
	return current_time * 1000 + timer_ticks / (F_CPU / 1000);
}

void entryCallback(int key) {
	writeTimeBuffer(key);
	int transpose = 4 * (key % 4) + key / 4;
	if (transpose < NOTE_FREQUENCIES_LENGTH) {
		setSound(NOTE_FREQUENCIES[transpose]);
	}
	lcd_clear();
	printTimeBuffer();
}

/************************************************************************/
/* States                                                               */
/************************************************************************/

#define STATE_WAIT 0
#define STATE_SET_ALARM 1
#define STATE_SET_TUNE  2
#define STATE_SET_CLOCK 3
#define STATE_TEST      4
#define STATE_ALARM     5

int state = STATE_WAIT;


void callback_set_alarm(int key) {
	// Let the user enter a time
	writeTimeBuffer(key);
	lcd_clear();
	printTimeBuffer();
	if (key == KEY_HASH) {
		int32_t time = readTimeBuffer();
		cli();
		alarm_time = time;
		sei();
		state = STATE_WAIT;
	}
}

#define note_duration 500
// The time this note started playing
int32_t note_time;
void callback_set_tune(int key) {
	if (key == KEY_HASH) {
		state = STATE_TEST;
	}
	int transpose = 4 * (key % 4) + key / 4;
	if (transpose < NOTE_FREQUENCIES_LENGTH) {
		note_time = get_ms();
		setSound(NOTE_FREQUENCIES[transpose]);
		tune[tune_length] = transpose;
		tune_length++;
	}
	if (tune_length >= MAX_TUNE_LENGTH){
		state = STATE_TEST;
	}
}

void callback_set_clock(int key) {
	// Let the user enter a time
	writeTimeBuffer(key);
	lcd_clear();
	printTimeBuffer();
	if (key == KEY_HASH) {
		int32_t time = readTimeBuffer();
		cli();
		current_time = time;
		sei();
		state = STATE_WAIT;
	}
}

void callback_test() {
	printTime(alarm_time);
	
}

void callback_alarm(int key) {
	if (key == KEY_HASH) {
		state = STATE_WAIT;
	}
}

void callback_wait(int key) {
	char symbol = KEYS[key];
	switch (symbol) {
		case 'A':
			// Set Alarm
			state = STATE_SET_ALARM;
			lcd_clear();
			lcd_print("Set Alarm");
			break;
		case 'B':
			// Set Sound
			tune_length = 0;
			state = STATE_SET_TUNE;
			lcd_clear();
			lcd_print("Set Tune");
			break;
		case 'C':
			// Set Clock
			state = STATE_SET_CLOCK;
			lcd_clear();
			lcd_print("Set Clock");
			break;
		case 'D':
			// Test
			state = STATE_TEST;
			lcd_clear();
			lcd_print("Test Alarm");
			_delay_ms(1000);
			lcd_clear();
			printTime(alarm_time);
			break;
		default:
			lcd_clear();
			printTime(current_time);
	}
}

void key_callback(int key) {
	switch (state) {
		case STATE_WAIT:
			callback_wait(key);
			break;
		case STATE_SET_ALARM:
			callback_set_alarm(key);
			break;
		case STATE_SET_TUNE:
			callback_set_tune(key);
			break;
		case STATE_SET_CLOCK:
			callback_set_clock(key);
			break;
		case STATE_TEST:
			callback_test();
			break;
		default:
			state = STATE_WAIT;
	}
}

int main(void)
{
	lcd_init();
	clock_init();
	PORTB |= 0b00000100;
	DDRC = 0x00;
	DDRB = 0x2F;
	
	setSound(0);
	keys_on_press = &key_callback;
	state = STATE_WAIT;
	
	lcd_clear();
	lcd_print("Hello, World!");
	bool before_alarm = FALSE;
    while (1) 
    {
		// Handle inputs first
		keys_poll();

		// See if the alarm should go off
		bool was_before_alarm = before_alarm;
		before_alarm = current_time < alarm_time;
		if (was_before_alarm &! before_alarm) {
			state = STATE_ALARM;
			setSound(0);
			tune_counter = -1;
		}
		
		// If a note is playing, make sure it turns off eventually
		if (getSound() && get_ms() > note_time + note_duration) {
			setSound(0);
		}
		
		// If this is a test, make sure all the notes are played,
		// then set the state to wait
		if (state == STATE_TEST) {
			if (!getSound()) {
				tune_counter++;
				if (tune_counter < tune_length) {
					setSound(NOTE_FREQUENCIES[tune[tune_counter]]);
					note_time = get_ms();
				} else {
					tune_counter = -1;
					state = STATE_WAIT;
				}
			}
		}
		
		// If the alarm is going off, play all the notes until
		// the alarm interrupt goes off
		if (state == STATE_ALARM) {
			if (!getSound()) {
				tune_counter++;
				if (tune_counter < tune_length) {
					setSound(NOTE_FREQUENCIES[tune[tune_counter]]);
					note_time = get_ms();
				} else {
					tune_counter = -8;
				}
			}
		}
		
		_delay_ms(20);
    }
}

