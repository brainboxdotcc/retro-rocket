/**
 * @file visualizer.c
 * @brief Audio spectrum analyser
 */
#include <kernel.h>
#include <visualiser.h>
#include <emmintrin.h>

#define FFT_SIZE 512

static double fft_real[FFT_SIZE];
static double fft_imag[FFT_SIZE];
static double fft_window[FFT_SIZE];

static uint16_t visualiser_bands[VISUALISER_BANDS];
static uint32_t fft_fill;

void fft_init(void) {
	for (uint32_t i = 0; i < FFT_SIZE; i++) {
		fft_window[i] = 0.5 * (1.0 - cos((2.0 * M_PI * i) / (FFT_SIZE - 1)));
	}
}

static void fft_run(void) {
	static const uint16_t band_edges[VISUALISER_BANDS + 1] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 10, 12, 14, 16, 19, 22, 26,
		30, 35, 40, 46, 53, 61, 70, 80,
		91, 103, 117, 132, 148, 166, 186, 208,
		255
	};

	uint32_t j = 0;

	for (uint32_t i = 0; i < FFT_SIZE; i++) {
		if (i < j) {
			double tr = fft_real[i];
			double ti = fft_imag[i];

			fft_real[i] = fft_real[j];
			fft_imag[i] = fft_imag[j];

			fft_real[j] = tr;
			fft_imag[j] = ti;
		}

		uint32_t bit = FFT_SIZE >> 1;

		while (j & bit) {
			j ^= bit;
			bit >>= 1;
		}

		j ^= bit;
	}

	for (uint32_t len = 2; len <= FFT_SIZE; len <<= 1) {
		double angle = -2.0 * M_PI / (double)len;

		double wlen_r = cos(angle);
		double wlen_i = sin(angle);

		for (uint32_t i = 0; i < FFT_SIZE; i += len) {
			double wr = 1.0;
			double wi = 0.0;

			for (uint32_t j = 0; j < len / 2; j++) {
				uint32_t even = i + j;
				uint32_t odd = even + (len / 2);

				double ur = fft_real[even];
				double ui = fft_imag[even];

				double vr = (fft_real[odd] * wr) - (fft_imag[odd] * wi);
				double vi = (fft_real[odd] * wi) + (fft_imag[odd] * wr);

				fft_real[even] = ur + vr;
				fft_imag[even] = ui + vi;

				fft_real[odd] = ur - vr;
				fft_imag[odd] = ui - vi;

				double next_wr = (wr * wlen_r) - (wi * wlen_i);

				wi = (wr * wlen_i) + (wi * wlen_r);
				wr = next_wr;
			}
		}
	}

	for (uint32_t b = 0; b < VISUALISER_BANDS; b++) {
		uint32_t start = band_edges[b];
		uint32_t end = band_edges[b + 1];

		double peak = 0.0;

		for (uint32_t i = start; i < end; i++) {
			double mag = sqrt((fft_real[i] * fft_real[i]) + (fft_imag[i] * fft_imag[i]));

			mag /= 32.0;

			if (mag > peak) {
				peak = mag;
			}
		}

		uint16_t level = (uint16_t)(sqrt(peak) * 512.0);

		if (level > visualiser_bands[b]) {
			visualiser_bands[b] = level;
		} else {
			if (visualiser_bands[b] > 12) {
				visualiser_bands[b] -= 12;
			} else {
				visualiser_bands[b] = 0;
			}
		}
	}
}

void visualiser_process(const int16_t *samples, uint32_t frames)
{
	for (uint32_t i = 0; i + 1 < frames; i += 2) {
		__m128i s16 = _mm_loadu_si128((const __m128i *)(samples + (i * 2)));
		__m128i lo32 = _mm_unpacklo_epi16(s16, _mm_srai_epi16(s16, 15));

		int32_t vals[4];
		_mm_storeu_si128((__m128i *)vals, lo32);
		double mono0 = ((double)vals[0] + (double)vals[1]) / 65536.0;
		double mono1 = ((double)vals[2] + (double)vals[3]) / 65536.0;
		__m128d mono = _mm_set_pd(mono1, mono0);
		__m128d window = _mm_loadu_pd(&fft_window[fft_fill]);
		mono = _mm_mul_pd(mono, window);

		double out[2];
		_mm_storeu_pd(out, mono);
		fft_real[fft_fill] = out[0];
		fft_imag[fft_fill] = 0.0;
		fft_fill++;
		fft_real[fft_fill] = out[1];
		fft_imag[fft_fill] = 0.0;
		fft_fill++;
		if (fft_fill >= FFT_SIZE) {
			fft_run();
			fft_fill = 0;
		}
	}
	for (uint32_t i = 0; i < frames; i++) {
		int16_t left = samples[i * 2];
		int16_t right = samples[i * 2 + 1];
		double mono = ((double)left + (double)right) / 65536.0;
		fft_real[fft_fill] = mono * fft_window[fft_fill];
		fft_imag[fft_fill] = 0.0;
		fft_fill++;
		if (fft_fill >= FFT_SIZE) {
			fft_run();
			fft_fill = 0;
		}
	}
}

uint16_t visualiser_get_band(uint32_t band)
{
	if (band >= VISUALISER_BANDS) {
		return 0;
	}

	return visualiser_bands[band];
}
