/**
 * @file visualizer.h
 * @brief Audio spectrum analyser
 */
#pragma once

#include <kernel.h>

/**
 * @brief Number of spectrum bands exposed by the visualiser.
 */
#define VISUALISER_BANDS 32

/**
 * @brief Initialise the audio spectrum analyser.
 *
 * Precomputes FFT windowing tables and resets analyser state.
 * Must be called before visualiser_process().
 */
void fft_init(void);

/**
 * @brief Process a block of stereo PCM audio samples.
 *
 * Consumes signed 16-bit interleaved stereo frames from the
 * mixer output and updates the internal spectrum bands.
 *
 * @param samples Interleaved stereo PCM samples.
 * @param frames Number of stereo frames in the buffer.
 */
void visualiser_process(const int16_t *samples, uint32_t frames);

/**
 * @brief Retrieve the current value of a spectrum band.
 *
 * @param band Band index from 0 to VISUALISER_BANDS - 1.
 * @return Current band intensity value.
 */
uint16_t visualiser_get_band(uint32_t band);