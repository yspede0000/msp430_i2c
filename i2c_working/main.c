#include <msp430g2231.h>
#include "i2c.h"

unsigned char radio = 0xC6; // address on si4735 write 0x22
unsigned char expander = 0x20; //address on mcp23016
unsigned char port0 = 0x00; // Port0; can be directly written/read.
unsigned char port1 = 0x01; // Port0; can be directly written/read.
unsigned char readval = 0x00;

void exint(void) {
	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(0x06); // direction of port 0
	i2c_write8(0x00); // all port0 output
	i2c_stop();

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(0x07); // direction of port 1
	i2c_write8(0x00); // all port0 output
	i2c_stop();
}

unsigned char lcdsendc(unsigned char val) {

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x00); // Control register / set enable low
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x01); // Control register / set enable high
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port0);
	i2c_write8(val); // data to be send sign.
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x00); // Control register / set enable low
	i2c_stop();

	__delay_cycles(10000);
	return val;
}

unsigned char lcdsendd(unsigned char val) {

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x02); // Control register / set enable low
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x03); // Control register / set enable high
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port0);
	i2c_write8(val); // data to be send
	i2c_stop();

	__delay_cycles(5000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(0x02); // Control register / set enable low
	i2c_stop();

	__delay_cycles(10000);
	return val;
}

unsigned char readi2c(unsigned char val, unsigned char val1) {
	i2c_start();
	i2c_write8(val << 1); // what chip to point on
	i2c_write8(val1); // what register to read from

	i2c_rpt(); // Repeated start bit.
	i2c_write8(val << 1 | 1);
	readval = i2c_read8(0x0);
	i2c_stop();

	return (readval);
}

void lcdint(void) {
	lcdsendc(0x30); //int reset word
	lcdsendc(0x30); //int reset word
	lcdsendc(0x30); //int reset word
	lcdsendc(0x38); // interface
	lcdsendc(0x08); // turn off display
	lcdsendc(0x01); // clear
	lcdsendc(0x06); // courser to right
	lcdsendc(0x0F); // turn on display
}

void main(void) {
	WDTCTL = WDTPW + WDTHOLD;
	__delay_cycles(500000); // start with delay to get lcd ready for init
	i2c_init(); // startup i2c
	exint(); // init the expander ic
	lcdint(); // init the lcd display

	lcdsendd(0x48); // H
	lcdsendd(0x45); // E
	lcdsendd(0x4C); // L
	lcdsendd(0x4C); // L
	lcdsendd(0x4F); // O

	readi2c(expander, port0); // read from i2c (address, register)

	lcdsendd(readval);

}

