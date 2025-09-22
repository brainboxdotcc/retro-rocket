#include <stdint.h>
#pragma once

#define MOD_HEADER_LEN            1084
#define MOD_SIG_OFFSET            1080
#define MOD_ORDER_COUNT_OFFSET     950
#define MOD_ORDER_TABLE_OFFSET     952
#define MOD_SAMPLE_COUNT           31
#define MOD_SAMPLE_HDR_BYTES       30
#define MOD_NOTE_BYTES              4
#define MOD_ROWS_PER_PATTERN       64
#define MOD_MAX_ORDERS            128
#define MOD_MAX_CHANNELS           32
#define MOD_TARGET_RATE         44100

/* Linear-phase tables and helpers */
static const int32_t finetune_q16[16] = {
	65536, 65065, 64596, 64132,
	63670, 63212, 62757, 62306,
	69433, 68933, 68438, 67945,
	67456, 66971, 66489, 66011
};

static const uint8_t sine_table[32] = {
	0, 24, 49, 74, 97, 120, 141, 161,
	180, 197, 212, 224, 235, 244, 250, 253,
	255, 253, 250, 244, 235, 224, 212, 197,
	180, 161, 141, 120, 97, 74, 49, 24
};

static const int32_t arpeggio_q16[16] = {
	65536, 61858, 58386, 55109,
	52016, 49096, 46341, 43740,
	41285, 38968, 36781, 34716,
	32768, 30929, 29193, 27554
};

typedef struct {
	const int8_t *sample;
	uint32_t age;
	uint32_t current_ptr;
	uint32_t length;
	uint32_t loop_length;
	uint32_t period_q16;       /* period as Q16.16 step over sample data */
	int32_t volume;           /* 0..64 */
	int32_t subptr_q16;       /* fractional position within current sample */
	int8_t muted;
} mod_channel_t;

typedef struct {
	const int8_t *data;
	uint32_t actual_len;  /* bytes */
	uint32_t loop_len;    /* bytes (0 if no loop) */
} mod_sample_t;

typedef struct {
	int32_t val;       /* scaled by depth */
	uint8_t waveform;  /* 0 sine, 1 saw, 2 square, 3 random */
	uint8_t phase;     /* 0..63 */
	uint8_t speed;     /* 0..15 */
	uint8_t depth;     /* 0..15 (4-bit classic) */
} osc_t;

typedef struct {
	uint32_t note_period;
	uint8_t sample_idx;
	uint8_t eff;
	uint8_t effval;

	uint8_t slide_amount;
	uint8_t sample_offset;
	int16_t vol;            /* 0..64 */
	int32_t slide_target;   /* used by portamento */

	int32_t period;         /* current period */

	osc_t vibrato;
	osc_t tremolo;
	mod_channel_t gen;

	uint8_t  glissando_on;
} tracker_ch_t;

typedef struct {
	/* song state */
	int channels;
	int orders;
	int max_pattern;
	int order;
	int row;
	int tick;
	int max_tick;        /* ticks per row (speed) */
	int speed;           /* current speed */
	int skip_order_req;
	int skip_order_dest_row;
	int pat_loop_row;
	int pat_loop_cycle;

	/* rates/timers */
	uint32_t samplerate;
	uint32_t sample_rate_q16;  /* (PAL clock << 16) / samplerate */
	uint32_t audio_speed;     /* samples per tick */
	uint32_t audio_tick;      /* samples remaining in current tick */
	uint32_t rng;             /* simple LCG; intentionally deterministic */

	tracker_ch_t ch[MOD_MAX_CHANNELS];

	const uint8_t *pattern_data;      /* start of pattern region */
	const uint8_t *order_table;       /* 128 bytes */
	const uint8_t *file_bytes;        /* entire file for bounds checks */
	size_t file_len;

	/* sample metadata */
	mod_sample_t samples[MOD_SAMPLE_COUNT];
	const uint8_t *sample_hdrs;       /* pointer into file (31 * 30 bytes) */

	/* end detection */
	bool ended;
} mod_state_t;
