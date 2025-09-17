/**
 * @file modules/ac97/ac97.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#include <kernel.h>
#include <stdbool.h>

/* BAR0 = NAM (Native Audio Mixer) — I/O ports */
#define AC97_NAM_RESET          0x00 /* write-anything: soft reset; read: caps */
#define AC97_NAM_MASTER_VOL     0x02 /* word: 0x0000=max, bit15=mute */
#define AC97_NAM_PCM_OUT_VOL    0x18 /* word: 0x0000=max, bit15=mute */
#define AC97_NAM_EXT_CAPS       0x28 /* word: bit0 = variable-rate capable */
#define AC97_NAM_EXT_CTRL       0x2A /* word: bit0 = enable variable-rate */
#define AC97_NAM_PCM_FRONT_RATE 0x2C /* word: sample rate when var-rate enabled */

/* BAR1 = NABM (Native Audio Bus Master) — I/O ports */
#define AC97_NABM_PO_BDBAR      0x10 /* dword: phys addr of BDL for PCM OUT */
#define AC97_NABM_PO_CIV        0x14 /*  u8  : current index (RO) */
#define AC97_NABM_PO_LVI        0x15 /*  u8  : last valid index (0..31) */
#define AC97_NABM_PO_SR         0x16 /* u16  : status (W1C bits) */
#define AC97_NABM_PO_PICB       0x18 /* u16  : samples left in current entry (RO) */
#define AC97_NABM_PO_CR         0x1B /*  u8  : control: bit1=RST, bit0=RPBM(run) */

#define AC97_NABM_GLOB_CNT      0x2C /* u32  : bit1=Cold Reset run (1=out of reset) */

/* PO_SR W1C bits (clear by writing 1s) */
#define AC97_SR_LVBCI           0x0004
#define AC97_SR_DCH             0x0008
#define AC97_SR_CELV            0x0010
#define AC97_SR_W1C_MASK        (AC97_SR_LVBCI | AC97_SR_DCH | AC97_SR_CELV)

/* PO_CR bits */
#define AC97_CR_RPBM            0x01
#define AC97_CR_RST             0x02

struct ac97_bdl_entry {
	uint32_t addr;   /* phys addr of audio data */
	uint16_t count;  /* number of samples in this buffer (all channels) */
	uint16_t flags;  /* bit15=IOC, bit14=BUP (buffer underrun policy / stop) */
} __attribute__((packed));

typedef struct {
	uint16_t nam;          /* I/O base (NAM) */
	uint16_t nabm;         /* I/O base (NABM) */
	struct ac97_bdl_entry *bdl;
	uint32_t bdl_phys;
	uint8_t *buf;
	uint32_t buf_phys;
	uint32_t buf_bytes;
	uint8_t bdl_n;         /* always 32 (hardware modulo) */
	uint32_t frag_bytes;    /* bytes per descriptor (multiple of 128, multiple of 4) */
	uint32_t frag_frames;   /* frag_bytes / 4 (S16LE stereo) */
	int tail;          /* last valid index we’ve published to HW; -1 means empty */
	bool started;       /* PO engine running flag */
	uint32_t rate_hz;
} ac97_dev_t;

/**
 * @brief  Enqueue PCM frames into the software FIFO without blocking.
 *
 * Appends interleaved S16LE stereo frames to the software queue. The hardware
 * ring is not touched here; the queue is drained opportunistically by
 * ac97_idle(). On success the whole buffer is accepted; on OOM nothing is
 * queued and 0 is returned.
 *
 * @param frames       Pointer to interleaved S16LE stereo samples (L,R).
 * @param total_frames Number of stereo frames to enqueue (not samples).
 *
 * @return size_t  Number of frames accepted into the queue
 *                 (== total_frames on success, 0 on failure/invalid args).
 *
 * @pre    Stream prepared (g.bdl != NULL, g.frag_frames > 0).
 * @note   This call is lock-free here; serialise at a higher level if multiple
 *         producers may call it concurrently.
 */
size_t push_all_s16le(const int16_t *frames, size_t total_frames);

/**
 * @brief  Idle hook: drain the software FIFO into the AC'97 DMA ring.
 *
 * Publishes up to the currently available descriptor space, then returns
 * immediately (never blocks). If paused, or if there is no pending data,
 * it is a cheap no-op. Also clears halted status and re-arms RUN if needed.
 *
 * @post   May advance g.tail/LVI and the software queue head.
 * @note   Safe to call frequently from a cooperative timer.
 */
void ac97_idle(void);

/**
 * @brief  Immediate hard stop and flush.
 *
 * Halts the PCM Out engine, resets the bus-master block, clears status,
 * re-points the BDL, and discards all pending software-queued audio.
 *
 * @post   DMA engine stopped; software queue empty; paused flag cleared.
 * @warning Any audio in flight is dropped; use ac97_pause() if you intend
 *          to resume without losing buffered data.
 */
void ac97_stop_clear(void);

/**
 * @brief  Pause playback without clearing buffers.
 *
 * Stops the PCM Out engine but preserves both the hardware ring contents
 * and the software queue so playback can be resumed later.
 *
 * @post   DMA engine stopped; queued data retained; s_paused = true.
 */
void ac97_pause(void);

/**
 * @brief  Resume playback after a pause.
 *
 * Clears latched status, sets RUN, clears the paused flag, and performs an
 * opportunistic drain via ac97_idle() to kick the ring if space exists.
 *
 * @post   DMA engine running; s_paused = false.
 */
void ac97_resume(void);

/**
 * @brief  Return total buffered audio time in milliseconds.
 *
 * Computes the sum of software-queued frames and hardware-queued frames
 * (pending descriptors plus PICB for the current fragment), then converts
 * to milliseconds using the current DAC rate.
 *
 * @return uint32_t  Milliseconds of audio ahead of the DAC.
 *
 * @pre    g.rate_hz reflects the active PCM rate; g.frag_frames > 0.
 * @note   Assumes g.bdl_n is a power of two (e.g., 32).
 */
uint32_t ac97_buffered_ms(void);

/**
 * @brief  Query the current DAC sample rate (Hz).
 *
 * If Variable Rate Audio (VRA) is enabled, returns the programmed
 * PCM Front DAC rate register; otherwise returns the fixed 48 kHz rate.
 *
 * @return uint32_t  Active sample rate in Hz (e.g., 44100 or 48000).
 */
uint32_t ac97_get_hz(void);

/**
 * @brief  Test helper: load a RAW S16LE stereo file and enqueue it.
 *
 * Loads "/system/webserver/test.raw" (header-less, S16LE, stereo) into
 * memory and enqueues the entire file into the software FIFO using
 * push_all_s16le(). Playback proceeds asynchronously via ac97_idle().
 *
 * @note   The stream must already be prepared; the file’s sample rate must
 *         match the active DAC rate or it will play at the wrong speed.
 * @warning Reads the whole file into memory in one go (no streaming).
 */
void ac97_test_melody(void);

/**
 * @brief  Prepare AC’97 PCM Out stream buffers and descriptors.
 *
 * Allocates and initialises the Buffer Descriptor List (BDL) and backing
 * DMA buffer for the PCM Out engine. Configures fragment size, fragment
 * count, and sample rate. If frag_bytes is zero, a default of 4096 bytes
 * per fragment is used. The DMA engine itself is not started here.
 *
 * @param[in] frag_bytes   Bytes per fragment (must be multiple of 128).
 *                         Pass 0 to use the default (4096).
 * @param[in] sample_rate  Desired sample rate in Hz (e.g. 44100, 48000).
 *
 * @return true on success, false on allocation/initialisation failure.
 *
 * @pre    Device has been probed and reset; g.nam/g.nabm valid.
 * @post   g.bdl, g.buf, g.frag_bytes, g.frag_frames, g.rate_hz initialised.
 */
bool ac97_stream_prepare(uint32_t frag_bytes, uint32_t sample_rate);

/**
 * @brief  Idle hook: drain the software FIFO into the AC’97 DMA ring.
 *
 * Publishes queued audio into available DMA descriptors without blocking.
 * If no audio is queued or if the engine is paused, this is a cheap no-op.
 * Also re-arms the PCM Out engine if a halt or underrun was detected.
 *
 * @pre    Stream prepared via ac97_stream_prepare().
 * @post   Advances software FIFO head and may update DMA write pointer.
 * @note   Should be called regularly from the cooperative scheduler.
 */
void ac97_idle(void);
