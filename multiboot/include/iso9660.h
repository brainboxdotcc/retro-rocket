#ifndef __ISO9660_H__
#define __ISO9660_H__

// Location of primary volume descriptor
#define PVD_LBA 0x10

typedef struct
{
	u32int drivenumber;
	char* volume_name;
	long pathtable_lba;
	long rootdir_lba;
	long csd_lba;
} iso9660;

// Primary volume descriptor
typedef struct
{
	unsigned char typecode;
	char standardidentifier[5];
	unsigned char version;
	unsigned char unused;
} PVD __attribute__((packed));

iso9660* mount_volume(u32int drivenumber);

#endif
