#include "equalization.h"

#include <string.h>

#include "utils.h"

/*
 * s(k) = round((L - 1) * cdf(k) / (M * N)), como visto em aula (Gonzalez &
 * Woods): a CDF acumulada da distribuição de probabilidades, escalada para
 * 0..255. Não garante ocupar toda a faixa 0..255 para qualquer histograma
 * (isso exigiria normalizar pelo cdf_min), mas é a fórmula ensinada.
 */
void equalization_build_lut(const Histogram *hist, uint8_t lut[HISTOGRAM_LEVELS])
{
    uint64_t cdf[HISTOGRAM_LEVELS];
    uint64_t running = 0;
    int levels_present = 0;

    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        running += hist->bins[k];
        cdf[k] = running;
        if (hist->bins[k] > 0) {
            levels_present++;
        }
    }

    /* Sem pixels ou com um único nível de cinza não há o que equalizar: identidade. */
    if (hist->total == 0 || levels_present <= 1) {
        for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
            lut[k] = (uint8_t)k;
        }
        return;
    }

    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        double s = 255.0 * (double)cdf[k] / (double)hist->total;
        lut[k] = utils_to_u8(s);
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
