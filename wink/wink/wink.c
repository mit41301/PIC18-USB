/*	File name: wink.c
	Description: Interrupt example
	Language: MPLAB C18
	Target: PIC18F14K50
*/

#include <p18cxxx.h>
#include <timers.h>

#pragma config MCLRE  = OFF
#pragma config PWRTEN = OFF
#pragma config BOREN  = OFF
#pragma config BORV   = 30
#pragma config WDTEN  = OFF
#pragma config WDTPS  = 32768
#pragma config STVREN = ON
#pragma config FOSC   = HS
#pragma config PLLEN  = ON
#pragma config CPUDIV = NOCLKDIV
#pragma config USBDIV = OFF
#pragma config FCMEN  = OFF
#pragma config IESO   = OFF
#pragma config HFOFST = OFF
#pragma config LVP    = OFF
#pragma config XINST  = OFF
#pragma config BBSIZ  = OFF
#pragma config CP0    = OFF
#pragma config CP1    = OFF
#pragma config CPB    = OFF
#pragma config WRT0   = OFF
#pragma config WRT1   = OFF
#pragma config WRTB   = OFF
#pragma config WRTC   = OFF
#pragma config EBTR0  = OFF
#pragma config EBTR1  = OFF
#pragma config EBTRB  = OFF

void isr_high(void);
void isr_low(void);

#pragma interrupt isr_high
#pragma interruptlow isr_low save = WREG,BSR,STATUS

#pragma code h_int_vect = 0x0008
void _h_isr (void){
     _asm goto isr_high _endasm
}

#pragma code l_int_vect = 0x0018
void _l_isr (void){
     _asm goto isr_low _endasm
}

#pragma code

unsigned char count;
unsigned char button;

void isr_high(){
	if(PIR1bits.TMR1IF){
		PIR1bits.TMR1IF = 0;
		if(++count == 8){
			count = 0;
			if(PORTCbits.RC0)
				PORTCbits.RC0 = 0;
			else
				PORTCbits.RC0 = 1;
		}
	}
}

void isr_low(){
	if(INTCONbits.RABIF){
		button = PORTAbits.RA3;
		INTCONbits.RABIF = 0;

		if(button == 0){
			if(PORTCbits.RC1)
				PORTCbits.RC1 = 0;
			else
				PORTCbits.RC1 = 1;
		}
	}
}

void main(void){
	count = 0;
	button = 0;

	ANSELbits.ANS4 = 0;
	LATCbits.LATC0 = 0;
	TRISCbits.TRISC0 = 0;

	ANSELbits.ANS5 = 0;
	LATCbits.LATC1 = 0;
	TRISCbits.TRISC1 = 0;


	INTCONbits.RABIE = 1;
	INTCON2bits.RABIP = 0;
	INTCONbits.RABIF = 0;
	IOCAbits.IOCA3 = 1;

	OpenTimer1(
		TIMER_INT_ON &
		T1_8BIT_RW &
		T1_SOURCE_INT &
		T1_PS_1_8 &
		T1_OSC1EN_OFF &
		T1_SYNC_EXT_OFF
	);
	IPR1bits.TMR1IP = 1;
	PIR1bits.TMR1IF = 0;

	RCONbits.IPEN=1;
	INTCONbits.GIEH=1;
	INTCONbits.GIEL=1;

	while(1);
}