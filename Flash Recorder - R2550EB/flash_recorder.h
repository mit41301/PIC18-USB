/*	File name: flash_recorder.h
	Description: Data logging terminal
	Language: MPLAB C18
	Target: PIC18F2550
*/
extern BYTE t1_count;
extern BYTE t1_speed;
extern BYTE t1_05sec;
extern BYTE t1_min;
extern BYTE t1_hour;

void flash_recorder(void);
