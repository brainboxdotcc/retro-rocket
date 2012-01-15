#ifndef __ISO9660_H__
#define __ISO9660_H__

#include "kernel.h"
#include "filesystem.h"

// LBA location of primary volume descriptor on a CD
#define PARTITION_TABLE_OFFSET 0x1BE

/* ISO9660 structure. This maps an ISO9660 filesystem to a linked
 * list of VFS entries which can be used in the virtual filesystem.
 */

typedef struct
{
	u8int bootable;
	u8int starthead;
	u16int startcylsect;
	u8int systemid;
	u8int endhead;
	u16int endcylsect;
	u32int startlba;
	u32int length;
} __attribute__((packed)) Partition;

typedef struct
{
	Partition p_entry[4];
} __attribute__((packed)) PartitionTable;

typedef struct
{
	u32int drivenumber;
	u8int partitionid;
	char* volume_name;
	u32int start;
	u32int length;
	FS_DirectoryEntry* root;
} fat32;

typedef struct
{
	u8int code1;
	u8int code2;
	u8int code3;
	char oemidentifier[8];
	u16int bytespersector;
	u8int sectorspercluster;
	u16int reservedsectors;
	u8int numberoffats;
	u16int numberofdirentries;
	u16int totalsectors;
	u8int mediatype;
	u16int unusedsectorsperfat;
	u16int sectorspertrack;
	u16int numberofheads;
	u32int hiddensectors;
	u32int sectorsonmedia;
	// Extended Boot Record	
	u32int sectorsperfat;
	u16int flags;
	u16int fatversion;
	u32int rootdircluster;
	u16int fsinfocluster;
	u16int backupbootsectorcluster;
	char reserved[12];
	u8int drivenumber;
	u8int ntflags;
	u8int signature; 
	u32int serialnumber;
	char volumelabel[11];
	char systemid[9];
} __attribute__((packed)) ParameterBlock;

fat32* fat32_mount_volume(u32int drivenumber);

int fat32_read_file(void* file, u32int start, u32int length, unsigned char* buffer);
void* iso_get_directory(void* t);
void init_fat32();
void fat32_attach(u32int drivenumber, const char* path);
int find_first_harddisk();

#endif

