#ifndef __ISO9660_H__
#define __ISO9660_H__

#include "kernel.h"
#include "filesystem.h"

// LBA location of primary volume descriptor on a CD
#define PVD_LBA 0x10

/* ISO9660 structure. This maps an ISO9660 filesystem to a linked
 * list of VFS entries which can be used in the virtual filesystem.
 */
typedef struct
{
	u32int drivenumber;
	char* volume_name;
	u32int pathtable_lba;
	u32int rootextent_lba;
	u32int rootextent_len;
	FS_DirectoryEntry* root;
} iso9660;

/* Date structure as defined in the primary volume descriptor */
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

/* Per-file date entries (they differe from above to save space) */
typedef struct
{
	u8int years_since_1900;
	u8int month;
	u8int day;
	u8int hour;
	u8int minute;
	u8int second;
	char tz_offset;
} __attribute__((packed)) DIRECTORY_date;

/* Directory entry (may refer to a file or another directory) */
typedef struct
{
	u8int length;
	u8int attribute_length;
	u32int extent_lba_lsb;
	u32int extent_lba_msb;
	u32int data_length_lsb;
	u32int data_length_msb;
	DIRECTORY_date recording_date;
	u8int file_flags;
	u16int interleave_unit_size;
	u16int sequence_number_lsb;
	u16int sequence_number_msb;
	u8int filename_length;
	/* NOTE: Filenames may be longer than 12 characters,
	 * up to filename_length in size. This does not trample
	 * any following structs, because where this happens,
	 * we walk a list of entries by using the length field
	 * which accounts for long filenames.
	 */
	char filename[12];
} __attribute__((packed)) ISO9660_directory;

// Primary volume descriptor
typedef struct
{
	u8int typecode;
	char standardidentifier[5];
	u8int version;
	u8int unused;
	char systemidentifier[32];
	char volumeidentifier[32];
	char unused2[8];
	u32int lsb_volumespacesize;
	u32int msb_volumespacesize;
	char unused3[32];
	u16int lsb_volumesetsize;
	u16int msb_volumesetsize;
	u16int lsb_volumeseqno;
	u16int msb_volumeseqno;
	u16int lsb_blocksize;
	u16int msb_blocksize;
	/* OK, whoever thought it was a good idea to have
	 * dual-endianness copies of every value larger than
	 * one byte in the structure needs a kicking.
	 */
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
	u8int file_structure_version;
	char unused4;
	u8int application_use[512];
	u8int reserved[653];
} __attribute__((packed)) PVD;

/* Mount an ISO 9660 filesystem on a given drive number (drive number from enumeration in ata.h)
 * Returns either NULL or an iso9660* which references the volume information and initially the
 * root directory of the dis.
 */
iso9660* iso_mount_volume(u32int drivenumber);

int iso_read_file(void* info, const char* filename, u32int start, u32int length, unsigned char* buffer);

void init_iso9660();
void iso9660_attach(u32int drivenumber, const char* path);

#endif

