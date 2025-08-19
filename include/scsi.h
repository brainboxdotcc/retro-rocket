#pragma once
#include <stdint.h>

/* Common SCSI CDB opcodes */
#define SCSI_OP_TEST_UNIT_READY       0x00
#define SCSI_OP_REQUEST_SENSE_6       0x03
#define SCSI_OP_INQUIRY_6             0x12
#define SCSI_OP_START_STOP_UNIT_6     0x1B
#define SCSI_OP_PREVENT_ALLOW_6       0x1E
#define SCSI_OP_READ_CAPACITY_10      0x25
#define SCSI_OP_READ_10               0x28
#define SCSI_OP_WRITE_10              0x2A
#define SCSI_OP_SYNCHRONIZE_CACHE_10  0x35
#define SCSI_OP_MODE_SENSE_10         0x5A
#define SCSI_OP_READ_12               0xA8
#define SCSI_OP_WRITE_12              0xAA
#define SCSI_OP_READ_16               0x88
#define SCSI_OP_WRITE_16              0x8A
#define SCSI_OP_SERVICE_ACTION_IN_16  0x9E  /* use SA=0x10 for READ CAPACITY(16) */

/* INQUIRY(6) — 6 bytes */
typedef struct __attribute__((packed)) scsi_cdb_inquiry6 {
	uint8_t  opcode;       /* 0x12 */
	uint8_t  evpd;         /* bit0 = EVPD; you use 0 */
	uint8_t  page_code;    /* usually 0 */
	uint8_t  reserved;     /* 0 */
	uint8_t  alloc_len;    /* 1-byte allocation length */
	uint8_t  control;      /* 0 */
} scsi_cdb_inquiry6;

/* READ(12) — 12 bytes; LBA and transfer length are big-endian */
typedef struct __attribute__((packed)) scsi_cdb_read12 {
	uint8_t  opcode;       /* 0xA8 */
	uint8_t  flags;        /* DPO/FUA/etc; you use 0 */
	uint32_t lba_be;       /* big-endian */
	uint32_t xfer_be;      /* big-endian (blocks) */
	uint8_t  group;        /* 0 */
	uint8_t  control;      /* 0 */
} scsi_cdb_read12;

/* TEST UNIT READY (6) — opcode 0x00 */
typedef struct __attribute__((packed)) scsi_cdb_test_unit_ready6 {
	uint8_t opcode;      /* 0x00 */
	uint8_t rsv1;
	uint8_t rsv2;
	uint8_t rsv3;
	uint8_t rsv4;
	uint8_t control;
} scsi_cdb_test_unit_ready6;

/* REQUEST SENSE (6) — opcode 0x03 */
typedef struct __attribute__((packed)) scsi_cdb_request_sense6 {
	uint8_t opcode;      /* 0x03 */
	uint8_t desc;        /* bit0 = DESC; usually 0 */
	uint8_t rsv1;
	uint8_t rsv2;
	uint8_t alloc_len;   /* <= 255 */
	uint8_t control;
} scsi_cdb_request_sense6;

/* START STOP UNIT (6) — opcode 0x1B */
typedef struct __attribute__((packed)) scsi_cdb_start_stop_unit6 {
	uint8_t opcode;      /* 0x1B */
	uint8_t immed;       /* bit0 IMMED if you need it */
	uint8_t rsv1;
	uint8_t rsv2;
	uint8_t power;       /* bits: LOEJ/START/POWER-COND */
	uint8_t control;
} scsi_cdb_start_stop_unit6;

/* PREVENT/ALLOW MEDIUM REMOVAL (6) — opcode 0x1E */
typedef struct __attribute__((packed)) scsi_cdb_prevent_allow6 {
	uint8_t opcode;      /* 0x1E */
	uint8_t rsv1;
	uint8_t rsv2;
	uint8_t rsv3;
	uint8_t prevent;     /* bit0: 1=prevent, 0=allow */
	uint8_t control;
} scsi_cdb_prevent_allow6;

/* SYNCHRONIZE CACHE (10) — opcode 0x35 */
typedef struct __attribute__((packed)) scsi_cdb_synchronize_cache10 {
	uint8_t  opcode;     /* 0x35 */
	uint8_t  flags;      /* e.g. SYNC NV; usually 0 */
	uint32_t lba_be;     /* big-endian start LBA (or 0) */
	uint8_t  group;      /* 0 */
	uint16_t len_be;     /* big-endian blocks (0 => all) */
	uint8_t  control;
} scsi_cdb_synchronize_cache10;

/* MODE SENSE (10) — opcode 0x5A */
typedef struct __attribute__((packed)) scsi_cdb_mode_sense10 {
	uint8_t  opcode;         /* 0x5A */
	uint8_t  dbd;            /* bit3 DBD; often set to 1 */
	uint8_t  page_code;      /* PC(2)/PageCode(6) literal */
	uint8_t  subpage;        /* 0 unless using subpages */
	uint8_t  rsv1;
	uint8_t  rsv2;
	uint16_t alloc_len_be;   /* big-endian allocation length */
	uint8_t  control;
} scsi_cdb_mode_sense10;

/* READ(16)/WRITE(16) — opcodes 0x88 / 0x8A */
typedef struct __attribute__((packed)) scsi_cdb_rw16 {
	uint8_t  opcode;     /* 0x88 READ(16) or 0x8A WRITE(16) */
	uint8_t  flags;      /* DPO/FUA/etc; 0 if unused */
	uint64_t lba_be;     /* big-endian */
	uint32_t xfer_be;    /* big-endian blocks */
	uint8_t  group;      /* 0 */
	uint8_t  control;    /* 0 */
} scsi_cdb_rw16;

/* READ CAPACITY (16) via SERVICE ACTION IN(16) — opcode 0x9E, service action 0x10 */
typedef struct __attribute__((packed)) scsi_cdb_read_capacity16 {
	uint8_t  opcode;        /* 0x9E */
	uint8_t  service;       /* 0x10 (READ CAPACITY(16)) */
	uint64_t lba_be;        /* usually 0 */
	uint32_t alloc_len_be;  /* response alloc length */
	uint8_t  pmi;           /* PMI & misc; usually 0 */
	uint8_t  control;       /* 0 */
} scsi_cdb_read_capacity16;
