/*
 * histogram.h - Histograma e estatísticas de uma imagem em escala de cinza.
 *
 * Responsabilidades: contar os 256 níveis de intensidade, calcular média e
 * desvio padrão e classificar a imagem quanto ao brilho (escura/média/clara)
 * e ao contraste (baixo/médio/alto).
 *
 * Os limiares de classificação são convenções do projeto (o enunciado exige a
 * classificação, mas não fixa os valores). Ficam todos aqui, para ajustar em
 * um só lugar.
 */
#ifndef ENTRETONS_HISTOGRAM_H
#define ENTRETONS_HISTOGRAM_H

#include <stdint.h>

#include "image.h"

#define HISTOGRAM_LEVELS 256

/*
 * Brilho, pela média das intensidades (0..255), dividindo a faixa em terços:
 *   média <  85          -> escura
 *   85 <= média < 170    -> média
 *   média >= 170         -> clara
 */
#define BRIGHTNESS_DARK_BELOW 85.0
#define BRIGHTNESS_LIGHT_FROM 170.0

/*
 * Contraste, pelo desvio padrão das intensidades. O máximo teórico é 127,5
 * (metade dos pixels pretos, metade brancos) e uma imagem com todos os níveis
 * igualmente frequentes tem desvio de cerca de 74:
 *   desvio <  40         -> baixo
 *   40 <= desvio < 70    -> médio
 *   desvio >= 70         -> alto
 */
#define CONTRAST_LOW_BELOW 40.0
#define CONTRAST_HIGH_FROM 70.0

typedef enum {
    BRIGHTNESS_DARK,
    BRIGHTNESS_MEDIUM,
    BRIGHTNESS_LIGHT
} Brightness;

typedef enum {
    CONTRAST_LOW,
    CONTRAST_MEDIUM,
    CONTRAST_HIGH
} Contrast;

typedef struct {
    uint32_t bins[HISTOGRAM_LEVELS]; /* bins[k] = quantidade de pixels com intensidade k */
    uint32_t max_count;              /* maior valor de bins (escala das barras) */
    uint64_t total;                  /* total de pixels */
    double mean;                     /* média das intensidades */
    double std_dev;                  /* desvio padrão (populacional) */
    Brightness brightness;
    Contrast contrast;
} Histogram;

/* Calcula histograma, média, desvio padrão e classificações de img. */
void histogram_compute(Histogram *hist, const GrayImage *img);

Brightness histogram_classify_brightness(double mean);
Contrast histogram_classify_contrast(double std_dev);

/* Rótulos em português: "escura", "média", "clara" / "baixo", "médio", "alto". */
const char *histogram_brightness_label(Brightness b);
const char *histogram_contrast_label(Contrast c);

#endif /* ENTRETONS_HISTOGRAM_H */
