#include <kernel.h>

static inline volatile uint32_t *nvme_db_ptr(volatile nvme_regs_t *r, uint32_t dstrd, uint16_t qid, bool cq) {
	uintptr_t base = (uintptr_t) r + (uintptr_t) 0x1000;
	uint32_t stride = (uint32_t) 4 << dstrd;
	uint32_t off = (uint32_t) ((qid * 2 + (cq ? 1 : 0)) * stride);
	return (volatile uint32_t *) (base + off);
}

/* ---------- Tiny 4 KiB page bounce (PRP1) ---------- */
static inline void *nvme_page_alloc_4k(void) {
	return kmalloc_aligned(4096, 4096);
}

static void nvme_page_free_4k(void *ptr) {
	kfree(ptr);
}

/* ---------- Helpers ---------- */
static inline void nvme_db_write(volatile nvme_regs_t *r, uint32_t dstrd, uint16_t qid, bool cq, uint16_t val) {
	volatile uint32_t *db = nvme_db_ptr(r, dstrd, qid, cq);
	*db = (uint32_t) val;
}

static nvme_sqe_t *nvme_sqe_alloc(nvme_dev_t *dev, bool admin, uint16_t *cid_out) {
	if (admin) {
		uint16_t tail = dev->a_sqt;
		*cid_out = tail;
		memset(&dev->asq[tail], 0, sizeof(nvme_sqe_t));
		dev->a_sqt = (uint16_t) ((tail + 1) & (dev->a_qd - 1));
		return &dev->asq[tail];
	} else {
		uint16_t tail = dev->io_sqt;
		*cid_out = tail;
		memset(&dev->iosq[tail], 0, sizeof(nvme_sqe_t));
		dev->io_sqt = (uint16_t) ((tail + 1) & (dev->io_qd - 1));
		return &dev->iosq[tail];
	}
}

static int nvme_cqe_poll(nvme_dev_t *dev, bool admin, uint16_t want_cid) {
	if (admin) {
		/* Ensure SQE stores are visible, then submit by ringing the tail */
		__asm__ volatile ("sfence":: : "memory");
		nvme_db_write(dev->regs, dev->dstrd, 0, false, dev->a_sqt);

		for (;;) {
			nvme_cqe_t *e = &dev->acq[dev->a_cqh];
			uint16_t st = e->status;
			uint8_t ph = (uint8_t) (st & 1); /* phase = bit 0 */
			if (ph != dev->a_phase) {
				__asm__ volatile("pause");
				continue;
			}

			dev->a_cqh = (uint16_t) ((dev->a_cqh + 1) & (dev->a_qd - 1));
			nvme_db_write(dev->regs, dev->dstrd, 0, true, dev->a_cqh);
			if (dev->a_cqh == 0) {
				dev->a_phase ^= 1;
			}
			uint8_t sc = (uint8_t) ((st >> 1) & 0xFF);
			uint8_t sct = (uint8_t) ((st >> 9) & 0x7);
			uint8_t dnr = (uint8_t) ((st >> 12) & 1);
			uint8_t m = (uint8_t) ((st >> 11) & 1);
			if (sc != 0 || sct != 0) {
				dprintf("NVMe %s error: SCT=%u SC=%u M=%u DNR=%u\n", admin ? "admin" : "io", sct, sc, m, dnr);
				return 0;
			}

			/* admin is sync; CID mismatch ignored */
			return 1;
		}
	} else {
		__asm__ volatile ("sfence":: : "memory");
		nvme_db_write(dev->regs, dev->dstrd, 1, false, dev->io_sqt);

		for (;;) {
			nvme_cqe_t *e = &dev->iocq[dev->io_cqh];
			uint16_t st = e->status;
			uint8_t ph = (uint8_t) (st & 1);
			if (ph != dev->io_phase) {
				__asm__ volatile("pause");
				continue;
			}

			dev->io_cqh = (uint16_t) ((dev->io_cqh + 1) & (dev->io_qd - 1));
			nvme_db_write(dev->regs, dev->dstrd, 1, true, dev->io_cqh);
			if (dev->io_cqh == 0) {
				dev->io_phase ^= 1;
			}

			uint8_t sc = (uint8_t) ((st >> 1) & 0xFF);
			uint8_t sct = (uint8_t) ((st >> 9) & 0x7);
			uint8_t dnr = (uint8_t) ((st >> 12) & 1);
			if (sc != 0 || sct != 0) {
				dprintf("NVMe io error: SCT=%u SC=%u DNR=%u\n", sct, sc, dnr);
				return 0;
			}

			return 1;
		}
	}
}

/* ---------- Admin / IO commands ---------- */
static int nvme_admin_identify(nvme_dev_t *dev, uint32_t cns, uint32_t ns, void *buf4k) {
	uint16_t cid;
	dprintf("admin identify sqe alloc\n");
	nvme_sqe_t *sqe = nvme_sqe_alloc(dev, true, &cid);
	sqe->opc = NVME_ADMIN_IDENTIFY;
	sqe->cid = cid;
	sqe->nsid = ns;
	sqe->prp1 = (uint64_t) (uintptr_t) buf4k;
	sqe->cdw10 = cns;
	dprintf("admin identify cqe poll\n");
	return nvme_cqe_poll(dev, true, cid);
}

static int nvme_admin_create_cq(nvme_dev_t *dev, uint16_t qid, nvme_cqe_t *cq, uint16_t qd) {
	uint64_t cap = dev->regs->cap;
	uint16_t mqes = (uint16_t) (cap & 0xFFFF);
	uint16_t max_q = (uint16_t) (mqes + 1);

	if (qd < 2) qd = 2;
	if (qd > max_q) qd = max_q;

	uint16_t cid;
	nvme_sqe_t *sqe = nvme_sqe_alloc(dev, true, &cid);
	sqe->opc = NVME_ADMIN_CREATE_IO_CQ;
	sqe->cid = cid;
	sqe->prp1 = (uint64_t) (uintptr_t) cq;

	/* Match ASM: CDW10 = (QSIZE-1)<<16 | QID */
	sqe->cdw10 = ((uint32_t) (qd - 1) << 16) | (uint32_t) qid;

	/* CDW11: PC=1 (bit0), IEN=0, IV=0 */
	sqe->cdw11 = 1u;

	dprintf("CreateCQ: qid=%u qd=%u cdw10=0x%08x cdw11=0x%08x PRP1=0x%016lx\n",
		qid, qd, sqe->cdw10, sqe->cdw11, sqe->prp1);

	return nvme_cqe_poll(dev, true, cid);
}

static int nvme_admin_create_sq(nvme_dev_t *dev, uint16_t qid, nvme_sqe_t *sq, uint16_t qd) {
	uint64_t cap = dev->regs->cap;
	uint16_t mqes = (uint16_t) (cap & 0xFFFF);
	uint16_t max_q = (uint16_t) (mqes + 1);

	if (qd < 2) qd = 2;
	if (qd > max_q) qd = max_q;

	uint16_t cid;
	nvme_sqe_t *sqe = nvme_sqe_alloc(dev, true, &cid);
	sqe->opc = NVME_ADMIN_CREATE_IO_SQ;
	sqe->cid = cid;
	sqe->prp1 = (uint64_t) (uintptr_t) sq;

	/* Match ASM: CDW10 = (QSIZE-1)<<16 | QID */
	sqe->cdw10 = ((uint32_t) (qd - 1) << 16) | (uint32_t) qid;

	/* CDW11: CQID=1 (bits 31:16), PC=1 (bit 0) */
	sqe->cdw11 = ((uint32_t) 1 << 16) | 1u;

	dprintf("CreateSQ: qid=%u qd=%u cdw10=0x%08x cdw11=0x%08x PRP1=0x%016lx\n",
		qid, qd, sqe->cdw10, sqe->cdw11, sqe->prp1);

	return nvme_cqe_poll(dev, true, cid);
}

static int nvme_io_rw(nvme_dev_t *dev, bool write, uint64_t slba, uint16_t nblocks, void *buf4k) {
	uint16_t cid;
	nvme_sqe_t *sqe = nvme_sqe_alloc(dev, false, &cid);
	sqe->opc = write ? NVME_IO_WRITE : NVME_IO_READ;
	sqe->cid = cid;
	sqe->nsid = dev->nsid;
	sqe->prp1 = (uint64_t) (uintptr_t) buf4k;
	sqe->cdw10 = (uint32_t) (slba & 0xFFFFFFFF);
	sqe->cdw11 = (uint32_t) (slba >> 32);
	sqe->cdw12 = (uint32_t) (nblocks - 1);
	return nvme_cqe_poll(dev, false, cid);
}

static int nvme_io_trim(nvme_dev_t *dev, uint64_t slba_native, uint32_t nlba_native) {
	nvme_dsm_range_t *rng = nvme_page_alloc_4k();
	if (!rng) {
		return 0;
	}
	memset(rng, 0, 4096);
	rng[0].cattr = (1u << 2); /* deallocate */
	rng[0].nlba = nlba_native;
	rng[0].slba = slba_native;

	uint16_t cid;
	nvme_sqe_t *sqe = nvme_sqe_alloc(dev, false, &cid);
	sqe->opc = NVME_IO_DSM;
	sqe->cid = cid;
	sqe->nsid = dev->nsid;
	sqe->prp1 = (uint64_t) (uintptr_t) rng;
	sqe->cdw10 = 1; /* one range */

	int ok = nvme_cqe_poll(dev, false, cid);
	nvme_page_free_4k(rng);
	return ok;
}

static int nvme_hw_enable(nvme_dev_t *dev) {
	volatile nvme_regs_t *r = dev->regs;
	uint64_t cap = r->cap;

	/* DSTRD & MQES just for logs */
	dev->dstrd = (uint32_t) ((cap >> 32) & 0xF);
	uint16_t mqes = (uint16_t) (cap & 0xFFFF);
	uint16_t max_q = (uint16_t) (mqes + 1);
	dprintf("NVMe CAP: DSTRD=%u MQES=%u (max_q=%u)\n",
		(unsigned) (dev->dstrd & 0xF), mqes, max_q);

	/* Disable if enabled */
	if (r->cc & 1u) {
		r->cc &= ~1u;
		while (r->csts & 1u) __asm__ volatile("pause");
	}

	/* Admin queues: 64 entries like the ASM driver (clamp to MQES+1 just in case) */
	dev->a_qd = 64;
	if (dev->a_qd > max_q) dev->a_qd = max_q;

	dev->asq = kmalloc_aligned(4096, 4096);
	dev->acq = kmalloc_aligned(4096, 4096);
	if (!dev->asq || !dev->acq) return 0;
	memset(dev->asq, 0, 4096);
	memset(dev->acq, 0, 4096);

	r->aqa = ((uint32_t) (dev->a_qd - 1) << 16) | (uint32_t) (dev->a_qd - 1);
	r->asq = (uint64_t) (uintptr_t) dev->asq;
	r->acq = (uint64_t) (uintptr_t) dev->acq;

	/* Mask controller interrupts during bring-up (polling mode) */
	r->intms = 0xFFFFFFFFu;

	/* CC exactly as in ASM: IOSQES=6, IOCQES=4, EN=1 */
	r->cc = 0x00460001u;

	/* Wait for ready, fail if CFS */
	for (;;) {
		uint32_t c = r->csts;
		if (c & (1u << 1)) { /* CFS */
			dprintf("NVMe fatal during enable (CSTS.CFS=1)\n");
			return 0;
		}
		if (c & 1u) break;
		__asm__ volatile("pause");
	}

	dev->a_sqt = 0;
	dev->a_cqh = 0;
	dev->a_phase = 1;
	return 1;
}

static int nvme_identify_and_ioq(nvme_dev_t *dev) {
	dprintf("Identify and IOQ\n");

	/* ---- I/O queues first (ASM order) ---- */
	dev->iosq = kmalloc_aligned(4096, 4096);
	dev->iocq = kmalloc_aligned(4096, 4096);
	if (!dev->iosq || !dev->iocq) return 0;
	memset(dev->iosq, 0, 4096);
	memset(dev->iocq, 0, 4096);

	/* Use QD=64 like ASM (will clamp in helpers) */
	dev->io_qd = 64;

	/* Helpful snapshot before queue create */
	uint64_t cap_dbg = dev->regs->cap;
	uint32_t cc_dbg = dev->regs->cc;
	uint32_t aqa_dbg = dev->regs->aqa;
	dprintf("Pre-CreateCQ: CAP=0x%016lx CC=0x%08x AQA=0x%08x ASQ=0x%016lx ACQ=0x%016lx\n", cap_dbg, cc_dbg, aqa_dbg, dev->regs->asq, dev->regs->acq);

	if (!nvme_admin_create_cq(dev, 1, dev->iocq, dev->io_qd)) {
		dprintf("nvme_admin_create_cq failed\n");
		return 0;
	}
	if (!nvme_admin_create_sq(dev, 1, dev->iosq, dev->io_qd)) {
		dprintf("nvme_admin_create_sq failed\n");
		return 0;
	}
	dev->io_sqt = 0;
	dev->io_cqh = 0;
	dev->io_phase = 1;

	/* ---- Now do Identify path (ASM does it after queues) ---- */
	void *page = nvme_page_alloc_4k();
	if (!page) {
		return 0;
	}

	/* Active NS list */
	if (!nvme_admin_identify(dev, NVME_ID_CNS_NS_ACTIVE, 0, page)) {
		nvme_page_free_4k(page);
		return 0;
	}
	uint32_t first = 0;
	for (size_t i = 0; i < 1024; i++) {
		uint32_t id = ((uint32_t *) page)[i];
		if (id) {
			first = id;
			break;
		}
	}
	if (first == 0) {
		first = 1;
	}
	dev->nsid = first;
	dprintf("dev->nsid = %u\n", dev->nsid);

	/* Controller identify: model + ONCS + SQES/CQES (for sanity) */
	memset(page, 0, 4096);
	if (!nvme_admin_identify(dev, NVME_ID_CNS_CTRL, 0, page)) {
		nvme_page_free_4k(page);
		return 0;
	}
	uint8_t sqes = *(((uint8_t *) page) + 512);
	uint8_t cqes = *(((uint8_t *) page) + 513);
	dprintf("ID-CTRL: CQES req=%u max=%u  SQES req=%u max=%u\n",
		(unsigned) (cqes & 0x0F), (unsigned) ((cqes >> 4) & 0x0F),
		(unsigned) (sqes & 0x0F), (unsigned) ((sqes >> 4) & 0x0F));

	const char *mn = (const char *) page + 24;
	size_t n = 40;
	if (n > sizeof(dev->model) - 1) {
		n = sizeof(dev->model) - 1;
	}
	memcpy(dev->model, mn, n);
	dev->model[n] = 0;
	while (n && (dev->model[n - 1] == ' ' || dev->model[n - 1] == 0)) {
		dev->model[--n] = 0;
	}

	uint32_t oncs = *(const uint32_t *) ((const uint8_t *) page + 256);
	dev->has_trim = ((oncs & (1u << 2)) != 0);
	dprintf("Has trim=%u\n", dev->has_trim);

	/* Namespace identify: lbaf + size */
	memset(page, 0, 4096);
	if (!nvme_admin_identify(dev, NVME_ID_CNS_NS, dev->nsid, page)) {
		nvme_page_free_4k(page);
		return 0;
	}
	uint8_t flbas = *(((uint8_t *) page) + 26);
	uint8_t lbaf = (uint8_t) (flbas & 0x0F);
	uint8_t lbads = *(((uint8_t *) page) + 128 + (size_t) lbaf * 4 + 2);
	uint32_t native = (uint32_t) 1u << lbads;
	dev->ns_is_4k = (native == 4096);
	uint64_t nsze = *(const uint64_t *) (((const uint8_t *) page) + 0);
	uint64_t factor = (uint64_t) (native / 512u);
	dev->size_in_512 = nsze * factor;
	dprintf("is_4k=%u size_in_512=%lu native=%u lbads=%u\n",
		dev->ns_is_4k, dev->size_in_512, native, lbads);

	nvme_page_free_4k(page);
	return 1;
}

/* ---------- 512-byte outward I/O (RMW on 4 KiB native) ---------- */
static int nvme_read_512(nvme_dev_t *dev, uint64_t lba512, uint32_t count512, unsigned char *out) {
	unsigned char *p = out;
	void *page = nvme_page_alloc_4k();
	if (!page) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}

	while (count512 != 0) {
		if (!dev->ns_is_4k) {
			uint32_t this512 = count512 < 8 ? count512 : 8;
			if (!nvme_io_rw(dev, false, lba512, (uint16_t) this512, page)) {
				nvme_page_free_4k(page);
				fs_set_error(FS_ERR_IO);
				return 0;
			}
			memcpy(p, page, (size_t) this512 * 512);
			p += (size_t) this512 * 512;
			lba512 += this512;
			count512 -= this512;
		} else {
			/* head alignment */
			if ((lba512 & 7) != 0) {
				uint64_t aligned = lba512 & ~7ULL;
				uint32_t head_pad = (uint32_t) (lba512 - aligned);
				if (!nvme_io_rw(dev, false, aligned >> 3, 1, page)) {
					nvme_page_free_4k(page);
					fs_set_error(FS_ERR_IO);
					return 0;
				}
				size_t head_bytes = (size_t) (8 - head_pad) * 512;
				memcpy(p, (uint8_t *) page + (size_t) head_pad * 512, head_bytes);
				p += head_bytes;
				lba512 += (8 - head_pad);
				if (count512 >= (8 - head_pad)) {
					count512 -= (8 - head_pad);
				} else {
					count512 = 0;
				}
				continue;
			}
			if (count512 < 8) {
				if (!nvme_io_rw(dev, false, lba512 >> 3, 1, page)) {
					nvme_page_free_4k(page);
					fs_set_error(FS_ERR_IO);
					return 0;
				}
				size_t tail_bytes = (size_t) count512 * 512;
				memcpy(p, page, tail_bytes);
				p += tail_bytes;
				lba512 += count512;
				count512 = 0;
				continue;
			}
			if (!nvme_io_rw(dev, false, lba512 >> 3, 1, page)) {
				nvme_page_free_4k(page);
				fs_set_error(FS_ERR_IO);
				return 0;
			}
			memcpy(p, page, 4096);
			p += 4096;
			lba512 += 8;
			count512 -= 8;
		}
	}

	nvme_page_free_4k(page);
	return 1;
}

static int nvme_write_512(nvme_dev_t *dev, uint64_t lba512, uint32_t count512, const unsigned char *in) {
	const unsigned char *p = in;
	void *page = nvme_page_alloc_4k();
	if (!page) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return 0;
	}

	while (count512 != 0) {
		if (!dev->ns_is_4k) {
			uint32_t this512 = count512 < 8 ? count512 : 8;
			memset(page, 0, 4096);
			memcpy(page, p, (size_t) this512 * 512);
			if (!nvme_io_rw(dev, true, lba512, (uint16_t) this512, page)) {
				nvme_page_free_4k(page);
				fs_set_error(FS_ERR_IO);
				return 0;
			}
			p += (size_t) this512 * 512;
			lba512 += this512;
			count512 -= this512;
		} else {
			uint64_t aligned = lba512 & ~7ULL;
			uint32_t head_pad = (uint32_t) (lba512 - aligned);
			if (head_pad != 0 || count512 < 8) {
				if (!nvme_io_rw(dev, false, aligned >> 3, 1, page)) {
					nvme_page_free_4k(page);
					fs_set_error(FS_ERR_IO);
					return 0;
				}
			} else {
				memset(page, 0, 4096);
			}

			size_t copy_start = (size_t) head_pad * 512;
			size_t max_copy = 4096 - copy_start;
			size_t want_copy = (size_t) count512 * 512;
			size_t do_copy = want_copy < max_copy ? want_copy : max_copy;
			memcpy((uint8_t *) page + copy_start, p, do_copy);

			if (!nvme_io_rw(dev, true, aligned >> 3, 1, page)) {
				nvme_page_free_4k(page);
				fs_set_error(FS_ERR_IO);
				return 0;
			}

			p += do_copy;
			uint32_t sectors_done = (uint32_t) (do_copy / 512);
			lba512 += sectors_done;
			count512 -= sectors_done;
		}
	}

	nvme_page_free_4k(page);
	return 1;
}

int storage_device_nvme_block_read(void *dev_ptr, uint64_t start, uint32_t bytes, unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !buffer || bytes == 0) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	nvme_dev_t *dev = (nvme_dev_t *) sd->opaque1;
	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	unsigned char *end = buffer + bytes;

	while (sectors > 0) {
		uint32_t this_xfer = sectors > 8 ? 8 : sectors; /* one PRP page worth for 512 native; RMW handles 4k */
		uint32_t bytes_this = this_xfer * sd->block_size;

		if (buffer + bytes_this > end) {
			uint64_t bytes_left = (uint64_t) (end - buffer);
			this_xfer = (uint32_t) (bytes_left / sd->block_size);
			if (this_xfer == 0) {
				break;
			}
			bytes_this = this_xfer * sd->block_size;
		}

		if (!nvme_read_512(dev, start, this_xfer, buffer)) {
			return 0;
		}

		start += this_xfer;
		buffer += bytes_this;
		sectors -= this_xfer;
	}

	return 1;
}

int storage_device_nvme_block_write(void *dev_ptr, uint64_t start, uint32_t bytes, const unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !buffer) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	nvme_dev_t *dev = (nvme_dev_t *) sd->opaque1;
	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	while (sectors > 0) {
		uint32_t this_xfer = sectors > 8 ? 8 : sectors;
		if (!nvme_write_512(dev, start, this_xfer, buffer)) {
			return 0;
		}
		start += this_xfer;
		buffer += (size_t) this_xfer * sd->block_size;
		sectors -= this_xfer;
	}

	return 1;
}

bool storage_device_nvme_block_clear(void *dev_ptr, uint64_t start, uint32_t bytes) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}
	nvme_dev_t *dev = (nvme_dev_t *) sd->opaque1;
	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return false;
	}
	if (!dev->has_trim) {
		return false;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	if (!dev->ns_is_4k) {
		return nvme_io_trim(dev, start, sectors) ? true : false;
	} else {
		uint64_t s4k = start & ~7ULL;
		uint64_t e512 = start + sectors;
		uint64_t e4k = (e512 + 7ULL) & ~7ULL;
		uint64_t n4k = (e4k - s4k) >> 3;
		return nvme_io_trim(dev, s4k >> 3, (uint32_t) n4k) ? true : false;
	}
}

int nvme_identify_device(void *dev_ptr, char *name_out, size_t name_out_size, uint64_t *size_in_sectors_512) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !name_out || name_out_size == 0 || !size_in_sectors_512) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	nvme_dev_t *dev = (nvme_dev_t *) sd->opaque1;
	if (!dev) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}
	strlcpy(name_out, dev->model, name_out_size);
	*size_in_sectors_512 = dev->size_in_512;
	return 1;
}

void init_nvme(void) {
	pci_dev_t nvme_pci = pci_get_device(0, 0, NVME_CLASS_CODE);
	if (!nvme_pci.bits) {
		dprintf("NVMe: no device found\n");
		return;
	}

	pci_disable_memspace(nvme_pci);
	uint32_t bar0 = pci_read(nvme_pci, PCI_BAR0);
	uint32_t bar1 = pci_read(nvme_pci, PCI_BAR1);

	if (!pci_bar_is_mem64(bar0)) {
		dprintf("NVMe: BAR0 is not 64-bit MMIO\n");
		return;
	}

	uint64_t mem_base = pci_mem_base64(bar0, bar1);
	uint64_t bar_size = get_bar_size(nvme_pci, 0);
	dprintf("NVME: MMIO base 0x%016lx size %lu\n", mem_base, bar_size);

	pci_enable_memspace(nvme_pci);
	pci_bus_master(nvme_pci);

	if (!mmio_identity_map(mem_base, bar_size)) {
		dprintf("Can't identity map mem_base\n");
		return;
	}

	nvme_dev_t *dev = kmalloc(sizeof(nvme_dev_t));
	if (!dev) {
		return;
	}

	memset(dev, 0, sizeof(*dev));
	dev->regs = (volatile nvme_regs_t *) (uintptr_t) mem_base;

	if (!nvme_hw_enable(dev)) {
		kfree(dev);
		return;
	}
	if (!nvme_identify_and_ioq(dev)) {
		dprintf("nvme_identify_and_ioq failed\n");
		kfree(dev);
		return;
	}

	storage_device_t *sd = kmalloc(sizeof(storage_device_t));
	if (!sd) {
		dprintf("Out of memory allocating storage_device (nvme)");
		kfree(dev);
		return;
	}
	memset(sd, 0, sizeof(*sd));

	if (!make_unique_device_name("hd", sd->name, sizeof(sd->name))) {
		dprintf("Failed to make unique device name");
		kfree(sd);
		kfree(dev);
		return;
	}

	sd->opaque1 = dev;
	sd->opaque2 = NULL;
	sd->opaque3 = NULL;
	sd->cache = NULL;
	sd->blockread = storage_device_nvme_block_read;
	sd->blockwrite = storage_device_nvme_block_write;
	sd->blockclear = storage_device_nvme_block_clear;
	sd->block_size = 512;
	sd->size = dev->size_in_512;

	/* end-user label */
	char size_str[24] = {0};
	humanise_capacity(size_str, sizeof(size_str), dev->size_in_512 * 512);
	if (dev->model[0]) {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "%s - %s", dev->model, size_str);
	} else {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "NVMe Device - %s", size_str);
	}
	sd->ui.is_optical = false;

	register_storage_device(sd);
	storage_enable_cache(sd);
	kprintf("NVMe storage: %s (Native Sector Size: %d%s)\n", sd->ui.label, dev->ns_is_4k ? 4096 : 512, dev->has_trim ? ", TRIM" : "");
}

