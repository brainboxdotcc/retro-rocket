#ifndef __FAT32_H__
#define __FAT32_H__

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
	u8 bootable;
	u8 starthead;
	u16 startcylsect;
	u8 systemid;
	u8 endhead;
	u16 endcylsect;
	u32 startlba;
	u32 length;
} __attribute__((packed)) Partition;

typedef struct
{
	Partition p_entry[4];
} __attribute__((packed)) PartitionTable;

typedef struct
{
	u32 signature1;
	char reserved1[480];
	u32 structsig;
	u32 freecount;
	u32 nextfree;
	char reserved2[12];
	u32 trailsig;
} __attribute__((packed)) FSInfo;

typedef struct
{
	u32 drivenumber;
	u8 partitionid;
	char* volume_name;
	u32 start;
	u32 length;
	u32 rootdircluster;
	u16 reservedsectors;
	u32 fsinfocluster;
	u8 numberoffats;
	u32 fatsize;
	u32 clustersize;
	u32* fat;
	FS_DirectoryEntry* root;
	FSInfo* info;
} fat32;

typedef struct
{
	char name[11];
	u8 attr;
	u8 nt;
	u8 create_time_tenths;
	u16 create_time;
	u16 create_date;
	u16 access_date;
	u16 first_cluster_hi;
	u16 write_time;
	u16 write_date;
	u16 first_cluster_lo;
	u32 size;
} __attribute__((packed)) DirectoryEntry;

typedef struct
{
	u8 code1;
	u8 code2;
	u8 code3;
	char oemidentifier[8];
	u16 bytespersector;
	u8 sectorspercluster;
	u16 reservedsectors;
	u8 numberoffats;
	u16 numberofdirentries;
	u16 totalsectors;
	u8 mediatype;
	u16 unusedsectorsperfat;
	u16 sectorspertrack;
	u16 numberofheads;
	u32 hiddensectors;
	u32 sectorsonmedia;
	// Extended Boot Record	
	u32 sectorsperfat;
	u16 flags;
	u16 fatversion;
	u32 rootdircluster;
	u16 fsinfocluster;
	u16 backupbootsectorcluster;
	char reserved[12];
	u8 drivenumber;
	u8 ntflags;
	u8 signature; 
	u32 serialnumber;
	char volumelabel[11];
	char systemid[9];
} __attribute__((packed)) ParameterBlock;

fat32* fat32_mount_volume(u32 drivenumber);
int fat32_read_file(void* file, u32 start, u32 length, unsigned char* buffer);
void* iso_get_directory(void* t);
void init_fat32();
void fat32_attach(u32 drivenumber, const char* path);
int find_first_harddisk();

#endif

