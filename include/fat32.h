#ifndef __FAT32_H__
#define __FAT32_H__

#define FAT32_SIGNATURE		0x41615252

#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_LONG_NAME		0x0F

typedef struct fat32_fs_info_t {
	uint32_t signature1;
	char reserved1[480];
	uint32_t structsig;
	uint32_t freecount;
	uint32_t nextfree;
	char reserved2[12];
	uint32_t trailsig;
} __attribute__((packed)) fat32_fs_info_t;

typedef struct fat32_t {
	char device_name[16];
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
	fs_directory_entry_t* root;
	fat32_fs_info_t* info;
} fat32_t;

typedef struct directory_entry_t {
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
} __attribute__((packed)) directory_entry_t;

typedef struct parameter_block_t {
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
} __attribute__((packed)) parameter_block_t;

typedef struct lfn_t {
	uint8_t order;
	uint16_t first[5];
	uint8_t attributes; // Always 0x0f
	uint8_t entry_type; // always zero for name entries
	uint8_t checksum;
	uint16_t second[6];
	uint16_t reserved;  // always 0
	uint16_t third[2];
} __attribute__((packed)) lfn_t;

void init_fat32();

#endif

