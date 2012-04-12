#ifndef __ISO9660_H__
#define __ISO9660_H__

// LBA location of primary volume descriptor on a CD
#define PVD_LBA 0x10

/* ISO9660 structure. This maps an ISO9660 filesystem to a linked
 * list of VFS entries which can be used in the virtual filesystem.
 */
typedef struct
{
	u32 drivenumber;
	int joliet;
	char* volume_name;
	u32 pathtable_lba;
	u32 rootextent_lba;
	u32 rootextent_len;
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
	u8 years_since_1900;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
	char tz_offset;
} __attribute__((packed)) DIRECTORY_date;

/* Directory entry (may refer to a file or another directory) */
typedef struct
{
	u8 length;
	u8 attribute_length;
	u32 extent_lba_lsb;
	u32 extent_lba_msb;
	u32 data_length_lsb;
	u32 data_length_msb;
	DIRECTORY_date recording_date;
	u8 file_flags;
	u16 interleave_unit_size;
	u16 sequence_number_lsb;
	u16 sequence_number_msb;
	u8 filename_length;
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
	u8 typecode;
	char standardidentifier[5];
	u8 version;
	u8 unused;
	char systemidentifier[32];
	char volumeidentifier[32];
	char unused2[8];
	u32 lsb_volumespacesize;
	u32 msb_volumespacesize;
	//char unused3[32];
	char escape_seq[8];
	char unused3[32-8];
	u16 lsb_volumesetsize;
	u16 msb_volumesetsize;
	u16 lsb_volumeseqno;
	u16 msb_volumeseqno;
	u16 lsb_blocksize;
	u16 msb_blocksize;
	/* OK, whoever thought it was a good idea to have
	 * dual-endianness copies of every value larger than
	 * one byte in the structure needs a kicking.
	 */
	u32 lsb_pathtablesize;
	u32 msb_pathtablesize;
	u32 lsb_pathtable_L_lba;
	u32 lsb_optpathtable_L_lba;
	u32 lsb_pathtable_M_lba;
	u32 lsb_optpathtable_M_lba;
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
	u8 file_structure_version;
	char unused4;
	u8 application_use[512];
	u8 reserved[653];
} __attribute__((packed)) PVD;

typedef struct
{
} __attribute__((packed)) SVD;

/* Mount an ISO 9660 filesystem on a given drive number (drive number from enumeration in ata.h)
 * Returns either NULL or an iso9660* which references the volume information and initially the
 * root directory of the dis.
 */
iso9660* iso_mount_volume(u32 drivenumber);

int iso_read_file(void* file, u32 start, u32 length, unsigned char* buffer);

void init_iso9660();
void iso9660_attach(u32 drivenumber, const char* path);
int find_first_cdrom();

#endif

