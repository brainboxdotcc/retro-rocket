#pragma once

#define NVME_CLASS_CODE 0x0108 /* Mass Storage / NVM Express */

typedef volatile struct nvme_regs_t {
	uint64_t cap;         /* 0x00: CAP */
	uint32_t vs;          /* 0x08: VS */
	uint32_t intms;   /* 0x0C */
	uint32_t intmc;   /* 0x10 */
	uint32_t cc;          /* 0x14 */
	uint32_t rsvd1;   /* 0x18 */
	uint32_t csts;        /* 0x1C */
	uint32_t nssr;        /* 0x20 */
	uint32_t aqa;         /* 0x24 */
	uint64_t asq;         /* 0x28 */
	uint64_t acq;         /* 0x30 */
	/* 0x38.. doorbells at 0x1000 */
} nvme_regs_t;

typedef struct nvme_sqe_t {
	uint8_t opc;
	uint8_t fuse;
	uint16_t cid;
	uint32_t nsid;
	uint64_t rsvd2;
	uint64_t mptr;
	uint64_t prp1;
	uint64_t prp2;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
} __attribute__((packed)) nvme_sqe_t;

typedef struct nvme_cqe_t {
	uint32_t dw0;
	uint32_t dw1;
	uint16_t sq_head;
	uint16_t sq_id;
	uint16_t cid;
	uint16_t status;  /* bit15: phase; bit0..7: status code */
} __attribute__((packed)) nvme_cqe_t;

/* ---------- Opcodes / Identify ---------- */
#define NVME_ADMIN_CREATE_IO_CQ  0x05
#define NVME_ADMIN_CREATE_IO_SQ  0x01
#define NVME_ADMIN_IDENTIFY          0x06

#define NVME_IO_READ                         0x02
#define NVME_IO_WRITE                        0x01
#define NVME_IO_DSM                          0x09

#define NVME_ID_CNS_CTRL                 0x01
#define NVME_ID_CNS_NS                   0x00
#define NVME_ID_CNS_NS_ACTIVE        0x02

typedef struct nvme_dsm_range_t {
	uint32_t cattr;   /* bit2 = deallocate */
	uint32_t nlba;        /* number of native LBAs */
	uint64_t slba;        /* starting native LBA */
} __attribute__((packed)) nvme_dsm_range_t;

typedef struct nvme_dev_t {
	volatile nvme_regs_t *regs;
	uint32_t dstrd;

	/* admin queue (QD=16) */
	nvme_sqe_t *asq;
	nvme_cqe_t *acq;
	uint16_t a_qd;
	uint16_t a_sqt;
	uint16_t a_cqh;
	uint8_t a_phase;

	/* io queue (QD=1) */
	nvme_sqe_t *iosq;
	nvme_cqe_t *iocq;
	uint16_t io_qd;
	uint16_t io_sqt;
	uint16_t io_cqh;
	uint8_t io_phase;

	uint32_t nsid;
	bool ns_is_4k;
	uint64_t size_in_512;

	char model[41];
	bool has_trim; /* dataset management supported */
} nvme_dev_t;

void init_nvme(void);

