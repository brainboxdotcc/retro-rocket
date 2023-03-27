#ifndef __ISO9660_H__
#define __ISO9660_H__

// LBA location of primary volume descriptor on a CD
#define PVD_LBA 0x10

/* ISO9660 structure. This maps an ISO9660 filesystem to a linked
 * list of VFS entries which can be used in the virtual filesystem.
 */
typedef struct
{
	int joliet;
	char* volume_name;
	uint32_t pathtable_lba;
	uint32_t rootextent_lba;
	uint32_t rootextent_len;
	fs_directory_entry_t* root;
	storage_device_t* device;
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
	uint8_t years_since_1900;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	char tz_offset;
} __attribute__((packed)) DIRECTORY_date;

/* Directory entry (may refer to a file or another directory) */
typedef struct
{
	uint8_t length;
	uint8_t attribute_length;
	uint32_t extent_lba_lsb;
	uint32_t extent_lba_msb;
	uint32_t data_length_lsb;
	uint32_t data_length_msb;
	DIRECTORY_date recording_date;
	uint8_t file_flags;
	uint16_t interleave_unit_size;
	uint16_t sequence_number_lsb;
	uint16_t sequence_number_msb;
	uint8_t filename_length;
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
	uint8_t typecode;
	char standardidentifier[5];
	uint8_t version;
	uint8_t unused;
	char systemidentifier[32];
	char volumeidentifier[32];
	char unused2[8];
	uint32_t lsb_volumespacesize;
	uint32_t msb_volumespacesize;
	//char unused3[32];
	char escape_seq[8];
	char unused3[32-8];
	uint16_t lsb_volumesetsize;
	uint16_t msb_volumesetsize;
	uint16_t lsb_volumeseqno;
	uint16_t msb_volumeseqno;
	uint16_t lsb_blocksize;
	uint16_t msb_blocksize;
	/* OK, whoever thought it was a good idea to have
	 * dual-endianness copies of every value larger than
	 * one byte in the structure needs a kicking.
	 */
	uint32_t lsb_pathtablesize;
	uint32_t msb_pathtablesize;
	uint32_t lsb_pathtable_L_lba;
	uint32_t lsb_optpathtable_L_lba;
	uint32_t lsb_pathtable_M_lba;
	uint32_t lsb_optpathtable_M_lba;
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
	uint8_t file_structure_version;
	char unused4;
	uint8_t application_use[512];
	uint8_t reserved[653];
} __attribute__((packed)) PVD;

typedef struct
{
} __attribute__((packed)) SVD;

void init_iso9660();

#endif

