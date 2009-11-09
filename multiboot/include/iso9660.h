#ifndef __ISO9660_H__
#define __ISO9660_H__

#include "kernel.h"
#include "filesystem.h"

// Location of primary volume descriptor
#define PVD_LBA 0x10

typedef struct
{
	u32int drivenumber;
	char* volume_name;
	u32int pathtable_lba;
	u32int rootextent_lba;
	FS_DirectoryEntry* root;
} iso9660;

typedef struct
{
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char millisecond[2];
	char tz_offset;
} __attribute__((packed)) PVD_date;

typedef struct
{
	unsigned char years_since_1900;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	char tz_offset;
} __attribute__((packed)) DIRECTORY_date;

typedef struct
{
	unsigned char length;
	unsigned char attribute_length;
	u32int extent_lba_lsb;
	u32int extent_lba_msb;
	u32int data_length_lsb;
	u32int data_length_msb;
	DIRECTORY_date recording_date;
	unsigned char file_flags;
	unsigned short interleave_unit_size;
	unsigned short sequence_number_lsb;
	unsigned short sequence_number_msb;
	//char sequence[6];
	unsigned char filename_length;
	char filename[12];
} __attribute__((packed)) ISO9660_directory;

// Primary volume descriptor
typedef struct
{
	unsigned char typecode;
	char standardidentifier[5];
	unsigned char version;
	unsigned char unused;
	char systemidentifier[32];
	char volumeidentifier[32];
	char unused2[8];
	u32int lsb_volumespacesize;
	u32int msb_volumespacesize;
	char unused3[32];
	unsigned short int lsb_volumesetsize;
	unsigned short int msb_volumesetsize;
	unsigned short int lsb_volumeseqno;
	unsigned short int msb_volumeseqno;
	unsigned short int lsb_blocksize;
	unsigned short int msb_blocksize;
	u32int lsb_pathtablesize;
	u32int msb_pathtablesize;
	u32int lsb_pathtable_L_lba;
	u32int lsb_optpathtable_L_lba;
	u32int lsb_pathtable_M_lba;
	u32int lsb_optpathtable_M_lba;
	ISO9660_directory root_directory;
	char volume_set_id[128];
	char publisher_id[128];
	char data_preparer[128];
	char application_id[128];
	char copyright_file[38];
	char abstract_file[36];
	char bibliographic_file[37];
	PVD_date volume_creation_date;
	PVD_date volume_modification_date;
	PVD_date volume_expire_date;
	PVD_date volume_effective_date;
	unsigned char file_structure_version;
	char unused4;
	unsigned char application_use[512];
	unsigned char reserved[653];
} __attribute__((packed)) PVD;

iso9660* mount_volume(u32int drivenumber);

#endif
