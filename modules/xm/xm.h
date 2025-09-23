#include <stdint.h>
#pragma once

#define XM_MAGIC_TEXT                 "Extended Module: "
#define XM_MAGIC_LEN                  17

#define XM_MAX_CHANNELS               32
#define XM_MAX_ORDERS                256
#define XM_MAX_PATTERNS              256
#define XM_MAX_INSTRUMENTS           128
#define XM_MAX_ENV_POINTS             12
#define XM_TARGET_RATE             44100

/* Pattern decode results per cell */
typedef struct {
	uint8_t note;          /* 0=no note, 1..96 = C-0..B-7, 97=Note Off */
	uint8_t instrument;    /* 0=no change, 1..128 */
	uint8_t volcol;        /* raw volume column byte (0..255, 0=none) */
	uint8_t effect;        /* raw effect type byte */
	uint8_t param;         /* raw effect param byte */
} xm_cell_t;

/* Decoded pattern: rows x channels */
typedef struct {
	uint16_t rows;         /* 1..256 (FT2 commonly 64) */
	xm_cell_t *cells;      /* rows * channels */
} xm_pattern_t;

/* One sample attached to an instrument */
typedef struct {
	int16_t *pcm;          /* decoded PCM (S16) */
	uint32_t length;       /* samples (not bytes) */
	uint32_t loop_start;   /* samples */
	uint32_t loop_len;     /* samples */
	int loop_type;         /* 0=none,1=forward,2=pingpong */
	int relative_note;     /* -96..+96 typical */
	int fine_tune;         /* -128..+127 (1/128 semitone units) */
	int default_volume;    /* 0..64 */
	int default_pan;       /* 0..255 */
} xm_sample_t;

/* Envelope definition (discrete points) */
typedef struct {
	int enabled;
	int sustain;           /* point index */
	int loop_start;        /* point index */
	int loop_end;          /* point index */
	int num_points;        /* 0..12 */
	int points[XM_MAX_ENV_POINTS][2]; /* x=tick, y=value(0..64 for vol, 0..255 for pan) */
} xm_envelope_t;

/* Instrument (keymap + envelopes) */
typedef struct {
	int num_samples;
	xm_sample_t *samples;
	int keymap[96];        /* 1..96 => sample index (0..num_samples-1), -1 for none */
	xm_envelope_t vol_env;
	xm_envelope_t pan_env;
	int fadeout;           /* 0..32767 (applied post key-off per tick) */
} xm_instrument_t;

/* Per-channel runtime */
typedef struct {
	int active;

	/* Current instrument/sample selection */
	int inst_idx;          /* -1 if none */
	int smp_idx;           /* -1 if none */
	xm_sample_t *smp;      /* shortcut */

	/* Playback cursor and step */
	int sample_pos;        /* integer sample index */
	int sample_frac;       /* Q16 fractional position */
	int sample_inc;        /* Q16 step per output sample */
	int loop_dir;          /* +1 forward, -1 backward (for pingpong) */

	/* Musical state */
	int key_on;            /* 1 while note held, cleared by Note Off */
	int curr_note;         /* last note value (1..96) */
	int target_note;       /* for tone portamento */
	int period;            /* Amiga-style period (for compat path) */
	int porta_speed;       /* last non-zero 3xx speed */
	int vol;               /* 0..64 base volume before envelopes */
	int pan;               /* 0..255 */
	int global_vol_applied;/* cached last global vol applied */

	/* Effect oscillators */
	int vib_phase;         /* 0..63 */
	int vib_speed;         /* 0..63 (volcol 0xA?) */
	int vib_depth;         /* 0..15 (volcol 0xB?) */
	int trem_phase;        /* 0..63 */
	int trem_speed;        /* 0..63 */
	int trem_depth;        /* 0..15 */

	/* Envelope runtime */
	int vol_env_tick;      /* elapsed ticks on vol env timeline */
	int pan_env_tick;      /* elapsed ticks on pan env timeline */
	int released;          /* set on Note Off to enable fadeout */
	int fade;              /* running fadeout value */

	/* Memorised params */
	int sample_offset_mem; /* for 9xx with param=0 */
	int volslide_mem;      /* for Axy when x/y = 0 in combo effects */
	int retrig_countdown;  /* E9x retrigger counter */
} xm_channel_t;

/* Whole song runtime */
typedef struct {
	/* Song */
	int channels;
	int orders;
	const uint8_t *order_table; /* size >= orders */
	int patterns_count;
	int instruments_count;
	int flags_linear_freq;      /* 1 if linear frequency mode */
	int initial_speed;          /* ticks/row */
	int initial_tempo;          /* BPM */

	/* Runtime position */
	int order;
	int row;
	int tick;
	int speed;                  /* ticks per row (runtime) */
	int tempo;                  /* BPM (runtime) */
	int ended;
	int pat_loop_row;
	int pat_loop_count;

	/* Timing to audio */
	uint32_t samplerate;
	uint32_t audio_speed;       /* samples per tick */
	uint32_t audio_tick;        /* samples remaining in current tick */

	/* Globals */
	int global_vol;             /* 0..64 */

	/* Assets */
	xm_pattern_t *patterns;     /* patterns_count */
	xm_instrument_t *instruments; /* instruments_count */

	/* Channels */
	xm_channel_t ch[XM_MAX_CHANNELS];

	/* Decode scratch */
	int rng;
} xm_state_t;
