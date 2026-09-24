/*
 * main.c - Ponto de entrada do Entretons.
 *
 * Nesta etapa: valida os argumentos e inicializa a SDL3.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <string.h>

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

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        utils_error("não foi possível inicializar a SDL: %s", SDL_GetError());
        return 3;
    }

    utils_info("SDL %d.%d.%d inicializada. Imagem: %s", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
               SDL_MICRO_VERSION, argv[1]);

    SDL_Quit();
    return 0;
}
