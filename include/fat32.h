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
	uint8_t bootable;
	uint8_t starthead;
	uint16_t startcylsect;
	uint8_t systemid;
	uint8_t endhead;
	uint16_t endcylsect;
	uint32_t startlba;
	uint32_t length;
} __attribute__((packed)) Partition;

typedef struct
{
	Partition p_entry[4];
} __attribute__((packed)) PartitionTable;

typedef struct
{
	uint32_t signature1;
	char reserved1[480];
	uint32_t structsig;
	uint32_t freecount;
	uint32_t nextfree;
	char reserved2[12];
	uint32_t trailsig;
} __attribute__((packed)) FSInfo;

typedef struct
{
	uint32_t drivenumber;
	uint8_t partitionid;
	char* volume_name;
	uint32_t start;
	uint32_t length;
	uint32_t rootdircluster;
	uint16_t reservedsectors;
	uint32_t fsinfocluster;
	uint8_t numberoffats;
	uint32_t fatsize;
	uint32_t clustersize;
	uint32_t* fat;
	FS_DirectoryEntry* root;
	FSInfo* info;
} fat32;

typedef struct
{
	char name[11];
	uint8_t attr;
	uint8_t nt;
	uint8_t create_time_tenths;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t first_cluster_hi;
	uint16_t write_time;
	uint16_t write_date;
	uint16_t first_cluster_lo;
	uint32_t size;
} __attribute__((packed)) DirectoryEntry;

typedef struct
{
	uint8_t code1;
	uint8_t code2;
	uint8_t code3;
	char oemidentifier[8];
	uint16_t bytespersector;
	uint8_t sectorspercluster;
	uint16_t reservedsectors;
	uint8_t numberoffats;
	uint16_t numberofdirentries;
	uint16_t totalsectors;
	uint8_t mediatype;
	uint16_t unusedsectorsperfat;
	uint16_t sectorspertrack;
	uint16_t numberofheads;
	uint32_t hiddensectors;
	uint32_t sectorsonmedia;
	// Extended Boot Record	
	uint32_t sectorsperfat;
	uint16_t flags;
	uint16_t fatversion;
	uint32_t rootdircluster;
	uint16_t fsinfocluster;
	uint16_t backupbootsectorcluster;
	char reserved[12];
	uint8_t drivenumber;
	uint8_t ntflags;
	uint8_t signature; 
	uint32_t serialnumber;
	char volumelabel[11];
	char systemid[9];
} __attribute__((packed)) ParameterBlock;

fat32* fat32_mount_volume(uint32_t drivenumber);
int fat32_read_file(void* file, uint32_t start, uint32_t length, unsigned char* buffer);
void* iso_get_directory(void* t);
void init_fat32();
void fat32_attach(uint32_t drivenumber, const char* path);
int find_first_harddisk();

#endif

