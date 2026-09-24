#include "histogram.h"

#include <math.h>
#include <string.h>

Brightness histogram_classify_brightness(double mean)
{
    if (mean < BRIGHTNESS_DARK_BELOW) {
        return BRIGHTNESS_DARK;
    }
    if (mean < BRIGHTNESS_LIGHT_FROM) {
        return BRIGHTNESS_MEDIUM;
    }
    return BRIGHTNESS_LIGHT;
}

Contrast histogram_classify_contrast(double std_dev)
{
    if (std_dev < CONTRAST_LOW_BELOW) {
        return CONTRAST_LOW;
    }
    if (std_dev < CONTRAST_HIGH_FROM) {
        return CONTRAST_MEDIUM;
    }
    return CONTRAST_HIGH;
}

const char *histogram_brightness_label(Brightness b)
{
    switch (b) {
    case BRIGHTNESS_DARK:
        return "escura";
    case BRIGHTNESS_MEDIUM:
        return "média";
    case BRIGHTNESS_LIGHT:
        return "clara";
    }
    return "?";
}

const char *histogram_contrast_label(Contrast c)
{
    switch (c) {
    case CONTRAST_LOW:
        return "baixo";
    case CONTRAST_MEDIUM:
        return "médio";
    case CONTRAST_HIGH:
        return "alto";
    }
    return "?";
}

void histogram_compute(Histogram *hist, const GrayImage *img)
{
    memset(hist, 0, sizeof *hist);
    if (img == NULL || img->pixels == NULL) {
        return;
    }

    const size_t count = (size_t)img->width * (size_t)img->height;
    for (size_t i = 0; i < count; i++) {
        hist->bins[img->pixels[i]]++;
    }
    hist->total = count;

    /* Média e variância a partir das contagens (256 iterações em vez de N). */
    double sum = 0.0;
    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        if (hist->bins[k] > hist->max_count) {
            hist->max_count = hist->bins[k];
        }
        sum += (double)k * (double)hist->bins[k];
    }
    hist->mean = sum / (double)count;

    double variance = 0.0;
    for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
        double d = (double)k - hist->mean;
        variance += d * d * (double)hist->bins[k];
    }
    hist->std_dev = sqrt(variance / (double)count);

    hist->brightness = histogram_classify_brightness(hist->mean);
    hist->contrast = histogram_classify_contrast(hist->std_dev);
}
