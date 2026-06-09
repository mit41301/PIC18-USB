/*	File name: tinyFAT.c
	Description: People's File system
	Language: MPLAB C18
	Target: PIC18F2550
*/
#include "Compiler.h"
#include "MDD File System\internal flash.h"
#include <string.h>

extern volatile char msd_buffer[512];
extern ROM BYTE FAT0[512];
extern ROM BYTE RootDirectory0[512];

typedef struct{
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
} DIRENT;
DIRENT dir_root;
BYTE dir_entry;
BYTE file_is_open;

void init_tinyFAT(void){
	file_is_open = 0;
}

BYTE dir_load(const rom char far* fname){
	BYTE i;
	ROM far DIRENT *dir;

	dir = (ROM far DIRENT *)RootDirectory0;
	for(i = 0; i < 16; i++){
		if(memcmppgm(
			&dir[i].DIR_Name[0],
			fname,
			11
		) == 0){
		memcpypgm2ram(
		&dir_root, 
		(const rom far void*)
		&RootDirectory0[i * 32], 
		32
		);
		return i;
		}
	}
	return 0xff;
}

void dir_save(BYTE entry){
	DIRENT *dir;

	dir = (DIRENT *)msd_buffer;
	MDD_IntFlash_SectorRead(
		3, (BYTE* )msd_buffer
	);
	memcpy(
		(void*)&dir[entry],
		(void*)&dir_root,
		32
	);
	MDD_IntFlash_SectorWrite(
		3, (BYTE* )msd_buffer, 
		1
	);
}

BYTE dir_search_empty(void){
	BYTE i;
	char c;
	ROM far DIRENT *dir;

	dir = (ROM far DIRENT *)RootDirectory0;
	for(i = 0; i < 16; i++){
		c = dir[i].DIR_Name[0];
		if((c == 0) || (c == 5) || (c == 0xe5))
			return i;
	}
	return 0xff;
}

WORD fat_read(WORD ccls){
	WORD v;
	BYTE q;
	WORD p;

	q = p & 1;
	p = (((ccls * 3) >> 1) & 0x1ff);

	if(q){
	v = 
	(((WORD)(FAT0[p] >> 4) & 0x000f) |
	(((WORD)FAT0[p+1] << 4)  & 0x0ff0));
	} else {
	v = 
	(((WORD)FAT0[p+1] << 8) & 0x0f00) |
	((WORD)FAT0[p] & 0x00ff);
	}
	return v;
}

void fat_write(WORD ccls, WORD value){
	BYTE c;
	BYTE q;
	WORD p;

	MDD_IntFlash_SectorRead(
		2, (BYTE* )msd_buffer
	);
	q = p & 1;
	p = (((ccls * 3) >> 1) & 0x1ff);

	c = msd_buffer[p];
	if (q) {
		c = ((value & 0x000f) << 4) | ( c & 0x0f);
	}
	else {
		c = (value & 0x00ff);
	}
	msd_buffer[p] = c;

	c = msd_buffer[p + 1];
	if (q){
		c = (value >> 4);
	}
	else{
		c = ((value >> 8) & 0x000f) | (c & 0xf0);
	}
	msd_buffer[p + 1] = c;

	MDD_IntFlash_SectorWrite(
		2, (BYTE* )msd_buffer,
		1
	);
}

WORD fat_search_empty(void){
	WORD ccls;

	for(
	ccls = 2;
	ccls < MDD_INTERNAL_FLASH_DRIVE_CAPACITY + 2;
	ccls++
	){
		if(fat_read(ccls) == 0)
			return ccls;
	}
	return 0xffff;
}

BYTE file_open(const rom char far* fname){
	WORD i;

	if(file_is_open){
		return 0xff; // Too many files opened
	}

	file_is_open = 1;

	dir_entry = dir_load(fname);
	if(dir_entry == 0xff){
		dir_root.DIR_FstClusLO = 
			fat_search_empty();
			if(
			dir_root.DIR_FstClusLO == 0xffff
			){
			return 0xff; // Disk full
		}
		dir_entry = dir_search_empty();
		if(dir_entry == 0xff){
			return 0xff; // Disk full
		}
		memcpypgm2ram(
			&dir_root.DIR_Name[0], 
			fname, 
			11
		);
		dir_root.DIR_Attr = 0x20;
		dir_root.DIR_NTRes = 0;
		dir_root.DIR_FstClusHI = 0;
		dir_root.DIR_CrtTimeTenth = 0;
		dir_root.DIR_CrtTime = 0x7278;
		dir_root.DIR_CrtDate = 0x32b0;
		dir_root.DIR_LstAccDate = 0;
		dir_root.DIR_WrtTime = 0;
		dir_root.DIR_WrtDate = 0;
		dir_root.DIR_FileSize = 0;
		for(i = 0; i < 512; i++) msd_buffer[i] = 0;
	}
	else {
		dir_root.DIR_LstAccDate = 0x32b0;
		dir_root.DIR_WrtTime = 0x7279;
		dir_root.DIR_WrtDate = 0x32b0;
		MDD_IntFlash_SectorRead(
			dir_root.DIR_FstClusLO + 2, 
			(BYTE* )msd_buffer);
	}
	return 0;
}

void file_write(void){
	if(file_is_open == 0) return;
	MDD_IntFlash_SectorWrite(
		dir_root.DIR_FstClusLO + 2, 
		(BYTE* )msd_buffer, 
		1
	);
	dir_save(dir_entry);
	fat_write(
		dir_root.DIR_FstClusLO, 
		0x0fff
	);
}

void file_close(void){
	file_is_open = 0;
}

BYTE file_remove(const rom char far* fname){
	if(file_is_open)
		return 0xff; // File is in use
	dir_entry = dir_load(fname);
	if(dir_entry == 0xff) 
		return 0xff; // File not found
	fat_write(dir_root.DIR_FstClusLO, 0);
	dir_root.DIR_Name[0] = 0xe5;
	dir_root.DIR_FstClusLO = 0;
	dir_root.DIR_FileSize = 0;
	dir_save(dir_entry);
}