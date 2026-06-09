/*	File name: flash_recorder.c
	Description: Data logging terminal
	Language: MPLAB C18
	Target: PIC18F2550
*/
#include "Compiler.h"
#include "MDD File System\internal flash.h"
#include <stdio.h>
#include <timers.h>
#include "tinyFAT.h"

#define BLK_HIGH 0xe2 // 200Hz 0.005s
#define BLK_LOW 0x15 // 25Hz 0.04s
#define BLK_NORM 0x8a // 50Hz 0.02s
#define AD_INTV t1_min

BYTE t1_count;
BYTE t1_05sec;
BYTE t1_min;
BYTE t1_hour;
BYTE t1_speed;

BYTE old_time;
BYTE old_sw2;

const rom char far rec_fname[] = {
	"RECORDERCSV"
};

BYTE ee_write(BYTE address, BYTE data){
	EEDATA = data;
	EEADR = address;

	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON1bits.WREN = 1;
	INTCONbits.GIE = 0;

	EECON2 = 0x55;
	EECON2 = 0x0AA;
	EECON1bits.WR = 1; 
	while(EECON1bits.WR);

	EECON1bits.WREN = 0;
	INTCONbits.GIE = 1;
	return(EECON1bits.WRERR);
}

BYTE ee_read(BYTE address){
    EEADR = address;
    EECON1bits.CFGS = 0;
    EECON1bits.EEPGD = 0;
    EECON1bits.RD = 1;
    return(EEDATA);
}

BOOL Switch2IsPressed(void) {
	if(sw2 != old_sw2) {
		old_sw2 = sw2; // Save new value
		if(sw2 == 0) // If pressed
			return TRUE; // Was pressed
	}//end if
	return FALSE; // Was not pressed
}//end Switch2IsPressed

WORD ReadPOT(void){
	WORD_VAL w;

	ADCON0bits.GO = 1;
	while(ADCON0bits.NOT_DONE);

	w.v[0] = ADRESL;
	w.v[1] = ADRESH;

	return w.Val;
}//end ReadPOT

void init_wink(void){
	ADCON1 = 0x06; // AN9- to digital
	mInitAllLEDs();

	OpenTimer1(
		TIMER_INT_ON &
		T1_8BIT_RW &
		T1_SOURCE_INT &
		T1_PS_1_8 &
		T1_OSC1EN_OFF &
		T1_SYNC_EXT_OFF
	);
	IPR1bits.TMR1IP = 0;
	PIR1bits.TMR1IF = 0;
}

void flash_recorder(void){
	WORD i;
	BYTE dc;
	WORD_VAL ad_val;

	old_sw2 = sw2;
	t1_count = 0;
	t1_speed = BLK_HIGH;

	mInitPOT(); // All analog
	init_wink(); // -AN8 analog

	RCONbits.IPEN=1;
	INTCONbits.GIEL=1;
	INTCONbits.GIE=1;

#if defined(USE_USB_BUS_SENSE_IO)
	if(USB_BUS_SENSE){
		PIE1bits.TMR1IE = 0;
		INTCONbits.GIEL=0;
		INTCONbits.GIE=0;
		mLED_1_Off();
		mLED_2_Off();
		return;
	}
#else
	for(i = 0; i < 80; i++){
		while(t1_count != 0);
		while(t1_count == 0);
		if(Switch2IsPressed()){
			PIE1bits.TMR1IE = 0;
			INTCONbits.GIEL=0;
			INTCONbits.GIE=0;
			mLED_1_Off();
			mLED_2_Off();
			return;
		}
	}

	while(Switch2IsPressed());
#endif

	t1_speed = BLK_NORM; // 50Hz 0.02s
	t1_05sec = 0;
	t1_min = 0;
	t1_hour = 0;

	init_tinyFAT();
	for(i = 0; i < 51; i++){
		old_time = AD_INTV;
		ad_val.Val = ReadPOT();
		ee_write(0, i + 1);
		ee_write(i * 2 + 1, ad_val.v[0]);
		ee_write(i * 2 + 2, ad_val.v[1]);

		while(AD_INTV == old_time){
			if(sw2 == 0){
				break;
			}
		}
		if(sw2 == 0)
			break;
	}
	while(sw2 == 0);

	dc = ee_read(0);
	file_open(rec_fname);
	for(i = 0; i < dc; i++){
		ad_val.v[0] = ee_read(i * 2 + 1);
		ad_val.v[1] = ee_read(i * 2 + 2);

		sprintf(
			(char *)&msd_buffer[i * 10],
			(const rom char far *)"%3d,%4d\r\n",
			i + 1,
			ad_val.Val
		);
	}
	FILE_SIZE = dc * 10;
	file_write();
	file_close();

	PIE1bits.TMR1IE = 0;
	mLED_1_Off();
	mLED_2_Off();

	while(1);
}