/*
 * test_core.c - Testes automáticos da lógica do Entretons (sem abrir janelas).
 *
 * Cobrem: conversão para cinza, detecção colorida/cinza, tratamento de erros
 * de carregamento, histograma e classificação, equalização (inclusive restaurar
 * a original), redimensionamento, salvamento em PNG e funções de utils.
 *
 * Execute com:  make test
 * (rode a partir da pasta do projeto: alguns testes usam assets/samples)
 */
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "equalization.h"
#include "histogram.h"
#include "image.h"
#include "utils.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond)                                                                              \
    do {                                                                                         \
        tests_run++;                                                                             \
        if (!(cond)) {                                                                           \
            tests_failed++;                                                                      \
            printf("  FALHOU (%s:%d): %s\n", __FILE__, __LINE__, #cond);                          \
        }                                                                                        \
    } while (0)

#define CHECK_NEAR(a, b, tol) CHECK(fabs((double)(a) - (double)(b)) <= (tol))

#define TMP_PNG "test_tmp_image.png"

/* ---- utilidades dos testes ------------------------------------------------ */

/* Grava uma imagem RGB (3 bytes por pixel) em PNG. */
static bool write_rgb_png(const char *path, int w, int h, const uint8_t *rgb)
{
    SDL_Surface *s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGB24);
    if (s == NULL) {
        return false;
    }
    for (int y = 0; y < h; y++) {
        memcpy((uint8_t *)s->pixels + (size_t)y * (size_t)s->pitch, rgb + (size_t)y * (size_t)w * 3,
               (size_t)w * 3);
    }
    bool ok = IMG_SavePNG(s, path);
    SDL_DestroySurface(s);
    return ok;
}

static void make_gray(GrayImage *img, int w, int h, const uint8_t *values)
{
    gray_image_create(img, w, h);
    memcpy(img->pixels, values, (size_t)w * (size_t)h);
}

/* ---- testes --------------------------------------------------------------- */

static void test_luma_formula(void)
{
    puts("conversão RGB -> cinza (Y = 0,2125R + 0,7154G + 0,0721B)");
    const uint8_t rgb[] = {
        255, 0,   0,   /* -> 54  */
        0,   255, 0,   /* -> 182 */
        0,   0,   255, /* -> 18  */
        255, 255, 255, /* -> 255 */
        0,   0,   0,   /* -> 0   */
        100, 150, 200, /* -> 143 */
    };
    const uint8_t expected[] = {54, 182, 18, 255, 0, 143};

    CHECK(write_rgb_png(TMP_PNG, 6, 1, rgb));
    GrayImage img;
    ImageInfo info;
    char err[256];
    CHECK(image_load(TMP_PNG, &img, &info, err, sizeof err));
    CHECK(info.was_color);
    CHECK(img.width == 6 && img.height == 1);
    for (int i = 0; i < 6; i++) {
        CHECK(img.pixels[i] == expected[i]);
    }
    gray_image_free(&img);
    SDL_RemovePath(TMP_PNG);
}

static void test_gray_detection(void)
{
    puts("detecção de imagem colorida x escala de cinza");
    GrayImage img;
    ImageInfo info;
    char err[256];

    /* R = G = B em todos os pixels: já é cinza, valores preservados sem conversão */
    const uint8_t gray_rgb[] = {10, 10, 10, 200, 200, 200, 255, 255, 255, 0, 0, 0};
    CHECK(write_rgb_png(TMP_PNG, 4, 1, gray_rgb));
    CHECK(image_load(TMP_PNG, &img, &info, err, sizeof err));
    CHECK(!info.was_color);
    CHECK(img.pixels[0] == 10 && img.pixels[1] == 200 && img.pixels[2] == 255 && img.pixels[3] == 0);
    gray_image_free(&img);

    /* um único pixel com canal diferente basta para ser colorida */
    const uint8_t almost_gray[] = {10, 10, 10, 200, 200, 201, 255, 255, 255, 0, 0, 0};
    CHECK(write_rgb_png(TMP_PNG, 4, 1, almost_gray));
    CHECK(image_load(TMP_PNG, &img, &info, err, sizeof err));
    CHECK(info.was_color);
    gray_image_free(&img);
    SDL_RemovePath(TMP_PNG);
}

static void test_load_errors(void)
{
    puts("erros de carregamento");
    GrayImage img;
    ImageInfo info;
    char err[256];

    err[0] = '\0';
    CHECK(!image_load("arquivo_que_nao_existe.png", &img, &info, err, sizeof err));
    CHECK(err[0] != '\0');
    CHECK(img.pixels == NULL);

    err[0] = '\0';
    CHECK(!image_load(".", &img, &info, err, sizeof err)); /* diretório */
    CHECK(err[0] != '\0');

    err[0] = '\0';
    CHECK(!image_load("Makefile", &img, &info, err, sizeof err)); /* não é imagem */
    CHECK(err[0] != '\0');

    CHECK(!image_load("", &img, &info, err, sizeof err));
    CHECK(!image_load(NULL, &img, &info, err, sizeof err));
}

static void test_sample_images(void)
{
    puts("imagens de exemplo (assets/samples)");
    GrayImage img;
    ImageInfo info;
    char err[256];

    if (!utils_is_regular_file("assets/samples/cinza_equilibrada.png")) {
        puts("  (ignorado: execute a partir da pasta do projeto)");
        return;
    }

    /*
     * PNG cinza de 8 bits: os níveis não podem mudar (regressão do deslocamento
     * de paleta da SDL_image, em que 255 virava 254). Valores de referência
     * calculados fora do programa (numpy).
     */
    CHECK(image_load("assets/samples/cinza_equilibrada.png", &img, &info, err, sizeof err));
    CHECK(!info.was_color);
    CHECK(img.width == 800 && img.height == 600);
    Histogram h;
    histogram_compute(&h, &img);
    CHECK(h.bins[0] > 0 && h.bins[255] > 0);
    CHECK_NEAR(h.mean, 92.70, 0.01);
    CHECK_NEAR(h.std_dev, 54.04, 0.01);
    gray_image_free(&img);

    /* imagem colorida: fórmula do enunciado aplicada a todos os pixels */
    CHECK(image_load("assets/samples/paisagem_cores.png", &img, &info, err, sizeof err));
    CHECK(info.was_color);
    histogram_compute(&h, &img);
    CHECK_NEAR(h.mean, 111.88, 0.01);
    CHECK_NEAR(h.std_dev, 47.26, 0.01);
    gray_image_free(&img);
}

static void test_histogram(void)
{
    puts("histograma, média e desvio padrão");
    const uint8_t values[] = {0, 0, 255, 255};
    GrayImage img;
    make_gray(&img, 2, 2, values);
    Histogram h;
    histogram_compute(&h, &img);
    CHECK(h.total == 4);
    CHECK(h.bins[0] == 2 && h.bins[255] == 2 && h.max_count == 2);
    CHECK_NEAR(h.mean, 127.5, 1e-9);
    CHECK_NEAR(h.std_dev, 127.5, 1e-9);
    CHECK(h.contrast == CONTRAST_HIGH);
    CHECK(h.brightness == BRIGHTNESS_MEDIUM);
    gray_image_free(&img);

    const uint8_t flat[] = {200, 200, 200, 200};
    make_gray(&img, 2, 2, flat);
    histogram_compute(&h, &img);
    CHECK_NEAR(h.mean, 200.0, 1e-9);
    CHECK_NEAR(h.std_dev, 0.0, 1e-9);
    CHECK(h.brightness == BRIGHTNESS_LIGHT);
    CHECK(h.contrast == CONTRAST_LOW);
    gray_image_free(&img);
}

static void test_classification_limits(void)
{
    puts("limiares de classificação");
    CHECK(histogram_classify_brightness(0.0) == BRIGHTNESS_DARK);
    CHECK(histogram_classify_brightness(84.99) == BRIGHTNESS_DARK);
    CHECK(histogram_classify_brightness(85.0) == BRIGHTNESS_MEDIUM);
    CHECK(histogram_classify_brightness(169.99) == BRIGHTNESS_MEDIUM);
    CHECK(histogram_classify_brightness(170.0) == BRIGHTNESS_LIGHT);
    CHECK(histogram_classify_brightness(255.0) == BRIGHTNESS_LIGHT);

    CHECK(histogram_classify_contrast(0.0) == CONTRAST_LOW);
    CHECK(histogram_classify_contrast(39.99) == CONTRAST_LOW);
    CHECK(histogram_classify_contrast(40.0) == CONTRAST_MEDIUM);
    CHECK(histogram_classify_contrast(69.99) == CONTRAST_MEDIUM);
    CHECK(histogram_classify_contrast(70.0) == CONTRAST_HIGH);
    CHECK(histogram_classify_contrast(127.5) == CONTRAST_HIGH);
}

static void test_equalization_lut(void)
{
    puts("equalização: tabela de transformação");
    /* níveis {0,0,1,1,1,2,3,3}: cdf = 2,5,6,8 -> s = 255*cdf/8 = 63,75 (64), 159,375 (159), 191,25 (191), 255 */
    const uint8_t values[] = {0, 0, 1, 1, 1, 2, 3, 3};
    GrayImage img;
    make_gray(&img, 4, 2, values);
    Histogram h;
    histogram_compute(&h, &img);
    uint8_t lut[HISTOGRAM_LEVELS];
    equalization_build_lut(&h, lut);
    CHECK(lut[0] == 64);
    CHECK(lut[1] == 159);
    CHECK(lut[2] == 191);
    CHECK(lut[3] == 255);
    gray_image_free(&img);
}

static void test_equalization_image(void)
{
    puts("equalização: imagem, restauração e imagem de nível único");
    /* degradê de baixo contraste: níveis 100..120 */
    enum { W = 21, H = 8 };
    uint8_t values[W * H];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            values[y * W + x] = (uint8_t)(100 + x);
        }
    }
    GrayImage original;
    make_gray(&original, W, H, values);
    Histogram before;
    histogram_compute(&before, &original);

    EqualizationState state;
    equalization_state_init(&state, &original);
    CHECK(original.pixels == NULL); /* o estado assumiu a posse */
    CHECK(!state.active);
    CHECK(equalization_current(&state)->pixels[0] == 100);

    CHECK(equalization_toggle(&state));
    CHECK(state.active);
    const GrayImage *eq = equalization_current(&state);
    Histogram after;
    histogram_compute(&after, eq);
    CHECK(eq->width == W && eq->height == H);
    CHECK(after.bins[255] > 0); /* o nível mais alto sempre mapeia para 255 */
    CHECK(after.std_dev > before.std_dev);
    CHECK(before.contrast == CONTRAST_LOW);
    CHECK(after.contrast == CONTRAST_HIGH);
    for (int x = 1; x < W; x++) { /* monotônica: preserva a ordem dos níveis */
        CHECK(eq->pixels[x] >= eq->pixels[x - 1]);
    }

    /* restaurar: volta exatamente à original, sem recarregar nada */
    CHECK(equalization_toggle(&state));
    CHECK(!state.active);
    const GrayImage *back = equalization_current(&state);
    CHECK(memcmp(back->pixels, values, sizeof values) == 0);

    /* alternar de novo reaproveita a equalizada já calculada */
    uint8_t *eq_pixels = state.equalized.pixels;
    CHECK(equalization_toggle(&state));
    CHECK(state.equalized.pixels == eq_pixels);
    equalization_state_free(&state);

    /* imagem com um único nível de cinza não deve mudar */
    const uint8_t flat[] = {77, 77, 77, 77};
    GrayImage flat_img, flat_eq = {0};
    make_gray(&flat_img, 2, 2, flat);
    CHECK(equalization_apply(&flat_img, &flat_eq));
    CHECK(memcmp(flat_eq.pixels, flat, sizeof flat) == 0);
    gray_image_free(&flat_img);
    gray_image_free(&flat_eq);
}

static void test_resize(void)
{
    puts("redimensionamento bilinear");
    const uint8_t values[] = {0, 100, 200, 50};
    GrayImage src, dst = {0};
    make_gray(&src, 2, 2, values);

    CHECK(image_resize(&src, 4, 4, &dst));
    CHECK(dst.width == 4 && dst.height == 4);
    CHECK(dst.pixels[0] == 0);            /* canto superior esquerdo = origem */
    CHECK(dst.pixels[3] == 100);          /* canto superior direito */
    CHECK(dst.pixels[12] == 200);         /* canto inferior esquerdo */
    CHECK(dst.pixels[15] == 50);          /* canto inferior direito */
    CHECK(dst.pixels[1] > 0 && dst.pixels[1] < 100); /* interpolou entre os vizinhos */
    gray_image_free(&dst);

    /* mesma resolução: cópia idêntica */
    CHECK(image_resize(&src, 2, 2, &dst));
    CHECK(memcmp(dst.pixels, values, sizeof values) == 0);
    gray_image_free(&dst);

    /* imagem constante continua constante ao ampliar e reduzir */
    GrayImage flat;
    gray_image_create(&flat, 33, 17);
    memset(flat.pixels, 123, (size_t)flat.width * (size_t)flat.height);
    CHECK(image_resize(&flat, IMAGE_ALT_WIDTH, IMAGE_ALT_HEIGHT, &dst));
    CHECK(dst.width == 1024 && dst.height == 768);
    int all_same = 1;
    for (size_t i = 0; i < (size_t)dst.width * (size_t)dst.height; i++) {
        if (dst.pixels[i] != 123) {
            all_same = 0;
            break;
        }
    }
    CHECK(all_same);
    gray_image_free(&dst);
    GrayImage small;
    CHECK(image_resize(&flat, 5, 3, &small));
    CHECK(small.pixels[0] == 123 && small.pixels[14] == 123);
    gray_image_free(&small);
    gray_image_free(&flat);
    gray_image_free(&src);

    CHECK(!image_resize(&src, 4, 4, &dst)); /* origem vazia */
}

static void test_png_roundtrip(void)
{
    puts("salvar e recarregar PNG");
    enum { W = 37, H = 23 };
    uint8_t values[W * H];
    for (int i = 0; i < W * H; i++) {
        values[i] = (uint8_t)((i * 7) % 256);
    }
    GrayImage img, loaded;
    ImageInfo info;
    char err[256];
    make_gray(&img, W, H, values);

    CHECK(image_save_png(&img, TMP_PNG, err, sizeof err));
    CHECK(image_save_png(&img, TMP_PNG, err, sizeof err)); /* sobrescreve sem erro */
    CHECK(image_load(TMP_PNG, &loaded, &info, err, sizeof err));
    CHECK(!info.was_color);
    CHECK(loaded.width == W && loaded.height == H);
    CHECK(memcmp(loaded.pixels, values, sizeof values) == 0);
    gray_image_free(&loaded);
    SDL_RemovePath(TMP_PNG);

    CHECK(!image_save_png(&img, "pasta_inexistente/x.png", err, sizeof err));
    CHECK(err[0] != '\0');
    gray_image_free(&img);
}

static void test_utils(void)
{
    puts("utils");
    char buf[64];

    CHECK(strcmp(utils_basename("a/b/c.png"), "c.png") == 0);
    CHECK(strcmp(utils_basename("C:\\fotos\\c.png"), "c.png") == 0);
    CHECK(strcmp(utils_basename("c.png"), "c.png") == 0);

    utils_ellipsize_middle(buf, sizeof buf, "curto.png", 20);
    CHECK(strcmp(buf, "curto.png") == 0);
    utils_ellipsize_middle(buf, sizeof buf, "um_nome_de_arquivo_muito_comprido.png", 16);
    CHECK(strlen(buf) == 16);
    CHECK(strstr(buf, "...") != NULL);
    CHECK(strcmp(buf + strlen(buf) - 4, ".png") == 0);
    utils_ellipsize_middle(buf, sizeof buf, "ação_café_é_imagem_muito_longa.png", 12); /* UTF-8 */
    CHECK(strstr(buf, "...") != NULL);

    utils_format_number(buf, sizeof buf, 127.5, 2);
    CHECK(strcmp(buf, "127,50") == 0);

    CHECK(utils_to_u8(-5.0) == 0);
    CHECK(utils_to_u8(300.0) == 255);
    CHECK(utils_to_u8(127.5) == 128);
    CHECK(utils_to_u8(127.49) == 127);
    CHECK(utils_clamp_int(5, 0, 3) == 3);
    CHECK(utils_clamp_int(-1, 0, 3) == 0);
}

int main(void)
{
    utils_enable_utf8_console();

    test_luma_formula();
    test_gray_detection();
    test_load_errors();
    test_sample_images();
    test_histogram();
    test_classification_limits();
    test_equalization_lut();
    test_equalization_image();
    test_resize();
    test_png_roundtrip();
    test_utils();

    printf("\n%d verificações, %d falhas.\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
