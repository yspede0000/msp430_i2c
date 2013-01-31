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
char reg = 0;
char data = 1;
char menuval = 0;
short menu_state = 0;

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

void menuval_switch(void);
void i2c_update(void);

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

void lcdsend(unsigned char value, unsigned char reg){
	char C1;
	char C2;

	if(reg == 0){
		C1 = 0x00;
		C2 = 0x01;
	}

	if(reg == 1){
		C1 = 0x02;
		C2 = 0x03;
	}

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(C1); // Control register / set enable low
	i2c_stop();

	__delay_cycles(2000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(C2); // Control register / set enable high
	i2c_stop();

	__delay_cycles(2000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port0);
	i2c_write8(value); // data to be send sign.
	i2c_stop();

	__delay_cycles(2000);

	i2c_start();
	i2c_write8(expander << 1);
	i2c_write8(port1);
	i2c_write8(C1); // Control register / set enable low
	i2c_stop();
}

void lcdsendvol(unsigned char val, unsigned char rpt) {
	int i;
	for(i = 0; i < rpt;i++){
		lcdsend(val, data);
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
	lcdsend(0x30, reg); //int reset word
	lcdsend(0x30, reg); //int reset word
	lcdsend(0x30, reg); //int reset word
	lcdsend(0x38, reg); // interface
	lcdsend(0x08, reg); // turn off display
	lcdsend(0x01, reg); // clear
	lcdsend(0x06, reg); // courser to right
	lcdsend(0x0C, reg); // turn on display
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){

switch ( P1IN ){

case 0xFC: // up
	switch(menuval){

	case MAIN_DISPLAY:
		last = last +1;
		freq = freq + 1;
		tunefreq = tunefreq + 12;
		menuval_switch();
		i2c_update();
		break;

	case MENU:
		break;

	case SLEEP:
		break;

	case SETTING:
		break;

	case BASS:
		break;

	case TREBLE:
		break;
	}
	break;


	case 0xEE: // Left
		switch(menuval){

			case MAIN_DISPLAY:
				volrpt = volrpt -1;
					if(volrpt == 0){
						volrpt = volrpt +1;
					}
					volrptcnl = 12 - volrpt;
					menuval_switch();
					i2c_update();
					break;

			case MENU:
				menuval=SLEEP;
				menuval_switch();
				//mute
				break;

			case SLEEP:
					break;

			case SETTING:
				menuval=BASS;
				menuval_switch();
					break;

			case BASS:
					break;

			case TREBLE:
					break;
			}
		break;

		case 0xFA: // right
			switch(menuval){

				case MAIN_DISPLAY:
					volrpt = volrpt +1;
						if(volrpt == 13){
							volrpt = volrpt -1;
						}
						volrptcnl = 12 - volrpt;
						menuval_switch();
						i2c_update();
						break;

				case MENU:
					menuval=SETTING;
					menuval_switch();
				break;

				case SLEEP:
					break;

				case SETTING:
					menuval=TREBLE;
					menuval_switch();
					break;

				case BASS:
					break;

				case TREBLE:
					break;
				}
			break;

			case 0xF6: // center
				switch(menuval){

					case MAIN_DISPLAY:
						menuval=MENU;
						menuval_switch();
						break;

					case MENU:
								break;
					case SLEEP:
								break;
					case SETTING:
								break;
					case BASS:
								break;
					case TREBLE:
								break;
					}
				break;

				case 0xDE: // down
					switch(menuval){

						case MAIN_DISPLAY:
							last = last -1;
								freq = freq -1;
								tunefreq = tunefreq - 12;
								menuval_switch();
								i2c_update();
								break;
						case MENU:
							--menuval;
							menuval_switch();
						break;
						case SLEEP:
							menuval=MAIN_DISPLAY;
							menuval_switch();
								break;
						case SETTING:
							menuval=MENU;
							menuval_switch();
								break;
						case BASS:
							menuval=SETTING;
							menuval_switch();
								break;
						case TREBLE:
							menuval=SETTING;
							menuval_switch();
								break;
						}
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
	lcdsend(0x80, reg);
	switch (counter) {
	case 3:
		counter = 0;
		while (buffer[counter]) {
			if (counter == 2) {
				lcdsend(0x2E, data);
			}
			lcdsend(buffer[counter], data);
			counter++;
		}
		break;
	case 4:
		counter = 0;
		while (buffer[counter]) {
			if (counter == 3) {
				lcdsend(0x2E, data);
			}
			lcdsend(buffer[counter], data);
			counter++;
		}
		break;
	}
}

void PrintStr(char *Text)
{
    char *c;

    c = Text;

    while ((c != 0) && (*c != 0))
    {
    	lcdsend(*c, data);
        c++;
    }
}

void lcddisplay(int freq, char buffer[32], unsigned char volrpt,
		unsigned char volrptcnl) {
	lcdsendfreq(freq, buffer);
	PrintStr("MHz");
	lcdsend(0x8E, reg);
	PrintStr("FM");
	lcdsend(0xC0, reg);
	PrintStr("Vol ");
	lcdsendvol(0xFF, volrpt); // box
	lcdsendvol(0x20, volrptcnl); // box
}


void menuval_switch(void) {
	switch (menuval) {
	case STARTUP:
		menu_state = STARTUP;
		PrintStr("Super radio 3000");
		__delay_cycles(900000);
		lcdsend(0x01, reg); // clear display
		menuval++;
	case MAIN_DISPLAY: // main display
		menu_state = MAIN_DISPLAY;
		lcddisplay(freq, buffer, volrpt, volrptcnl);

		break;
	case MENU: // menu
		menu_state = MENU;
		lcdsend(0x01, reg);
		PrintStr("Menu");
		lcdsend(0xC0, reg);
		PrintStr("sleep    setting");
		//lcdsendc(0x01);
		//menuval++;
		break;
	case SLEEP: // shutdown
		//mute radio
		lcdsend(0x01, reg);
		menu_state = SLEEP;
		lcdsend(0xC0, reg);
		PrintStr("Slepping...");
		//menuval++;
		//lcdsendc(0x01);
		break;
	case SETTING: // setting
		menu_state = SETTING;
		lcdsend(0x01, reg);
		PrintStr("Setting");
		lcdsend(0xC0, reg);
		PrintStr("Bass      Treble");
		break;
	case BASS: // bass
		menu_state = BASS;
	case TREBLE: // treble
		menu_state = TREBLE;
		break;
	}
}

void i2c_update(void) {
	frequencyB = tunefreq;
	frequencyH = frequencyB >> 8;
	frequencyL = (char) (frequencyB & 0xFF);
	__delay_cycles(100000);
	i2c_start();
	i2c_write8(0xC0); // address
	i2c_write8(frequencyH); // pll freq
	i2c_write8(frequencyL); // pll freq
	i2c_write8(0xB8); // control reg
	i2c_write8(0x10); // Control reg
	i2c_write8(0x00); // Control reg
	i2c_stop();
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
	menuval_switch();
	i2c_update();
for(;;) {




}


}









