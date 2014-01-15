/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2014, Steven Noonan <steven@uplinklabs.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "prefix.h"
#include "clock.h"

#include <math.h>
#include <time.h>

static uint32_t cycles_per_usec;

static uint64_t wallclock_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

static uint32_t get_cycles_per_usec(void)
{
	uint64_t wc_s, wc_e;
	uint64_t c_s, c_e;

	wc_s = wallclock_ns();
	c_s = get_cpu_clock();
	do {
		uint64_t elapsed;

		wc_e = wallclock_ns();
		elapsed = wc_e - wc_s;
		if (elapsed >= 1280000ULL) {
			c_e = get_cpu_clock();
			break;
		}
	} while (1);

	return (c_e - c_s + 127) >> 7;
}

static void calibrate_cpu_clock(void)
{
	const int NR_TIME_ITERS = 10;
	double delta, mean, S;
	uint32_t avg, cycles[NR_TIME_ITERS];
	int i, samples;

	cycles[0] = get_cycles_per_usec();
	S = delta = mean = 0.0;
	for (i = 0; i < NR_TIME_ITERS; i++) {
		cycles[i] = get_cycles_per_usec();
		delta = cycles[i] - mean;
		if (delta) {
			mean += delta / (i + 1.0);
			S += delta * (cycles[i] - mean);
		}
	}

	S = sqrt(S / (NR_TIME_ITERS - 1.0));

	samples = avg = 0;
	for (i = 0; i < NR_TIME_ITERS; i++) {
		double this = cycles[i];

		if ((fmax(this, mean) - fmin(this, mean)) > S)
			continue;
		samples++;
		avg += this;
	}

	S /= (double)NR_TIME_ITERS;
	mean /= 10.0;

	avg /= samples;
	avg = (avg + 9) / 10;

	cycles_per_usec = avg;
}

uint64_t cpu_clock_to_wall(uint64_t clock)
{
	if (!cycles_per_usec)
		init_cpu_clock();
	return (clock * 1000ULL) / cycles_per_usec;
}

void init_cpu_clock(void)
{
	calibrate_cpu_clock();
}
