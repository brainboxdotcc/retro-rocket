/**
 * @file xm.h
 * @brief FastTracker II (XM) runtime structures and constants.
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 *
 * Defines immutable limits, parse/runtime data structures, and
 * per-channel/per-song state used by the XM loader and mixer.
 *
 * @details
 * - Notes: 0 = none, 1..96 = C-0..B-7, 97 = Note Off.
 * - Instruments: 0 = no change, 1..128 selects instrument.
 * - Volume column: 0 = none, otherwise raw byte.
 * - Order table: 0xFF indicates end-of-song.
 */

#pragma once
#include <stdint.h>

/** @brief ASCII file signature present at the beginning of XM files. */
#define XM_MAGIC_TEXT  "Extended Module: "

/** @brief Length of XM_MAGIC_TEXT in bytes. */
#define XM_MAGIC_LEN   17

/** @brief Maximum number of mixer channels supported. */
#define XM_MAX_CHANNELS   32

/** @brief Maximum length of the order table. */
#define XM_MAX_ORDERS    256

/** @brief Maximum number of addressable patterns. */
#define XM_MAX_PATTERNS  256

/** @brief Maximum number of instruments. */
#define XM_MAX_INSTRUMENTS 128

/** @brief Maximum number of envelope points per envelope. */
#define XM_MAX_ENV_POINTS  12

/** @brief Internal rendering samplerate in Hz. */
#define XM_TARGET_RATE     44100

/**
 * @brief One decoded pattern cell (note/effect slot).
 */
typedef struct {
	/** Note value: 0 = none, 1..96 = C-0..B-7, 97 = Note Off. */
	uint8_t note;
	/** Instrument: 0 = no change, 1..128 selects instrument. */
	uint8_t instrument;
	/** Raw volume column byte (0..255, 0 = none). */
	uint8_t volcol;
	/** Raw effect type byte. */
	uint8_t effect;
	/** Raw effect parameter byte. */
	uint8_t param;
} xm_cell_t;

/**
 * @brief A fully decoded pattern: rows × channels cells.
 */
typedef struct {
	/** Number of rows in this pattern (1..256, typically 64). */
	uint16_t rows;
	/** Pointer to cell storage: rows × channels. */
	xm_cell_t *cells;
} xm_pattern_t;

/**
 * @brief One decoded sample attached to an instrument.
 */
typedef struct {
	/** Pointer to decoded PCM data (signed 16-bit mono). */
	int16_t *pcm;
	/** Number of samples in pcm (not bytes). */
	uint32_t length;
	/** Loop start position in samples. */
	uint32_t loop_start;
	/** Loop length in samples. */
	uint32_t loop_len;
	/** Loop type: 0 = none, 1 = forward, 2 = ping-pong. */
	int loop_type;
	/** Relative note offset (typical -96..+96 semitones). */
	int relative_note;
	/** Fine tune in 1/128 semitone units (-128..127). */
	int fine_tune;
	/** Default volume (0..64). */
	int default_volume;
	/** Default pan (0..255). */
	int default_pan;
} xm_sample_t;

/**
 * @brief Discrete envelope definition.
 */
typedef struct {
	/** Non-zero when enabled. */
	int enabled;
	/** Sustain point index. */
	int sustain;
	/** Loop start point index. */
	int loop_start;
	/** Loop end point index. */
	int loop_end;
	/** Number of valid points. */
	int num_points;
	/** Envelope points {tick, value}. */
	int points[XM_MAX_ENV_POINTS][2];
} xm_envelope_t;

/**
 * @brief Instrument definition: keymap, envelopes, samples.
 */
typedef struct {
	/** Number of samples attached. */
	int num_samples;
	/** Pointer to array of samples. */
	xm_sample_t *samples;
	/** Keymap: note 1..96 to sample index, or -1 for none. */
	int keymap[96];
	/** Volume envelope. */
	xm_envelope_t vol_env;
	/** Pan envelope. */
	xm_envelope_t pan_env;
	/** Fadeout applied per tick after Note Off (0..32767). */
	int fadeout;
} xm_instrument_t;

/**
 * @brief Runtime state for a single channel.
 */
typedef struct {
	int active;              /**< Non-zero if active. */

	/* Instrument/sample selection */
	int inst_idx;            /**< Instrument index, -1 if none. */
	int smp_idx;             /**< Sample index, -1 if none. */
	xm_sample_t *smp;        /**< Shortcut to current sample. */

	/* Playback cursor and step */
	int sample_pos;          /**< Integer sample index. */
	int sample_frac;         /**< Q16 fractional position. */
	int sample_inc;          /**< Q16.16 step per output sample. */
	int loop_dir;            /**< +1 forward, -1 backward for ping-pong. */

	/* Musical state */
	int key_on;              /**< 1 while key held, cleared by Note Off. */
	int curr_note;           /**< Current effective note value (1..96). */
	int target_note;         /**< Target note for portamento. */
	int period;              /**< Amiga-style period. */
	int porta_speed;         /**< Last non-zero 3xx speed. */
	int vol;                 /**< Base volume before envelopes (0..64). */
	int pan;                 /**< Pan position (0..255). */
	int global_vol_applied;  /**< Cached last global volume applied. */

	/* Effect oscillators */
	int vib_phase;           /**< Vibrato phase (0..63). */
	int vib_speed;           /**< Vibrato speed (0..63). */
	int vib_depth;           /**< Vibrato depth (0..15). */
	int trem_phase;          /**< Tremolo phase (0..63). */
	int trem_speed;          /**< Tremolo speed (0..63). */
	int trem_depth;          /**< Tremolo depth (0..15). */

	/* Envelope runtime */
	int vol_env_tick;        /**< Elapsed ticks on volume envelope. */
	int pan_env_tick;        /**< Elapsed ticks on pan envelope. */
	int released;            /**< Set on Note Off to enable fadeout. */
	int fade;                /**< Running fadeout value. */

	/* Memorised params */
	int sample_offset_mem;   /**< Remembered 9xx sample offset. */
	int volslide_mem;        /**< Remembered Axy volume slide. */
	int retrig_countdown;    /**< E9x retrigger countdown. */
} xm_channel_t;

/**
 * @brief Whole XM song runtime state.
 */
typedef struct {
	/* Song */
	int channels;                /**< Number of channels. */
	int orders;                  /**< Number of orders. */
	const uint8_t *order_table;  /**< Pointer to order table. */
	int patterns_count;          /**< Number of patterns. */
	int instruments_count;       /**< Number of instruments. */
	int flags_linear_freq;       /**< 1 if linear frequency mode. */
	int initial_speed;           /**< Initial ticks/row. */
	int initial_tempo;           /**< Initial BPM. */

	/* Runtime position */
	int order;                   /**< Current order index. */
	int row;                     /**< Current row index. */
	int tick;                    /**< Current tick within row. */
	int speed;                   /**< Runtime ticks per row. */
	int tempo;                   /**< Runtime BPM. */
	int ended;                   /**< Non-zero if playback ended. */
	int pat_loop_row;            /**< Pattern loop start row. */
	int pat_loop_count;          /**< Pattern loop counter. */

	/* Timing to audio */
	uint32_t samplerate;         /**< Output samplerate. */
	uint32_t audio_speed;        /**< Samples per tick. */
	uint32_t audio_tick;         /**< Remaining samples in tick. */

	int restart_order;           /**< Restart order from header. */
	int restart_row;             /**< Restart row (usually 0). */
	int loop_armed;              /**< Armed when left restart pos. */

	/* Globals */
	int global_vol;              /**< Global volume (0..64). */

	/* Assets */
	xm_pattern_t *patterns;      /**< Array of patterns. */
	xm_instrument_t *instruments;/**< Array of instruments. */

	/* Channels */
	xm_channel_t ch[XM_MAX_CHANNELS]; /**< Channel states. */

	/* Decode scratch */
	int rng;                     /**< Internal RNG scratch. */
} xm_state_t;
