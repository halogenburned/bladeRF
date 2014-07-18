/*
 * This file is part of the bladeRF project:
 *   http://www.github.com/nuand/bladeRF
 *
 * Copyright (C) 2014 Nuand LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "host_config.h"
#include "test_ctrl.h"

static int set_and_check(struct bladerf *dev, bladerf_module m,
                         unsigned int rate)
{
    int status;
    unsigned int actual, readback;

    status = bladerf_set_sample_rate(dev, m, rate, &actual);
    if (status != 0) {
        fprintf(stderr, "Failed to set sample rate: %s\n",
                bladerf_strerror(status));
        return status;
    }

    status = bladerf_get_sample_rate(dev, m, &readback);
    if (status != 0) {
        fprintf(stderr, "Failed to read back sample rate: %s\n",
                bladerf_strerror(status));
        return status;
    }

    return 0;
}

static int set_and_check_rational(struct bladerf *dev, bladerf_module m,
                                  struct bladerf_rational_rate *rate)
{
    int status;
    struct bladerf_rational_rate actual, readback;

    status = bladerf_set_rational_sample_rate(dev, m, rate, &actual);
    if (status != 0) {
        fprintf(stderr, "Failed to set rational sample rate: %s\n",
                bladerf_strerror(status));
        return status;
    }

    status = bladerf_get_rational_sample_rate(dev, m, &readback);
    if (status != 0) {
        fprintf(stderr, "Failed to read back rational sample rate: %s\n",
                bladerf_strerror(status));
        return status;
    }

    if (actual.integer != readback.integer  ||
        actual.num != readback.num          ||
        actual.den != readback.den          ) {

        fprintf(stderr,
                "Readback mismatch:\n"
                " actual:   int=%"PRIu64" num=%"PRIu64" den=%"PRIu64"\n"
                "  readback: int=%"PRIu64" num=%"PRIu64" den=%"PRIu64"\n",
                actual.integer, actual.num, actual.den,
                readback.integer, readback.num, readback.den);

        return status;
    }

    return 0;
}

static int sweep_samplerate(struct bladerf *dev, bladerf_module m)
{
    int status;
    unsigned int rate;
    const unsigned int inc = 10000;
    unsigned int n;
    unsigned int failures = 0;

    for (rate = BLADERF_SAMPLERATE_MIN, n = 0;
         rate <= BLADERF_SAMPLERATE_REC_MAX;
         rate += inc, n++) {

        status = set_and_check(dev, m, rate);
        if (status != 0) {
            failures++;
        } else if (n % 50 == 0) {
            printf("\r  Sample rate currently set to %-10u Hz...", rate);
            fflush(stdout);
        }
    }

    printf("\n");
    fflush(stdout);
    return failures;
}

static int random_samplerates(struct bladerf *dev, bladerf_module m)
{
    int status;
    unsigned int i, n;
    const unsigned int interations = 2500;
    unsigned int rate, scaling;
    unsigned failures = 0;

    scaling = BLADERF_SAMPLERATE_REC_MAX / RAND_MAX;
    if (scaling == 0) {
        scaling = 1;
    }

    for (i = n = 0; i < interations; i++, n++) {
        rate = BLADERF_SAMPLERATE_MIN +
                ((scaling * rand()) % BLADERF_SAMPLERATE_REC_MAX);

        if (rate < BLADERF_SAMPLERATE_MIN) {
            rate = BLADERF_SAMPLERATE_MIN;
        } else if (rate > BLADERF_SAMPLERATE_REC_MAX) {
            rate = BLADERF_SAMPLERATE_REC_MAX;
        }

        status = set_and_check(dev, m, rate);
        if (status != 0) {
            failures++;
        } else if (n % 50 == 0) {
            printf("\r  Sample rate currently set to %-10u Hz...", rate);
            fflush(stdout);
        }
    }

    printf("\n");
    fflush(stdout);
    return failures;
}

static int random_rational_samplerates(struct bladerf *dev, bladerf_module m)
{
    int status;
    unsigned int i, n;
    const unsigned int iterations = 2500;
    struct bladerf_rational_rate rate;
    unsigned int scaling;
    unsigned failures = 0;

    scaling = BLADERF_SAMPLERATE_REC_MAX / RAND_MAX;
    if (scaling == 0) {
        scaling = 1;
    }

    for (i = n = 0; i < iterations; i++, n++) {

        rate.integer = BLADERF_SAMPLERATE_MIN +
                ((scaling * rand()) % BLADERF_SAMPLERATE_REC_MAX);

        if (rate.integer < BLADERF_SAMPLERATE_MIN) {
            rate.integer = BLADERF_SAMPLERATE_MIN;
        } else if (rate.integer > BLADERF_SAMPLERATE_REC_MAX) {
            rate.integer = BLADERF_SAMPLERATE_REC_MAX;
        }

        if (rate.integer != BLADERF_SAMPLERATE_REC_MAX) {
            rate.num = rand();
            rate.den = rand();

            if (rate.den == 0) {
                rate.den = 1;
            }


        } else {
            rate.num = 0;
            rate.den = 1;
        }

        status = set_and_check_rational(dev, m, &rate);
        if (status != 0) {
            failures++;
        } else if (n % 50 == 0) {
            printf("\r  Sample rate currently set to "
                   "%-10"PRIu64" %-10"PRIu64"/%-10"PRIu64" Hz...",
                   rate.integer, rate.num, rate.den);
            fflush(stdout);
        }
    }

    printf("\n");
    fflush(stdout);
    return failures;
}

int test_samplerate(struct bladerf *dev, struct app_params *p)
{
    unsigned int failures = 0;

    printf("%s: Sweeping RX sample rates...\n", __FUNCTION__);
    failures += sweep_samplerate(dev, BLADERF_MODULE_RX);

    printf("%s: Applying random RX sample rates...\n", __FUNCTION__);
    failures += random_samplerates(dev, BLADERF_MODULE_RX);

    printf("%s: Applying random RX rational sample rates...\n", __FUNCTION__);
    failures += random_rational_samplerates(dev, BLADERF_MODULE_RX);

    printf("%s: Sweeping TX sample rates...\n", __FUNCTION__);
    failures += sweep_samplerate(dev, BLADERF_MODULE_TX);

    printf("%s: Applying random TX sample rates...\n", __FUNCTION__);
    failures += random_samplerates(dev, BLADERF_MODULE_TX);

    printf("%s: Applying random TX rational sample rates...\n", __FUNCTION__);
    failures += random_rational_samplerates(dev, BLADERF_MODULE_TX);

    return failures;
}
