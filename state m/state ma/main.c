#include <msp430g2452.h>
#include "i2c.h"

unsigned char radio = 0xC6; // address on si4735 write 0x22
unsigned char expander = 0x20; //address on mcp23016
unsigned char port0 = 0x00; // Port0; can be directly written/read.
unsigned char port1 = 0x01; // Port0; can be directly written/read.
unsigned char readval = 0x00;
unsigned char last = 0x31;
unsigned char volrpt = 6;
unsigned char volrptcnl = 6;
char buffer[6]="0";
short freq = 926;

char menuval = 0;

typedef enum {
	STARTUP,
	MAIN_DISPLAY,
	MENU,
	SLEEP,
	SETTING,
	BASS,
	TREBLE
} STATES;

unsigned int tunefreq = 11331.1;
unsigned int frequencyB;
unsigned char frequencyH=0;
unsigned char frequencyL=0;
#define BUTTON 0x3E

extern void CSL_init(void);
unsigned char TXData;
unsigned char TXByteCtr;

char* itoa(int value, char* result, int base) {
                // check that the base if valid
                if (base < 2 || base > 36) { *result = '\0'; return result; }

                char* ptr = result, *ptr1 = result, tmp_char;
                int tmp_value;

                do {
                        tmp_value = value;
                        value /= base;
                        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
                } while ( value );

                // Apply negative sign
                if (tmp_value < 0) *ptr++ = '-';
                *ptr-- = '\0';
                while(ptr1 < ptr) {
                        tmp_char = *ptr;
                        *ptr--= *ptr1;
                        *ptr1++ = tmp_char;
                }
                return result;
        }

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

void lcdsendc(unsigned char val) {

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

}

void lcdsendd(unsigned char val) {

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

void lcdsendvol(unsigned char val, unsigned char rpt) {
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

}

/*
unsigned char readi2c(unsigned char val, unsigned char val1) {
	i2c_start();
	i2c_write8(val << 1); // what chip to point on
	i2c_write8(val1); // what register to read from

//	i2c_rpt(); // Repeated start bit.
	i2c_write8(val << 1 | 1);
	readval = i2c_read8(0x0);
	i2c_stop();

	return (readval);
}
*/
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
	freq = freq + 1;
	tunefreq = tunefreq + 12;
	break;

case 0xEE: // Left
	volrpt = volrpt -1;
	if(volrpt == 0){
		volrpt = volrpt +1;
	}
	volrptcnl = 12 - volrpt;
	break;
case 0xFA: // right
	volrpt = volrpt +1;
	if(volrpt == 13){
		volrpt = volrpt -1;
	}
	volrptcnl = 12 - volrpt;
	break;
case 0xF6: // center

	break;

case 0xDE: // down
	last = last -1;
	freq = freq -1;
	tunefreq = tunefreq - 12;
	break;
}

P1IFG &= ~BUTTON; // P1.3 IFG cleared
//__delay_cycles(10000);
}

void lcdsendfreq(int freq, char buffer[32]) {
	itoa(freq, buffer, 10);
	int counter = 0;
	while (buffer[counter]) {
		counter++;
	}
	lcdsendc(0x80);
	switch (counter) {
	case 3:
		counter = 0;
		while (buffer[counter]) {
			if (counter == 2) {
				lcdsendd(0x2E);
			}
			lcdsendd(buffer[counter]);
			counter++;
		}
		break;
	case 4:
		counter = 0;
		while (buffer[counter]) {
			if (counter == 3) {
				lcdsendd(0x2E);
			}
			lcdsendd(buffer[counter]);
			counter++;
		}
		break;
	}
}

void lcddisplay(int freq, char buffer[32], unsigned char volrpt,
		unsigned char volrptcnl) {
	lcdsendfreq(freq, buffer);
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
	lcdsendvol(0xFF, volrpt); // box
	lcdsendvol(0x20, volrptcnl); // box
}

void PrintStr(char *Text)
{
    char *c;

    c = Text;

    while ((c != 0) && (*c != 0))
    {
    	lcdsendd(*c);
        c++;
    }
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

for(;;){

	frequencyB=tunefreq;

	frequencyH=frequencyB>>8;

	frequencyL=(char)(frequencyB&0xFF);

	__delay_cycles(100000);
	i2c_start();
	i2c_write8(0xC0); // address
	i2c_write8(frequencyH); // pll freq
	i2c_write8(frequencyL); // pll freq
	i2c_write8(0xB8); // control reg
	i2c_write8(0x10); // Control reg
	i2c_write8(0x00); // Control reg
	i2c_stop();



		switch ( menuval ){

		case STARTUP :
			PrintStr("Super radio 3000");
			__delay_cycles(2000000);
			lcdsendc(0x01); // clear display
			menuval ++;
			break;

		case MAIN_DISPLAY : // main display
			lcddisplay(freq, buffer, volrpt, volrptcnl);
			lcdsendc(0x01);
			menuval ++;
			break;

		case MENU : // menu
			PrintStr("Menu");
			lcdsendc(0xC0);
			PrintStr("sleep    setting");
			lcdsendc(0x01);
			menuval ++;
			break;

		case SLEEP : // shutdown
			//mute radio

			lcdsendc(0xC0);
			PrintStr("Slepping...");
			menuval ++;
			lcdsendc(0x01);
			break;

		case SETTING : // setting
			lcdsendc(0x01);
			PrintStr("Setting");
			lcdsendc(0xC0);
			PrintStr("Bass      Treble");
			break;

		case BASS : // bass

		case TREBLE : // treble


break;




}


}



}





