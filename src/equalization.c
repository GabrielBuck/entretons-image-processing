#include "equalization.h"

#include <string.h>

#include "utils.h"

void equalization_build_lut(const Histogram *hist, uint8_t lut[HISTOGRAM_LEVELS])
{
    /* Sem pixels ou com um único nível de cinza não há o que equalizar: identidade. */
    uint64_t cdf_min = 0;
    uint64_t cdf[HISTOGRAM_LEVELS];
    uint64_t running = 0;

    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        running += hist->bins[k];
        cdf[k] = running;
        if (cdf_min == 0 && running > 0) {
            cdf_min = running;
        }
    }

    const uint64_t denominator = hist->total - cdf_min;
    if (hist->total == 0 || denominator == 0) {
        for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
            lut[k] = (uint8_t)k;
        }
        return;
    }

    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        if (cdf[k] <= cdf_min) {
            lut[k] = 0; /* níveis abaixo do menor presente e o próprio menor nível */
        } else {
            double s = 255.0 * (double)(cdf[k] - cdf_min) / (double)denominator;
            lut[k] = utils_to_u8(s);
        }
    }
}

bool equalization_apply(const GrayImage *src, GrayImage *dst)
{
    if (src == NULL || src->pixels == NULL) {
        return false;
    }

    Histogram hist;
    histogram_compute(&hist, src);

    uint8_t lut[HISTOGRAM_LEVELS];
    equalization_build_lut(&hist, lut);

    if (!gray_image_create(dst, src->width, src->height)) {
        return false;
    }
    const size_t count = (size_t)src->width * (size_t)src->height;
    for (size_t i = 0; i < count; i++) {
        dst->pixels[i] = lut[src->pixels[i]];
    }
    return true;
}

void equalization_state_init(EqualizationState *state, GrayImage *original)
{
    memset(state, 0, sizeof *state);
    state->original = *original;
    original->width = 0;
    original->height = 0;
    original->pixels = NULL;
}

bool equalization_toggle(EqualizationState *state)
{
    if (!state->active && !state->equalized_ready) {
        if (!equalization_apply(&state->original, &state->equalized)) {
            return false;
        }
        state->equalized_ready = true;
    }
    state->active = !state->active;
    return true;
}

const GrayImage *equalization_current(const EqualizationState *state)
{
    return state->active ? &state->equalized : &state->original;
}

void equalization_state_free(EqualizationState *state)
{
    gray_image_free(&state->original);
    gray_image_free(&state->equalized);
    state->equalized_ready = false;
    state->active = false;
}
