/*
 * equalization.h - Equalização de histograma e controle de estado original/equalizada.
 *
 * Equalização clássica de histograma: a função de distribuição acumulada (CDF)
 * das intensidades vira a curva de transformação,
 *
 *     s(k) = round( 255 * (cdf(k) - cdf_min) / (N - cdf_min) )
 *
 * onde N é o total de pixels e cdf_min o menor valor não nulo da CDF (a
 * intensidade mais escura presente na imagem vira 0 e a mais clara, 255).
 *
 * A imagem original nunca é alterada: a versão equalizada é gerada uma única
 * vez e guardada. Alternar entre as duas não recarrega o arquivo.
 */
#ifndef ENTRETONS_EQUALIZATION_H
#define ENTRETONS_EQUALIZATION_H

#include <stdbool.h>
#include <stdint.h>

#include "histogram.h"
#include "image.h"

/* Calcula a tabela de transformação (256 entradas) a partir do histograma. */
void equalization_build_lut(const Histogram *hist, uint8_t lut[HISTOGRAM_LEVELS]);

/* Gera em dst a versão equalizada de src. dst deve estar vazia. */
bool equalization_apply(const GrayImage *src, GrayImage *dst);

/* Original em cinza + versão equalizada (gerada sob demanda) + qual está ativa. */
typedef struct {
    GrayImage original;
    GrayImage equalized;
    bool equalized_ready;
    bool active; /* true: a versão equalizada é a imagem atual */
} EqualizationState;

/* Assume a posse de *original (que fica zerada); começa com a imagem original ativa. */
void equalization_state_init(EqualizationState *state, GrayImage *original);

/*
 * Alterna entre original e equalizada. A equalização é calculada na primeira
 * vez que é pedida. Retorna false (e mantém o estado) se faltar memória.
 */
bool equalization_toggle(EqualizationState *state);

/* Imagem atualmente ativa (original ou equalizada). */
const GrayImage *equalization_current(const EqualizationState *state);

void equalization_state_free(EqualizationState *state);

#endif /* ENTRETONS_EQUALIZATION_H */
