#include <msp430g2231.h>
#include "i2c.h"

unsigned char radio = 0xC6; // address on si4735 write 0x22
unsigned char expander = 0x20; //address on mcp23016
unsigned char port0 = 0x00; // Port0; can be directly written/read.
unsigned char port1 = 0x01; // Port0; can be directly written/read.
unsigned char readval = 0x00;
unsigned char last = 0x31;
unsigned char rpt = 6;
unsigned char rptcnl = 6;

#define BUTTON 0x3E


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

	__delay_cycles(2000);
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

	__delay_cycles(2000);
	return val;
}

unsigned char lcdsendvol(unsigned char val, unsigned char rpt) {
	int i;
	for(i = 0; i < rpt;i++){

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

	__delay_cycles(2000);
	}
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
	lcdsendc(0x0C); // turn on display
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){

switch( P1IN )
{
case 0xFC: // up
	last = last +1;
	break;
case 0xEE: // Left
	rpt = rpt -1;
	if(rpt == 0){
		rpt = rpt +1;
	}
	rptcnl = 12 - rpt;
	break;
case 0xFA: // right
	rpt = rpt +1;
	if(rpt == 13){
		rpt = rpt -1;
	}
	rptcnl = 12 - rpt;
	break;
case 0xF6: // center
	last = last -4;
	break;
case 0xDE: // down
	last = last -1;
	break;
}

P1IFG &= ~BUTTON; // P1.3 IFG cleared
//__delay_cycles(10000);
}


void main(void) {
	WDTCTL = WDTPW + WDTHOLD;
	__delay_cycles(500000); // start with delay to get lcd ready for init
	i2c_init(); // startup i2c
	exint(); // init the expander ic
	lcdint(); // init the lcd display

	P1IE |= BUTTON; // P1.3 interrupt enabled
	P1IFG &= ~BUTTON; // P1.3 IFG cleared
	P1IES |= BUTTON;
	__enable_interrupt(); // enable all interrupts

	for (;;){

	lcdsendc(0x80);

	lcdsendd(0x39); // 9
	lcdsendd(0x32); // 2
	lcdsendd(0x2E); // .
	lcdsendd(last); // 6
	lcdsendd(0x20);
	lcdsendd(0x4D); // M
	lcdsendd(0x48); // H
	lcdsendd(0x7A); // z

	lcdsendc(0x8E);

	lcdsendd(0x46); // F
	lcdsendd(0x4D); // M

	lcdsendc(0xC0);

	lcdsendd(0x56); // V
	lcdsendd(0x6F); // O
	lcdsendd(0x6C); // L
	lcdsendd(0x20); // space

	lcdsendvol(0xFF, rpt); // box
	lcdsendvol(0x20, rptcnl); // box
}


//	readi2c(expander, port0); // read from i2c (address, register)

//	lcdsendd(readval);

}





