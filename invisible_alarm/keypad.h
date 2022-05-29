#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define KEY_1 0
#define KEY_4 1
#define KEY_7 2
#define KEY_STAR 3
#define KEY_ASTERISK 3
#define KEY_2 4
#define KEY_5 5
#define KEY_8 6
#define KEY_0 7
#define KEY_3 8
#define KEY_6 9
#define KEY_9 10
#define KEY_HASH 11
#define KEY_POUND 11
#define KEY_A 12
#define KEY_B 13
#define KEY_C 14
#define KEY_D 15

uint16_t keypad = 0x0000;

const char KEYS[16] = {
	'1', '4', '7', '*',
	'2', '5', '8', '0',
	'3', '6', '9', '#',
	'A', 'B', 'C', 'D'
};

/* Called whenever a key goes from unpressed to pressed */
void (*keys_on_press)(int key) = NULL;

/** 
 *  Reads the button states of the keypad as 16
 *  bits, where each bit represents a character.
 *  From the lowest bit, the bits represent:
 *  1 2 3 A 4 5 6 B 7 8 9 C * 0 # D
 */
uint16_t keys_poll() {
	// Store data direction of non-keypad pins
	// If this changes in an interrupt, bummer

	// Disable interrupts to prevent 'hold' from
	// becoming inaccurate.
	//cli();
	// Hold preserves the data direction of the upper nibble of port b
	uint8_t hold = 0xF0 & DDRB;
	uint16_t result = 0x0000;

	DDRB = 0B0000 | hold;
	_delay_us(1);
	PORTB |= 0x0F;
	// Row 1
	DDRB = 0B0001 | hold;
	PORTB |= 0x0F;
	_delay_us(1);
	result |= (PINC & 0x0F);
	_delay_us(1);
	// Row 2
	DDRB = 0B0010 | hold;
	PORTB |= 0x0F;
	_delay_us(1);
	result |= (PINC & 0x0F) << 4;
	_delay_us(1);
	// Row 3
	DDRB = 0B0100 | hold;
	PORTB |= 0x0F;
	_delay_us(1);
	result |= (PINC & 0x0F) << 8;
	_delay_us(1);
	// Row 4
	DDRB = 0B1000 | hold;
	PORTB |= 0x0F;
	_delay_us(1);
	result |= (PINC & 0x0F) << 12;
	_delay_us(1);
	
	// Nothing
	DDRB = 0B0000 | hold;
	//sei();
	
	// Run callbacks
	if (keys_on_press != NULL) {
		for (int i = 0; i < 16; i++) {
			if ((1 & (result >> i)) &! (1 & (keypad >> i))) {
				(*keys_on_press)(i);
			}
		}
	}
	
	keypad = result;
	return keypad;
}

uint8_t keys_get_from_state(uint16_t state, char key) {
	for (int i = 0; i < 16; i++) {
		if (key == KEYS[i]) {
			return 0x01 & (state << i);
		}
	}
	return 0;
}

uint8_t keys_get(char key) {
	return keys_get_from_state(keypad, key);
}

#endif