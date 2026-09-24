/*
 * image.h - Imagem em escala de cinza e operações de arquivo.
 *
 * Responsabilidades: carregar e validar a imagem, detectar se ela é colorida
 * ou já está em escala de cinza, converter para cinza e copiar.
 */
#ifndef ENTRETONS_IMAGE_H
#define ENTRETONS_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Limite de segurança: 64 milhões de pixels (ex.: 8000 x 8000). */
#define IMAGE_MAX_PIXELS ((size_t)64 * 1000 * 1000)

/* Coeficientes da conversão RGB -> cinza: Y = 0,2125 R + 0,7154 G + 0,0721 B. */
#define LUMA_R 0.2125
#define LUMA_G 0.7154
#define LUMA_B 0.0721

/* Imagem de 8 bits por pixel (0 = preto, 255 = branco), linhas contíguas, sem padding. */
typedef struct {
    int width;
    int height;
    uint8_t *pixels; /* width * height bytes; NULL quando vazia */
} GrayImage;

/* Informações sobre o arquivo carregado. */
typedef struct {
    int width;
    int height;
    bool was_color; /* true: a imagem original era colorida e foi convertida */
} ImageInfo;

/* Aloca uma imagem width x height zerada. Retorna false se os parâmetros forem inválidos ou faltar memória. */
bool gray_image_create(GrayImage *img, int width, int height);

/* Copia src em dst (dst é alocada; não pode conter uma imagem anterior sem gray_image_free). */
bool gray_image_copy(GrayImage *dst, const GrayImage *src);

/* Libera os pixels e zera a estrutura. Aceita imagem já vazia. */
void gray_image_free(GrayImage *img);

/*
 * Carrega o arquivo, valida (existência, formato, dimensões), detecta se é
 * colorido e converte para escala de cinza quando necessário. A transparência
 * (canal alfa), se houver, é ignorada.
 *
 * Em caso de erro retorna false e escreve uma mensagem em err (se não for NULL).
 */
bool image_load(const char *path, GrayImage *out, ImageInfo *info, char *err, size_t err_size);

#endif /* ENTRETONS_IMAGE_H */
