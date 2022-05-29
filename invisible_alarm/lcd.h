#ifndef _LCD_H_
#define _LCD_H_

/************************************************************************/
/*                                                                      */
/* Operates a 16x2 LCD display on port pins D2-D7 (D0-D3 aren't there)  */
/*                                                                      */
/* D2 - Register Select                                                 */
/* D3 - Enable (Operation performed on switch from high to low)         */
/* D4-D7 - Connected to D4-D7. The LCD operates in 4-bit mode.          */
/*                                                                      */
/*                                                                      */
/************************************************************************/

#include <avr/io.h>
#include <util/delay.h>

// Switching this off and on will send the current LCD instruction
// For the 4 bit case, it will send *half* of an instruction each toggle.
#define LCD_RS PORTD2
#define LCD_EN PORTD3

/** Converts a nibble to a hex character */
#define nibble_hex(nibble) (nibble > 9 ? ('A' + nibble - 10) : ('0' + nibble))

/** Sends a byte to the LCD (command or data) */
void lcd_byte(uint8_t b) {
	
	// Send the upper nibble
	PORTD = (PORTD & 0x0F) | (0xF0 & b);
	PORTD |= 1 << LCD_EN;
	_delay_us(1); // Min pulse width is only 140ns
	PORTD ^= 1 << LCD_EN;
	_delay_us(1); // Min pulse cycle is 1.2us
	
	// Send the lower nibble
	PORTD = (PORTD & 0x0F) | (b << 4);
	PORTD |= 1 << LCD_EN;
	_delay_us(1);
	PORTD ^= 1 << LCD_EN;
	_delay_us(1);
}

/** Sends a command to the LCD */
void lcd_command(uint8_t command) {
	
	PORTD &= !(1 << LCD_RS);
	lcd_byte(command);
	if (command == 1 || (command >> 1) == 1) {
		// clear and return home require a 1.53ms delay
		// (this is what the data sheet says, but it needs more)
		_delay_ms(2);
	} else {
		// All other commands (not writes) require 39us
		// The data sheet has failed me before, so I'm adding extra
		_delay_us(50);
	}
}

/** Initializes the LCD to 4 bit mode, no blinking cursor */
void lcd_init() {
	// Set Ports D2-D7 to output
	PORTD = 0x00;
	DDRD = 0xFC;
	_delay_ms(50);

	lcd_command(0x02); // Return Home
	lcd_command(0x28); // Set 4 bit, 2 line, 8 row
	lcd_command(0x01); // Clear display
	lcd_command(0x0C); // Turn display on
	lcd_command(0x06); // Set cursor direction
	
}

/** Clears the LCD */
void lcd_clear() {
	lcd_command(0x01);
}

/** Print a character to the LCD */
void lcd_print_char(char c) {
	PORTD |= 1 << LCD_RS;
	lcd_byte(c);
	// DDRAM changes require a 43us delay
	// The data sheet has failed me before, so I'm adding extra
	_delay_us(50);
}

/** Prints a string to the LCD */
void lcd_print(char* str) {
	int i = 0;
	while (str[i] != '\0') {
		lcd_print_char(str[i]);
		i++;
	}
}

/* Prints a byte in hex (ex. 0x0F) */
void lcd_print_byte(uint8_t hex) {
	// A 32 bit int has 8 hex characters
	lcd_print_char('0');
	lcd_print_char('x');
	uint8_t nibble;
	
	// Upper nibble
	nibble = 0xF & (hex >> 4);
	lcd_print_char(nibble_hex(nibble));
	
	// Lower nibble
	nibble = 0xF & hex;
	lcd_print_char(nibble_hex(nibble));
}

/** Prints an unsigned integer as hex (ex. 0x0000000F) */
void lcd_print_hex(unsigned int hex) {
	// A 32 bit int has 8 hex characters
	lcd_print_char('0');
	lcd_print_char('x');
	uint8_t nibble;
	for (int i = 28; i >= 0; i -= 4) {
		nibble = 0xF & (hex >> i);
		lcd_print_char(nibble_hex(nibble));
	}
	lcd_print_char('E');
}

/** Prints an integer in decimal (ex. -123) */
void lcd_print_int(int32_t num) {
	
	// Zero is a special case
	if (num == 0) {
		lcd_print_char('0');
		return;
	}

	// Handle negative numbers
	if (num < 0) {
		lcd_print_char('-');
		num = -num;
	}
	
	// The longest possible number is 2147483647
	// which is 10 characters long
	char buffer[10];
	int i=0;	
	// The digits will be read backwards. Store them in a buffer.
	while (num > 0) {
		buffer[i] = '0' + num % 10; // Store the lowest digit
		num /= 10; // Cuts off the lowest digit
		i++;
	}
	
	// Print the buffer in reverse
	while (i > 0) {
		i--;
		lcd_print_char(buffer[i]);
	}
}

#endif