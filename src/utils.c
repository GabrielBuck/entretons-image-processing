#include "utils.h"

#include <SDL3/SDL.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

void utils_enable_utf8_console(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void utils_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[" APP_NAME "] ");
    vprintf(fmt, args);
    printf("\n");
    fflush(stdout);
    va_end(args);
}

void utils_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[" APP_NAME "] Erro: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(args);
}

int utils_clamp_int(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

uint8_t utils_to_u8(double v)
{
    v += 0.5; /* arredonda para o inteiro mais próximo */
    if (v <= 0.0) {
        return 0;
    }
    if (v >= 255.0) {
        return 255;
    }
    return (uint8_t)v;
}

const char *utils_basename(const char *path)
{
    const char *name = path;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    return name;
}

/* Conta caracteres UTF-8 (ignora bytes de continuação 10xxxxxx). */
static size_t utf8_length(const char *s)
{
    size_t n = 0;
    for (; *s != '\0'; s++) {
        if (((unsigned char)*s & 0xC0) != 0x80) {
            n++;
        }
    }
    return n;
}

/* Avança count caracteres UTF-8 a partir de s e devolve o novo ponteiro. */
static const char *utf8_advance(const char *s, size_t count)
{
    while (count > 0 && *s != '\0') {
        s++;
        while (((unsigned char)*s & 0xC0) == 0x80) {
            s++;
        }
        count--;
    }
    return s;
}

void utils_ellipsize_middle(char *dst, size_t dst_size, const char *src, size_t max_chars)
{
    if (dst_size == 0) {
        return;
    }
    size_t len = utf8_length(src);
    if (len <= max_chars || max_chars < 5) {
        snprintf(dst, dst_size, "%s", src);
        return;
    }

    size_t keep = max_chars - 3;      /* espaço restante além das reticências */
    size_t head = (keep + 1) / 2;
    size_t tail = keep - head;

    const char *head_end = utf8_advance(src, head);
    const char *tail_start = utf8_advance(src, len - tail);

    snprintf(dst, dst_size, "%.*s...%s", (int)(head_end - src), src, tail_start);
}

bool utils_is_regular_file(const char *path)
{
    SDL_PathInfo info;
    if (path == NULL || !SDL_GetPathInfo(path, &info)) {
        return false;
    }
    return info.type == SDL_PATHTYPE_FILE;
}

bool utils_find_asset(const char *relative_path, char *out, size_t out_size)
{
    const char *base = SDL_GetBasePath(); /* termina com separador; não deve ser liberado */
    char candidate[1024];

    /* 1) ao lado do executável; 2) uma pasta acima (ex.: executável em build/) */
    if (base != NULL) {
        snprintf(candidate, sizeof candidate, "%s%s", base, relative_path);
        if (utils_is_regular_file(candidate)) {
            snprintf(out, out_size, "%s", candidate);
            return true;
        }
        snprintf(candidate, sizeof candidate, "%s../%s", base, relative_path);
        if (utils_is_regular_file(candidate)) {
            snprintf(out, out_size, "%s", candidate);
            return true;
        }
    }

    /* 3) relativo ao diretório atual */
    if (utils_is_regular_file(relative_path)) {
        snprintf(out, out_size, "%s", relative_path);
        return true;
    }
    return false;
}
