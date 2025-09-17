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

/** @brief Maximum length (including NUL) for an audio device’s display name. */
#define MAX_AUDIO_DEVICE_NAME 32

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

	/** Next device in the global list. */
	struct audio_device_t *next;
} audio_device_t;

/**
 * @brief Register a new audio device.
 *
 * Inserts @p newdev into the global device list. The caller retains ownership
 * of the storage; the struct must remain valid for the lifetime of the device.
 *
 * @param newdev Pointer to the device descriptor to register (must not be NULL).
 * @return true on success, false on failure.
 */
bool register_audio_device(audio_device_t *newdev);

/**
 * @brief Find a registered audio device by name.
 *
 * @param name NUL-terminated device name to search for.
 * @return Pointer to the matching device, or NULL if not found.
 */
audio_device_t *find_audio_device(const char *name);

/**
 * @brief Get the first registered audio device, if any.
 * @return Pointer to the first device in the list, or NULL if none.
 */
audio_device_t *find_first_audio_device(void);
