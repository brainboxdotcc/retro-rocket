/**
 * @file audio.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012–2025
 * @brief Minimal audio device abstraction for Retro Rocket.
 *
 * Provides a tiny, device-agnostic API for pushing interleaved stereo
 * S16LE frames to an output, querying the current sample rate, pausing,
 * resuming, and reporting buffered time. All frame counts refer to
 * **stereo frames** (L+R), not individual samples.
 */
#pragma once
#include <kernel.h>

typedef enum {
	TONE_SQUARE,
	TONE_TRIANGLE,
	TONE_SINE,
	TONE_SAW,
	TONE_NOISE
} tone_wave_t;

typedef struct {
	bool      in_use;
	tone_wave_t wave;        /* square/tri/sine/saw/noise */
	uint8_t   volume;        /* overall level (0–255) */
	uint8_t   pulse_width;   /* square duty, 128 = 50% */

	uint32_t  attack_ms;
	uint32_t  decay_ms;
	uint8_t   sustain;       /* sustain level (0–255) */
	uint32_t  release_ms;

	int32_t   vibrato_cents;
	uint32_t  vibrato_hz;
	uint32_t  glide_ms;

	uint32_t  pwm_hz;
	uint8_t   pwm_depth;
} sound_envelope_ex_t;

/**
 * @brief Callback: enqueue interleaved S16LE stereo frames (non-blocking).
 *
 * Implementations should accept as many frames as possible immediately and
 * return the number actually taken (which may be less than requested).
 *
 * @param frames Pointer to interleaved S16LE frames (L,R).
 * @param total_frames Number of stereo frames to enqueue.
 * @return Number of frames accepted this call.
 */
typedef size_t (*audio_push_t)(const int16_t *frames, size_t total_frames);

/**
 * @brief Callback: query the device’s current output rate in Hz.
 * @return Active sample rate (e.g., 44100 or 48000).
 */
typedef uint32_t (*audio_freq_t)(void);

/**
 * @brief Callback: immediate stop and clear of all buffered audio.
 */
typedef void (*audio_stop_t)(void);

/**
 * @brief Callback: pause output without clearing buffers.
 */
typedef void (*audio_pause_t)(void);

/**
 * @brief Callback: resume output after a pause.
 */
typedef void (*audio_resume_t)(void);

/**
 * @brief Callback: total buffered audio time (software queue + DMA) in ms.
 * @return Milliseconds of audio ahead of the DAC.
 */
typedef uint32_t (*audio_length_t)(void);

/**
 * @brief Callback: Get a list of audio outputs by name supported by the driver
 * @return Drivers as null terminated strings
 */
typedef const char** (*audio_outputs_list_t)(void);

/**
 * @brief Callback: Select audio output by name
 * @param output_name Output name to choose
 * @return True if selected
 */
typedef bool (*audio_select_output_t)(const char* output_name);

/**
 * @brief Callback: Get current output by name
 * @return Current output name
 */
typedef const char* (*audio_get_current_output_t)(void);

/**
 * @brief Callback: Try to load audio from a file buffer
 */
typedef bool (*try_load_audio_t)(const char*,const void*, size_t, void**, size_t*);

/** @brief Maximum length (including NUL) for an audio device’s display name. */
#define MAX_AUDIO_DEVICE_NAME 32

/** @brief Opaque stream handle. */
typedef struct mixer_stream mixer_stream_t;

/**
 * @brief Registered audio device descriptor.
 *
 * Drivers populate one of these and register it with the audio subsystem.
 * The list of devices is kept as an intrusive singly-linked list via @ref next.
 *
 * @note PCM format is S16LE, interleaved (L,R,L,R,…). All counts are frames.
 */
typedef struct audio_device_t {
	/** Driver-specific context pointer (may be NULL). */
	void *opaque;

	/** Human-readable device name (NUL-terminated, max @ref MAX_AUDIO_DEVICE_NAME). */
	char name[MAX_AUDIO_DEVICE_NAME];

	/** Non-blocking PCM enqueue (S16LE stereo frames). */
	audio_push_t play;

	/** Current output sample rate in Hz. */
	audio_freq_t frequency;

	/** Immediate stop + clear of queued/buffered data. */
	audio_stop_t stop;

	/** Pause output (retain queued/buffered data). */
	audio_pause_t pause;

	/** Resume output after pause. */
	audio_resume_t resume;

	/** Buffered time (software queue + DMA in flight) in milliseconds. */
	audio_length_t queue_length;

	/** Get array of output names */
	audio_outputs_list_t get_outputs;

	/** Select output by name */
	audio_select_output_t select_output;

	/** Get current output as string */
	audio_get_current_output_t get_current_output;

	/** Next device in the global list. */
	struct audio_device_t *next;
} audio_device_t;

typedef struct audio_file_loader_t {
	void* opaque;

	try_load_audio_t try_load_audio;

	struct audio_file_loader_t* next;
} audio_file_loader_t;

/**
 * @brief Register a new audio device.
 *
 * Inserts @p newdev into the global device list. The caller retains ownership
 * of the storage; the struct must remain valid for the lifetime of the device
 *
 * @param newdev Pointer to the device descriptor to register (must not be NULL)
 * @return true on success, false on failure.
 */
bool register_audio_device(audio_device_t *newdev);

/**
 * @brief Find a registered audio device by name
 *
 * @param name NUL-terminated device name to search for
 * @return Pointer to the matching device, or NULL if not found
 */
audio_device_t *find_audio_device(const char *name);

/**
 * @brief Get the first registered audio device, if any
 * @return Pointer to the first device in the list, or NULL if none
 */
audio_device_t *find_first_audio_device(void);

/**
 * @brief Initialise the mixer and register the idle tick
 *
 * Consumes queued frames from all active streams, applies gain/mute,
 * mixes them with 32-bit accumulation using SSE2, clips to 16-bit,
 * and pushes the result to the first audio device until the desired
 * latency is met. This is the sole hot path of the software mixer.
 *
 * @param dev               Audio device to attach the mixer to
 * @param target_latency_ms Desired steady-state output latency in ms
 * @param idle_period_ms    Foreground idle period in ms (mix cadence)
 * @param max_streams      Maximum number of concurrently open streams
 * @return true on success, false if no audio device or allocation failed
 */
bool mixer_init(audio_device_t* dev, uint32_t target_latency_ms, uint32_t idle_period_ms, uint32_t max_streams);

/**
 * @brief Open a stream slot for a producer
 *
 * Single producer per stream, the mixer is the sole consumer
 * @return Stream handle or NULL if none available
 */
mixer_stream_t *mixer_create_stream(void);

/**
 * @brief Close a stream (drains remaining data then frees the slot)
 */
void mixer_free_stream(mixer_stream_t *ch);

/**
 * @brief Submit interleaved S16LE stereo frames to a stream (non-blocking)
 *
 * Identical signature and semantics to audio_push_t; returns frames accepted
 */
size_t mixer_push(mixer_stream_t *ch, const int16_t *frames, size_t total_frames);

/**
 * @brief Set per-stream gain (Q8.8; 256 == 1.0).
 */
void mixer_set_gain(mixer_stream_t *ch, uint16_t q8_8_gain);

/**
 * @brief Mute/unmute a stream; muted streams do not contribute to the mix.
 */
void mixer_set_mute(mixer_stream_t *ch, bool mute);

/**
 * @brief Query frames currently queued in a stream
 */
uint32_t mixer_stream_queue_length(mixer_stream_t *ch);

/**
 * @brief Return the audio device used by the mixer (first device) or NULL.
 */
audio_device_t *mixer_device(void);

/**
 * @brief Foreground idle callback to drive the mixer.
 *
 * Called periodically by the process idle scheduler to top up the
 * output device with freshly mixed audio. Consumes queued frames
 * from all active mixer streams, applies gain/mute, and pushes
 * the mixed stream to the first audio device until the desired
 * latency is reached.
 *
 * This function is not intended to be called directly; it is
 * registered with @ref proc_register_idle by @ref mixer_init.
 */
void mixer_idle(void);

/**
 * @brief Immediately stop playback on a stream
 *
 * Clears all queued audio data and silences the stream.
 *
 * @param ch Stream handle to stop
 */
void mixer_stop_stream(mixer_stream_t *ch);

/**
 * @brief Pause playback on a stream
 *
 * Retains queued data but halts consumption until resumed.
 *
 * @param ch Stream handle to pause
 */
void mixer_pause_stream(mixer_stream_t *ch);

/**
 * @brief Resume playback on a paused stream
 *
 * Playback continues from the queued position.
 *
 * @param ch Stream handle to resume
 */
void mixer_resume_stream(mixer_stream_t *ch);

/**
 * @brief Query whether a stream is paused
 *
 * @param ch Stream handle to query
 * @return true if paused, false if active
 */
bool mixer_stream_is_paused(mixer_stream_t *ch);

/**
 * @brief Convert a memory-resident WAV into 44.1 kHz stereo S16_LE
 *
 * Parses an in-memory RIFF/WAVE file and converts its audio stream to
 * 44.1 kHz, stereo, signed 16-bit little-endian samples. The output is
 * always interleaved left/right at the fixed rate and format, regardless
 * of the input encoding.
 *
 * Supports:
 *  - PCM integer (format tag 0x0001): 8/16/24/32-bit
 *  - IEEE float (format tag 0x0003): 32/64-bit
 *
 * @param filename   The filename of the WAV that was loaded into memory
 * @param wav        Pointer to the start of the WAV file in memory
 * @param wav_bytes  Size in bytes of the input buffer
 * @param out_ptr    On success, receives pointer to a kmalloc'd buffer of
 *                   converted audio data. Caller must free with kfree().
 * @param out_bytes  On success, receives the size in bytes of *out_ptr
 * @return           true on successful conversion, false on error
 */
bool wav_from_memory(const char* filename, const void* wav, size_t wav_bytes, void** out_ptr, size_t* out_bytes);

/**
 * @brief Convert byte length of decoded WAV to number of samples
 *
 * Given the size in bytes of a converted 44.1 kHz stereo S16_LE buffer,
 * returns the number of stereo frames it contains. Each frame consists
 * of two interleaved 16-bit samples (left and right).
 *
 * @param length  Size in bytes of the decoded buffer
 * @return        Number of stereo frames
 */
size_t wav_size_to_samples(size_t length);

/**
 * @brief Convert number of samples to byte length of decoded WAV
 *
 * Given the number of stereo frames in a 44.1 kHz stereo S16_LE buffer,
 * returns the required size in bytes. Each frame consists of two
 * interleaved 16-bit samples (left and right).
 *
 * @param samples  Number of stereo frames
 * @return         Size in bytes of the decoded buffer
 */
size_t wav_samples_to_size(size_t samples);

bool has_suffix_icase(const char *str, const char *suffix);

/**
 * Register the WAV format in the list of file loaders
 */
void audio_init();

/**
 * @brief Load and convert an audio file into 44.1 kHz stereo S16_LE
 *
 * Opens a file from the filesystem, reads it into memory, and converts
 * its audio stream into 44.1 kHz, stereo, signed 16-bit little-endian
 * samples. The output is always interleaved left/right at the fixed rate
 * and format, regardless of the input encoding.
 *
 * By default, supports WAV files:
 *  - PCM integer (format tag 0x0001): 8/16/24/32-bit
 *  - IEEE float (format tag 0x0003): 32/64-bit
 * Plus, any extra formats implemented by loaded kernel modules.
 *
 * @param filename   Path to the WAV file
 * @param out_ptr    On success, receives pointer to a kmalloc'd buffer of
 *                   converted audio data. Caller must free with kfree().
 * @param out_bytes  On success, receives the size in bytes of *out_ptr
 * @return           true on successful conversion, false on error
 */
bool audio_file_load(const char *filename, void **out_ptr, size_t *out_bytes);

/**
 * @brief Register a new audio file loader
 *
 * Inserts the loader into the global loader chain so it can be used to
 * recognise and decode matching file formats (e.g. WAV, MP3, FLAC).
 * The caller retains ownership of the storage.
 *
 * @param loader Pointer to loader descriptor to register (must not be NULL)
 * @return true on success, false on failure
 */
bool register_audio_loader(audio_file_loader_t *loader);

/**
 * @brief Remove an audio file loader from the global chain
 *
 * Searches for the loader and unlinks it from the chain if present.
 * Safe to call on an already-removed loader.
 *
 * @param loader Pointer to loader previously passed to register_audio_loader
 * @return true if removed, false if not found
 */
bool deregister_audio_loader(audio_file_loader_t *loader);

/**
 * @brief Play a generated tone on a stream
 *
 * Pushes an ephemeral waveform (synthesised in software) directly into the
 * given stream, with optional envelope shaping.
 *
 * @param ctx        BASIC execution context (provides envelope table)
 * @param stream     Mixer stream handle to play into
 * @param freq_hz    Frequency in Hz of the tone
 * @param dur_cs     Duration in centiseconds (1/100 s)
 * @param env_idx_opt Envelope index 0–63, or -1 for raw tone with no envelope
 * @return true on success, false on error
 */
bool sound_cmd_tone(struct basic_ctx *ctx, mixer_stream_t *stream, uint32_t freq_hz, uint32_t dur_cs, int env_idx_opt);

/**
 * @brief Define a software envelope
 *
 * Stores a waveform and envelope shape into ctx->envelopes[idx], overwriting
 * any previous definition at that slot. Supports attack, decay, sustain,
 * release, vibrato, glide, and PWM modulation.
 *
 * @param ctx            BASIC execution context containing envelope table
 * @param idx            Envelope slot index 0–63
 * @param wave           Base waveform type (square, sine, saw, etc.)
 * @param volume         Base volume (0–255, Q8.0)
 * @param pulse_width    Duty cycle for square wave (128 = 50%)
 * @param attack_ms      Attack time in milliseconds
 * @param decay_ms       Decay time in milliseconds
 * @param sustain        Sustain level (0–255)
 * @param release_ms     Release time in milliseconds
 * @param vibrato_cents  Vibrato depth in cents (+/– pitch)
 * @param vibrato_hz     Vibrato frequency in Hz
 * @param glide_ms       Portamento/glide time in milliseconds
 * @param pwm_hz         Pulse-width modulation rate in Hz
 * @param pwm_depth      Pulse-width modulation depth (0–255)
 * @return true if defined successfully, false on invalid parameters
 */
bool envelope_define(struct basic_ctx *ctx, int idx, tone_wave_t wave, uint8_t volume, uint8_t pulse_width, uint32_t attack_ms, uint32_t decay_ms, uint8_t sustain,
	uint32_t release_ms, int32_t vibrato_cents, uint32_t vibrato_hz, uint32_t glide_ms, uint32_t pwm_hz, uint8_t pwm_depth);
