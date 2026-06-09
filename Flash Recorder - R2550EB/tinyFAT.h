/*	File name: tinyFAT.h
	Description: People's File system
	Language: MPLAB C18
	Target: PIC18F2550
*/
typedef struct
{
	char DIR_Name[8];
	char DIR_Extension[3];
	BYTE DIR_Attr;
	BYTE DIR_NTRes;
	BYTE DIR_CrtTimeTenth;
	WORD DIR_CrtTime;
	WORD DIR_CrtDate;
	WORD DIR_LstAccDate;
	WORD DIR_FstClusHI;
	WORD DIR_WrtTime;
	WORD DIR_WrtDate;
	WORD DIR_FstClusLO;
	DWORD DIR_FileSize;
}DIRENT;

extern DIRENT dir_root;
#define FILE_SIZE dir_root.DIR_FileSize
extern volatile char msd_buffer[512];

void init_tinyFAT(void);
BYTE file_open(const rom char far*);
void file_write(void);
void file_close(void);
BYTE file_remove(const rom char far*);
