#include "image.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/* Escreve uma mensagem formatada em err, se err existir. */
static void set_error(char *err, size_t err_size, const char *fmt, ...)
{
    if (err == NULL || err_size == 0) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(err, err_size, fmt, args);
    va_end(args);
}

/* ------------------------------------------------------------------------- */
/* Criação, cópia e liberação                                                */
/* ------------------------------------------------------------------------- */

bool gray_image_create(GrayImage *img, int width, int height)
{
    img->width = 0;
    img->height = 0;
    img->pixels = NULL;

    if (width <= 0 || height <= 0) {
        return false;
    }
    size_t count = (size_t)width * (size_t)height;
    if (count > IMAGE_MAX_PIXELS) {
        return false;
    }
    img->pixels = calloc(count, 1);
    if (img->pixels == NULL) {
        return false;
    }
    img->width = width;
    img->height = height;
    return true;
}

bool gray_image_copy(GrayImage *dst, const GrayImage *src)
{
    if (src == NULL || src->pixels == NULL) {
        return false;
    }
    if (!gray_image_create(dst, src->width, src->height)) {
        return false;
    }
    memcpy(dst->pixels, src->pixels, (size_t)src->width * (size_t)src->height);
    return true;
}

void gray_image_free(GrayImage *img)
{
    free(img->pixels);
    img->pixels = NULL;
    img->width = 0;
    img->height = 0;
}

/* ------------------------------------------------------------------------- */
/* Carregamento                                                              */
/* ------------------------------------------------------------------------- */

/* true se todos os pixels têm R == G == B (imagem já em tons de cinza). */
static bool surface_is_gray(const SDL_Surface *rgba)
{
    for (int y = 0; y < rgba->h; y++) {
        const uint8_t *p = (const uint8_t *)rgba->pixels + (size_t)y * (size_t)rgba->pitch;
        for (int x = 0; x < rgba->w; x++, p += 4) {
            if (p[0] != p[1] || p[1] != p[2]) {
                return false;
            }
        }
    }
    return true;
}

/*
 * Preenche out a partir de uma superfície RGBA32. Se is_gray, copia o canal R;
 * caso contrário aplica Y = 0,2125 R + 0,7154 G + 0,0721 B.
 */
static void surface_to_gray(const SDL_Surface *rgba, bool is_gray, GrayImage *out)
{
    for (int y = 0; y < rgba->h; y++) {
        const uint8_t *p = (const uint8_t *)rgba->pixels + (size_t)y * (size_t)rgba->pitch;
        uint8_t *dst = out->pixels + (size_t)y * (size_t)out->width;
        for (int x = 0; x < rgba->w; x++, p += 4) {
            if (is_gray) {
                dst[x] = p[0];
            } else {
                dst[x] = utils_to_u8(LUMA_R * p[0] + LUMA_G * p[1] + LUMA_B * p[2]);
            }
        }
    }
}

bool image_load(const char *path, GrayImage *out, ImageInfo *info, char *err, size_t err_size)
{
    out->width = 0;
    out->height = 0;
    out->pixels = NULL;

    if (path == NULL || path[0] == '\0') {
        set_error(err, err_size, "caminho da imagem vazio.");
        return false;
    }

    SDL_PathInfo path_info;
    if (!SDL_GetPathInfo(path, &path_info)) {
        set_error(err, err_size, "arquivo não encontrado: \"%s\".", path);
        return false;
    }
    if (path_info.type != SDL_PATHTYPE_FILE) {
        set_error(err, err_size, "o caminho não é um arquivo de imagem: \"%s\".", path);
        return false;
    }

    SDL_Surface *loaded = IMG_Load(path);
    if (loaded == NULL) {
        set_error(err, err_size,
                  "não foi possível abrir \"%s\" (formato não suportado ou arquivo corrompido): %s",
                  path, SDL_GetError());
        return false;
    }

    if (loaded->w <= 0 || loaded->h <= 0) {
        SDL_DestroySurface(loaded);
        set_error(err, err_size, "a imagem \"%s\" não possui pixels.", path);
        return false;
    }
    if ((size_t)loaded->w * (size_t)loaded->h > IMAGE_MAX_PIXELS) {
        set_error(err, err_size, "a imagem \"%s\" é grande demais (%d x %d; máximo de %lu milhões de pixels).",
                  path, loaded->w, loaded->h, (unsigned long)(IMAGE_MAX_PIXELS / 1000000));
        SDL_DestroySurface(loaded);
        return false;
    }

    /* Padroniza para RGBA32 (como nos exemplos da disciplina) antes de ler os pixels. */
    SDL_Surface *rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (rgba == NULL) {
        set_error(err, err_size, "não foi possível converter a imagem para RGBA32: %s", SDL_GetError());
        return false;
    }

    if (!gray_image_create(out, rgba->w, rgba->h)) {
        SDL_DestroySurface(rgba);
        set_error(err, err_size, "memória insuficiente para a imagem (%d x %d).", rgba->w, rgba->h);
        return false;
    }

    bool is_gray = surface_is_gray(rgba);
    surface_to_gray(rgba, is_gray, out);

    if (info != NULL) {
        info->width = out->width;
        info->height = out->height;
        info->was_color = !is_gray;
    }

    SDL_DestroySurface(rgba);
    return true;
}
