/*
 * main.c - Ponto de entrada do Entretons.
 *
 * Nesta etapa: valida os argumentos, carrega a imagem, informa no terminal
 * se ela era colorida (convertida para cinza) ou já estava em cinza e mostra
 * as estatísticas do histograma.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <string.h>

#include "histogram.h"
#include "image.h"
#include "utils.h"

static void print_usage(const char *program)
{
    printf("Uso: %s caminho_da_imagem.ext\n", program);
    printf("Exemplo: %s assets/samples/paisagem_cores.png\n", program);
}

int main(int argc, char *argv[])
{
    utils_enable_utf8_console();

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc != 2) {
        utils_error("informe exatamente um caminho de imagem.");
        print_usage(argv[0]);
        return 1;
    }

    GrayImage gray = {0};
    ImageInfo info = {0};
    char error[512];

    if (!image_load(argv[1], &gray, &info, error, sizeof error)) {
        utils_error("%s", error);
        return 2;
    }

    utils_info("Imagem \"%s\": %d x %d pixels.", utils_basename(argv[1]), info.width, info.height);
    utils_info(info.was_color ? "Imagem colorida: convertida para escala de cinza."
                              : "Imagem já está em escala de cinza.");

    Histogram hist;
    histogram_compute(&hist, &gray);
    utils_info("Histograma: média %.2f, desvio padrão %.2f -> imagem %s, contraste %s.", hist.mean,
               hist.std_dev, histogram_brightness_label(hist.brightness),
               histogram_contrast_label(hist.contrast));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        utils_error("não foi possível inicializar a SDL: %s", SDL_GetError());
        gray_image_free(&gray);
        return 3;
    }

    SDL_Quit();
    gray_image_free(&gray);
    return 0;
}
