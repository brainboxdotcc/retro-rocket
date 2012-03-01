#ifndef __FAT32_H__
#define __FAT32_H__

#include "kernel.h"
#include "filesystem.h"

// Offset of partition table in MBR
#define PARTITION_TABLE_OFFSET	0x1BE

#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_LONG_NAME		0x0F

struct FSInfo;

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
	u32int signature1;
	char reserved1[480];
	u32int structsig;
	u32int freecount;
	u32int nextfree;
	char reserved2[12];
	u32int trailsig;
} __attribute__((packed)) FSInfo;

typedef struct
{
	u32int drivenumber;
	u8int partitionid;
	char* volume_name;
	u32int start;
	u32int length;
	u32int rootdircluster;
	u16int reservedsectors;
	u16int fsinfocluster;
	u8int numberoffats;
	u32int fatsize;
	u32int clustersize;
	u32int* fat;
	FS_DirectoryEntry* root;
	FSInfo* info;
} fat32;

typedef struct
{
	char name[11];
	u8int attr;
	u8int nt;
	u8int create_time_tenths;
	u16int create_time;
	u16int create_date;
	u16int access_date;
	u16int first_cluster_hi;
	u16int write_time;
	u16int write_date;
	u16int first_cluster_lo;
	u32int size;
} __attribute__((packed)) DirectoryEntry;

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

