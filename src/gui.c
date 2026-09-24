#include "gui.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/* ------------------------------------------------------------------------- */
/* Identidade visual: fundo grafite, textos em creme, acentos âmbar          */
/* ------------------------------------------------------------------------- */

static const SDL_Color COLOR_BG = {27, 29, 33, 255};
static const SDL_Color COLOR_PANEL = {36, 39, 44, 255};
static const SDL_Color COLOR_PANEL_BORDER = {62, 66, 74, 255};
static const SDL_Color COLOR_GRID = {52, 56, 63, 255};
static const SDL_Color COLOR_BARS = {204, 196, 172, 255};
static const SDL_Color COLOR_CREAM = {242, 233, 208, 255};
static const SDL_Color COLOR_MUTED = {160, 155, 138, 255};
static const SDL_Color COLOR_AMBER = {227, 168, 59, 255};
static const SDL_Color COLOR_AMBER_DARK = {184, 131, 33, 255};
static const SDL_Color COLOR_ERROR = {226, 112, 98, 255};
static const SDL_Color COLOR_BUTTON = {46, 50, 57, 255};
static const SDL_Color COLOR_BUTTON_HOVER = {60, 65, 74, 255};
static const SDL_Color COLOR_BUTTON_BORDER = {78, 84, 95, 255};

/* ------------------------------------------------------------------------- */
/* Layout da janela secundária (coordenadas lógicas 440 x 720)               */
/* ------------------------------------------------------------------------- */

#define MARGIN 24.0f
#define CONTENT_WIDTH ((float)GUI_SIDE_WIDTH - 2.0f * MARGIN)

#define TITLE_Y 14.0f
#define FILE_Y 54.0f
#define INFO_Y 77.0f
#define SECTION_Y 100.0f
#define MEAN_LABEL_Y 118.0f
#define HIST_Y 148.0f
#define HIST_HEIGHT 190.0f
#define TICKS_Y 342.0f
#define STATS_Y 372.0f
#define STATS_STEP 24.0f
#define STATS_VALUE_X 190.0f
#define STATE_Y 480.0f
#define BUTTON1_Y 506.0f
#define BUTTON2_Y 566.0f
#define BUTTON_HEIGHT 52.0f
#define STATUS_Y 632.0f
#define FOOTER_Y 690.0f

/* Espaço entre as janelas e folga em relação às bordas da tela. */
#define WINDOW_GAP 12
#define SCREEN_MARGIN 24

#define FONT_RELATIVE_PATH "assets/fonts/DejaVuSans.ttf"

typedef struct {
    SDL_FRect rect;
    bool hover;
    bool pressed;
} Button;

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} TextAlign;

struct Gui {
    /* janela principal (imagem) */
    SDL_Window *main_window;
    SDL_Renderer *main_renderer;
    SDL_Texture *image_texture;
    int texture_width;
    int texture_height;
    uint8_t *rgba;
    size_t rgba_capacity;
    int logical_width;
    int logical_height;

    /* janela secundária (histograma e botões) */
    SDL_Window *side_window;
    SDL_Renderer *side_renderer;
    SDL_WindowID main_id;
    SDL_WindowID side_id;

    TTF_Font *font_title;
    TTF_Font *font_body;
    TTF_Font *font_small;
    TTF_Font *font_tiny;
    TTF_Font *font_button;

    char file_name[160];
    ImageInfo info;
    GuiView view;
    char status[256];
    bool status_is_error;

    Button button_equalize;
    Button button_resolution;

    bool windows_shown;
    bool dirty;
};

/* ------------------------------------------------------------------------- */
/* Utilidades                                                                */
/* ------------------------------------------------------------------------- */

static void set_error(char *err, size_t err_size, const char *fmt, ...) UTILS_PRINTF_LIKE(3, 4);

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

static void set_color(SDL_Renderer *renderer, SDL_Color c)
{
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static void fill_rect(SDL_Renderer *renderer, SDL_Color c, float x, float y, float w, float h)
{
    SDL_FRect r = {x, y, w, h};
    set_color(renderer, c);
    SDL_RenderFillRect(renderer, &r);
}

/* Contorno de 'thickness' pixels dentro do retângulo. */
static void outline_rect(SDL_Renderer *renderer, SDL_Color c, SDL_FRect rect, float thickness)
{
    set_color(renderer, c);
    for (float t = 0.0f; t < thickness; t += 1.0f) {
        SDL_FRect r = {rect.x + t, rect.y + t, rect.w - 2.0f * t, rect.h - 2.0f * t};
        SDL_RenderRect(renderer, &r);
    }
}

/* Desenha texto com o canto superior esquerdo (ou centro/direita, conforme align) em x, y. */
static void draw_text(Gui *gui, TTF_Font *font, const char *text, float x, float y, SDL_Color color,
                      TextAlign align)
{
    if (text == NULL || text[0] == '\0') {
        return;
    }
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);
    if (surface == NULL) {
        return;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(gui->side_renderer, surface);
    if (texture != NULL) {
        SDL_FRect dst = {x, y, (float)surface->w, (float)surface->h};
        if (align == ALIGN_CENTER) {
            dst.x = x - dst.w / 2.0f;
        } else if (align == ALIGN_RIGHT) {
            dst.x = x - dst.w;
        }
        dst.x = floorf(dst.x);
        dst.y = floorf(dst.y);
        SDL_RenderTexture(gui->side_renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

/* ------------------------------------------------------------------------- */
/* Janela principal: imagem                                                  */
/* ------------------------------------------------------------------------- */

/* Envia a imagem em tons de cinza para uma textura RGBA. */
static bool upload_image(Gui *gui, const GrayImage *image)
{
    const size_t count = (size_t)image->width * (size_t)image->height;
    const size_t needed = count * 4;

    if (needed > gui->rgba_capacity) {
        uint8_t *bigger = realloc(gui->rgba, needed);
        if (bigger == NULL) {
            return false;
        }
        gui->rgba = bigger;
        gui->rgba_capacity = needed;
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t v = image->pixels[i];
        gui->rgba[4 * i + 0] = v;
        gui->rgba[4 * i + 1] = v;
        gui->rgba[4 * i + 2] = v;
        gui->rgba[4 * i + 3] = 255;
    }

    if (gui->image_texture == NULL || gui->texture_width != image->width ||
        gui->texture_height != image->height) {
        SDL_DestroyTexture(gui->image_texture);
        gui->image_texture = SDL_CreateTexture(gui->main_renderer, SDL_PIXELFORMAT_RGBA32,
                                               SDL_TEXTUREACCESS_STATIC, image->width, image->height);
        if (gui->image_texture == NULL) {
            gui->texture_width = 0;
            gui->texture_height = 0;
            return false;
        }
        SDL_SetTextureScaleMode(gui->image_texture, SDL_SCALEMODE_LINEAR);
        gui->texture_width = image->width;
        gui->texture_height = image->height;
    }
    return SDL_UpdateTexture(gui->image_texture, NULL, gui->rgba, image->width * 4);
}

/*
 * Ajusta o tamanho e a posição da janela principal: tamanho da imagem (reduzido
 * proporcionalmente só se não couber na tela) e centralizada na área à direita
 * da janela secundária.
 */
static void layout_main_window(Gui *gui, int image_w, int image_h)
{
    SDL_Rect usable = {0, 0, 1920, 1080};
    SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable);

    int top = 0, left = 0, bottom = 0, right = 0;
    SDL_GetWindowBordersSize(gui->main_window, &top, &left, &bottom, &right);

    int side_w = 0, side_h = 0;
    SDL_GetWindowSize(gui->side_window, &side_w, &side_h);

    const int area_x = usable.x + side_w + left + right + WINDOW_GAP;
    const int area_w = usable.x + usable.w - area_x - SCREEN_MARGIN;
    const int area_h = usable.h - top - bottom - 2 * SCREEN_MARGIN;

    double scale = 1.0;
    if (area_w > 0 && (double)image_w > (double)area_w) {
        scale = (double)area_w / (double)image_w;
    }
    if (area_h > 0 && (double)image_h * scale > (double)area_h) {
        scale = (double)area_h / (double)image_h;
    }

    int win_w = utils_clamp_int((int)lround((double)image_w * scale), 1, 100000);
    int win_h = utils_clamp_int((int)lround((double)image_h * scale), 1, 100000);

    int x = area_x + (area_w - win_w) / 2;
    int y = usable.y + top + (usable.h - top - bottom - win_h) / 2;
    if (x < area_x) {
        x = area_x;
    }
    if (y < usable.y + top) {
        y = usable.y + top;
    }

    SDL_SetRenderLogicalPresentation(gui->main_renderer, image_w, image_h,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetWindowSize(gui->main_window, win_w, win_h);
    SDL_SetWindowPosition(gui->main_window, x, y);
    gui->logical_width = image_w;
    gui->logical_height = image_h;
}

static void render_main_window(Gui *gui)
{
    SDL_SetRenderDrawColor(gui->main_renderer, 0, 0, 0, 255);
    SDL_RenderClear(gui->main_renderer);
    if (gui->image_texture != NULL) {
        SDL_RenderTexture(gui->main_renderer, gui->image_texture, NULL, NULL);
    }
    SDL_RenderPresent(gui->main_renderer);
}

/* ------------------------------------------------------------------------- */
/* Janela secundária: histograma, estatísticas e botões                      */
/* ------------------------------------------------------------------------- */

static void draw_histogram(Gui *gui)
{
    SDL_Renderer *r = gui->side_renderer;
    const Histogram *h = gui->view.histogram;
    const SDL_FRect box = {MARGIN, HIST_Y, CONTENT_WIDTH, HIST_HEIGHT};
    const float bin_w = box.w / (float)HISTOGRAM_LEVELS;

    fill_rect(r, COLOR_PANEL, box.x, box.y, box.w, box.h);

    /* linhas de referência em 25%, 50% e 75% da altura */
    set_color(r, COLOR_GRID);
    for (int i = 1; i <= 3; i++) {
        float y = floorf(box.y + box.h * (float)i / 4.0f);
        SDL_RenderLine(r, box.x, y, box.x + box.w, y);
    }

    /* barras: altura proporcional à contagem, relativa ao nível mais frequente */
    if (h != NULL && h->max_count > 0) {
        SDL_FRect bars[HISTOGRAM_LEVELS];
        int n = 0;
        const float max_bar = box.h - 4.0f;
        for (int k = 0; k < HISTOGRAM_LEVELS; k++) {
            if (h->bins[k] == 0) {
                continue;
            }
            float x0 = floorf(box.x + (float)k * bin_w);
            float x1 = floorf(box.x + (float)(k + 1) * bin_w);
            float bar_h = (float)h->bins[k] / (float)h->max_count * max_bar;
            if (bar_h < 1.0f) {
                bar_h = 1.0f;
            }
            bars[n].x = x0;
            bars[n].y = box.y + box.h - bar_h;
            bars[n].w = (x1 > x0) ? (x1 - x0) : 1.0f;
            bars[n].h = bar_h;
            n++;
        }
        set_color(r, COLOR_BARS);
        SDL_RenderFillRects(r, bars, n);
    }

    outline_rect(r, COLOR_PANEL_BORDER, box, 1.0f);

    /* marca da média: linha âmbar, triângulo e rótulo */
    if (h != NULL && h->total > 0) {
        const float mx = floorf(box.x + ((float)h->mean + 0.5f) * bin_w);
        fill_rect(r, COLOR_AMBER, mx - 1.0f, box.y, 2.0f, box.h);

        SDL_FColor amber = {COLOR_AMBER.r / 255.0f, COLOR_AMBER.g / 255.0f, COLOR_AMBER.b / 255.0f, 1.0f};
        SDL_Vertex tri[3] = {
            {{mx - 6.0f, box.y - 11.0f}, amber, {0.0f, 0.0f}},
            {{mx + 6.0f, box.y - 11.0f}, amber, {0.0f, 0.0f}},
            {{mx, box.y - 1.0f}, amber, {0.0f, 0.0f}},
        };
        SDL_RenderGeometry(r, NULL, tri, 3, NULL, 0);

        char number[32];
        char label[64];
        utils_format_number(number, sizeof number, h->mean, 1);
        snprintf(label, sizeof label, "média %s", number);

        float lx = mx;
        int label_w = 0, label_h = 0;
        TTF_GetStringSize(gui->font_tiny, label, 0, &label_w, &label_h);
        if (lx - (float)label_w / 2.0f < MARGIN) {
            lx = MARGIN + (float)label_w / 2.0f;
        }
        if (lx + (float)label_w / 2.0f > (float)GUI_SIDE_WIDTH - MARGIN) {
            lx = (float)GUI_SIDE_WIDTH - MARGIN - (float)label_w / 2.0f;
        }
        draw_text(gui, gui->font_tiny, label, lx, MEAN_LABEL_Y, COLOR_AMBER, ALIGN_CENTER);
    }

    /* escala do eixo horizontal */
    static const int ticks[] = {0, 64, 128, 192, 255};
    for (size_t i = 0; i < sizeof ticks / sizeof ticks[0]; i++) {
        char text[8];
        snprintf(text, sizeof text, "%d", ticks[i]);
        float x = box.x + (float)ticks[i] * bin_w;
        TextAlign align = ALIGN_CENTER;
        if (ticks[i] == 0) {
            align = ALIGN_LEFT;
        } else if (ticks[i] == 255) {
            align = ALIGN_RIGHT;
            x = box.x + box.w;
        }
        draw_text(gui, gui->font_tiny, text, x, TICKS_Y, COLOR_MUTED, align);
    }
}

static void draw_stats(Gui *gui)
{
    const Histogram *h = gui->view.histogram;
    if (h == NULL) {
        return;
    }
    char value[64];
    float y = STATS_Y;

    utils_format_number(value, sizeof value, h->mean, 2);
    draw_text(gui, gui->font_body, "Média", MARGIN, y, COLOR_MUTED, ALIGN_LEFT);
    draw_text(gui, gui->font_body, value, STATS_VALUE_X, y, COLOR_CREAM, ALIGN_LEFT);
    y += STATS_STEP;

    utils_format_number(value, sizeof value, h->std_dev, 2);
    draw_text(gui, gui->font_body, "Desvio padrão", MARGIN, y, COLOR_MUTED, ALIGN_LEFT);
    draw_text(gui, gui->font_body, value, STATS_VALUE_X, y, COLOR_CREAM, ALIGN_LEFT);
    y += STATS_STEP;

    snprintf(value, sizeof value, "imagem %s", histogram_brightness_label(h->brightness));
    draw_text(gui, gui->font_body, "Brilho", MARGIN, y, COLOR_MUTED, ALIGN_LEFT);
    draw_text(gui, gui->font_body, value, STATS_VALUE_X, y, COLOR_AMBER, ALIGN_LEFT);
    y += STATS_STEP;

    snprintf(value, sizeof value, "%s", histogram_contrast_label(h->contrast));
    draw_text(gui, gui->font_body, "Contraste", MARGIN, y, COLOR_MUTED, ALIGN_LEFT);
    draw_text(gui, gui->font_body, value, STATS_VALUE_X, y, COLOR_AMBER, ALIGN_LEFT);
}

static void draw_button(Gui *gui, const Button *button, const char *label)
{
    SDL_Renderer *r = gui->side_renderer;
    SDL_Color fill = COLOR_BUTTON;
    SDL_Color border = COLOR_BUTTON_BORDER;
    SDL_Color text = COLOR_CREAM;

    if (button->hover && button->pressed) { /* pressionado */
        fill = COLOR_AMBER_DARK;
        border = COLOR_AMBER;
        text = COLOR_BG;
    } else if (button->hover) { /* mouse sobre o botão */
        fill = COLOR_BUTTON_HOVER;
        border = COLOR_AMBER;
    }

    set_color(r, fill);
    SDL_RenderFillRect(r, &button->rect);
    outline_rect(r, border, button->rect, 2.0f);

    int text_w = 0, text_h = 0;
    TTF_GetStringSize(gui->font_button, label, 0, &text_w, &text_h);
    draw_text(gui, gui->font_button, label, button->rect.x + button->rect.w / 2.0f,
              button->rect.y + (button->rect.h - (float)text_h) / 2.0f, text, ALIGN_CENTER);
}

static void render_side_window(Gui *gui)
{
    SDL_Renderer *r = gui->side_renderer;
    SDL_SetRenderDrawColor(r, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(r);

    /* cabeçalho */
    char text[256];
    draw_text(gui, gui->font_title, APP_NAME, MARGIN, TITLE_Y, COLOR_AMBER, ALIGN_LEFT);

    utils_ellipsize_middle(text, sizeof text, gui->file_name, 40);
    draw_text(gui, gui->font_body, text, MARGIN, FILE_Y, COLOR_CREAM, ALIGN_LEFT);

    snprintf(text, sizeof text, "%d × %d px · %s", gui->info.width, gui->info.height,
             gui->info.was_color ? "colorida, convertida para cinza" : "já estava em escala de cinza");
    draw_text(gui, gui->font_small, text, MARGIN, INFO_Y, COLOR_MUTED, ALIGN_LEFT);

    draw_text(gui, gui->font_tiny, "HISTOGRAMA  ·  256 NÍVEIS DE CINZA", MARGIN, SECTION_Y, COLOR_MUTED,
              ALIGN_LEFT);

    draw_histogram(gui);
    draw_stats(gui);

    /* estado atual */
    if (gui->view.image != NULL) {
        snprintf(text, sizeof text, "Exibindo: imagem %s · %d × %d px",
                 gui->view.equalized ? "equalizada" : "original", gui->view.image->width,
                 gui->view.image->height);
        draw_text(gui, gui->font_small, text, MARGIN, STATE_Y, COLOR_CREAM, ALIGN_LEFT);
    }

    /* botões: equalização abaixo do histograma e resolução logo abaixo dele */
    draw_button(gui, &gui->button_equalize, gui->view.equalized ? "Restaurar original" : "Equalizar");
    draw_button(gui, &gui->button_resolution,
                gui->view.alt_resolution ? "Tamanho original" : "Exibir 1024 x 768");

    /* retorno das ações (ex.: imagem salva) e dica de teclado */
    if (gui->status[0] != '\0') {
        utils_ellipsize_middle(text, sizeof text, gui->status, 54);
        draw_text(gui, gui->font_small, text, MARGIN, STATUS_Y,
                  gui->status_is_error ? COLOR_ERROR : COLOR_AMBER, ALIGN_LEFT);
    }
    draw_text(gui, gui->font_tiny, "S  salvar " OUTPUT_FILENAME "     ·     Esc  sair",
              (float)GUI_SIDE_WIDTH / 2.0f, FOOTER_Y, COLOR_MUTED, ALIGN_CENTER);

    SDL_RenderPresent(r);
}

/* ------------------------------------------------------------------------- */
/* Criação e destruição                                                      */
/* ------------------------------------------------------------------------- */

Gui *gui_create(const char *file_name, const ImageInfo *info, char *err, size_t err_size)
{
    Gui *gui = calloc(1, sizeof *gui);
    if (gui == NULL) {
        set_error(err, err_size, "memória insuficiente para a interface.");
        return NULL;
    }
    snprintf(gui->file_name, sizeof gui->file_name, "%s", file_name);
    gui->info = *info;

    /* fonte: DejaVu Sans, guardada dentro do projeto */
    char font_path[1024];
    if (!utils_find_asset(FONT_RELATIVE_PATH, font_path, sizeof font_path)) {
        set_error(err, err_size,
                  "fonte não encontrada (" FONT_RELATIVE_PATH "). Execute o programa a partir da pasta do projeto.");
        gui_destroy(gui);
        return NULL;
    }
    if (!TTF_Init()) {
        set_error(err, err_size, "não foi possível inicializar a SDL_ttf: %s", SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }
    gui->font_title = TTF_OpenFont(font_path, 30.0f);
    gui->font_body = TTF_OpenFont(font_path, 15.0f);
    gui->font_small = TTF_OpenFont(font_path, 13.0f);
    gui->font_tiny = TTF_OpenFont(font_path, 12.0f);
    gui->font_button = TTF_OpenFont(font_path, 18.0f);
    if (!gui->font_title || !gui->font_body || !gui->font_small || !gui->font_tiny || !gui->font_button) {
        set_error(err, err_size, "não foi possível abrir a fonte \"%s\": %s", font_path, SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }

    /* janela principal: começa oculta; gui_update define tamanho e posição */
    char title[256];
    snprintf(title, sizeof title, APP_NAME " - %s", gui->file_name);
    gui->main_window = SDL_CreateWindow(title, info->width, info->height, SDL_WINDOW_HIDDEN);
    if (gui->main_window == NULL) {
        set_error(err, err_size, "não foi possível criar a janela principal: %s", SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }
    gui->main_renderer = SDL_CreateRenderer(gui->main_window, NULL);
    if (gui->main_renderer == NULL) {
        set_error(err, err_size, "não foi possível criar o renderer da janela principal: %s", SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }

    /* janela secundária: 440 x 720, tamanho fixo, filha da principal */
    gui->side_window = SDL_CreateWindow(APP_NAME " - Histograma", GUI_SIDE_WIDTH, GUI_SIDE_HEIGHT,
                                        SDL_WINDOW_HIDDEN);
    if (gui->side_window == NULL) {
        set_error(err, err_size, "não foi possível criar a janela secundária: %s", SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }
    gui->side_renderer = SDL_CreateRenderer(gui->side_window, NULL);
    if (gui->side_renderer == NULL) {
        set_error(err, err_size, "não foi possível criar o renderer da janela secundária: %s",
                  SDL_GetError());
        gui_destroy(gui);
        return NULL;
    }
    SDL_SetRenderLogicalPresentation(gui->side_renderer, GUI_SIDE_WIDTH, GUI_SIDE_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    if (!SDL_SetWindowParent(gui->side_window, gui->main_window)) {
        /* Não é fatal: sem o vínculo de janela filha, as duas continuam funcionando. */
        utils_info("Aviso: não foi possível vincular a janela secundária à principal: %s", SDL_GetError());
    }

    gui->main_id = SDL_GetWindowID(gui->main_window);
    gui->side_id = SDL_GetWindowID(gui->side_window);

    gui->button_equalize.rect = (SDL_FRect){MARGIN, BUTTON1_Y, CONTENT_WIDTH, BUTTON_HEIGHT};
    gui->button_resolution.rect = (SDL_FRect){MARGIN, BUTTON2_Y, CONTENT_WIDTH, BUTTON_HEIGHT};
    gui->dirty = true;
    return gui;
}

void gui_destroy(Gui *gui)
{
    if (gui == NULL) {
        return;
    }
    SDL_DestroyTexture(gui->image_texture);
    if (gui->side_renderer) {
        SDL_DestroyRenderer(gui->side_renderer);
    }
    if (gui->side_window) {
        SDL_DestroyWindow(gui->side_window);
    }
    if (gui->main_renderer) {
        SDL_DestroyRenderer(gui->main_renderer);
    }
    if (gui->main_window) {
        SDL_DestroyWindow(gui->main_window);
    }
    if (gui->font_title) {
        TTF_CloseFont(gui->font_title);
    }
    if (gui->font_body) {
        TTF_CloseFont(gui->font_body);
    }
    if (gui->font_small) {
        TTF_CloseFont(gui->font_small);
    }
    if (gui->font_tiny) {
        TTF_CloseFont(gui->font_tiny);
    }
    if (gui->font_button) {
        TTF_CloseFont(gui->font_button);
    }
    if (TTF_WasInit()) {
        TTF_Quit();
    }
    free(gui->rgba);
    free(gui);
}

/* ------------------------------------------------------------------------- */
/* Atualização                                                               */
/* ------------------------------------------------------------------------- */

/* Mostra as janelas na primeira atualização e fixa a secundária no canto (0, 0). */
static void show_windows(Gui *gui)
{
    SDL_ShowWindow(gui->main_window);
    SDL_ShowWindow(gui->side_window);
    SDL_SyncWindow(gui->side_window);

    /*
     * Posição (0, 0) da janela secundária. A posição da SDL se refere à área
     * interna da janela; somando a borda e a barra de título, a moldura completa
     * fica encostada no canto superior esquerdo e a barra continua visível.
     */
    int top = 0, left = 0, bottom = 0, right = 0;
    SDL_GetWindowBordersSize(gui->side_window, &top, &left, &bottom, &right);
    SDL_SetWindowPosition(gui->side_window, left, top);
    gui->windows_shown = true;
}

bool gui_update(Gui *gui, const GuiView *view)
{
    gui->view = *view;
    bool ok = true;

    if (view->image != NULL && view->image->pixels != NULL) {
        if (!upload_image(gui, view->image)) {
            ok = false;
            gui_set_status(gui, true, "Não foi possível exibir a imagem nesta resolução.");
        }
        if (view->image->width != gui->logical_width || view->image->height != gui->logical_height) {
            layout_main_window(gui, view->image->width, view->image->height);
        }
    }

    if (!gui->windows_shown) {
        show_windows(gui);
        /* Depois de mostrar (e de existir barra de título), reposiciona a principal. */
        if (view->image != NULL) {
            layout_main_window(gui, view->image->width, view->image->height);
        }
    }

    gui->dirty = true;
    return ok;
}

void gui_set_status(Gui *gui, bool is_error, const char *message)
{
    snprintf(gui->status, sizeof gui->status, "%s", message != NULL ? message : "");
    gui->status_is_error = is_error;
    gui->dirty = true;
}

/* ------------------------------------------------------------------------- */
/* Eventos                                                                   */
/* ------------------------------------------------------------------------- */

static bool point_in_rect(const SDL_FRect *r, float x, float y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static void update_hover(Gui *gui, float x, float y)
{
    bool h1 = point_in_rect(&gui->button_equalize.rect, x, y);
    bool h2 = point_in_rect(&gui->button_resolution.rect, x, y);
    if (h1 != gui->button_equalize.hover || h2 != gui->button_resolution.hover) {
        gui->button_equalize.hover = h1;
        gui->button_resolution.hover = h2;
        gui->dirty = true;
    }
}

static void clear_hover(Gui *gui)
{
    if (gui->button_equalize.hover || gui->button_resolution.hover || gui->button_equalize.pressed ||
        gui->button_resolution.pressed) {
        gui->button_equalize.hover = gui->button_resolution.hover = false;
        gui->button_equalize.pressed = gui->button_resolution.pressed = false;
        gui->dirty = true;
    }
}

/* Processa um evento. Retorna true e preenche *action se ele gerou uma ação. */
static bool handle_event(Gui *gui, SDL_Event *event, GuiAction *action)
{
    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        *action = GUI_ACTION_QUIT;
        return true;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.repeat) {
            break;
        }
        if (event->key.key == SDLK_ESCAPE) {
            *action = GUI_ACTION_QUIT;
            return true;
        }
        if (event->key.key == SDLK_S) {
            *action = GUI_ACTION_SAVE;
            return true;
        }
        break;

    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_SHOWN:
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        gui->dirty = true;
        break;

    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        if (event->window.windowID == gui->side_id) {
            clear_hover(gui);
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (event->motion.windowID == gui->side_id) {
            SDL_ConvertEventToRenderCoordinates(gui->side_renderer, event);
            update_hover(gui, event->motion.x, event->motion.y);
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.windowID == gui->side_id && event->button.button == SDL_BUTTON_LEFT) {
            SDL_ConvertEventToRenderCoordinates(gui->side_renderer, event);
            update_hover(gui, event->button.x, event->button.y);
            gui->button_equalize.pressed = gui->button_equalize.hover;
            gui->button_resolution.pressed = gui->button_resolution.hover;
            gui->dirty = true;
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            bool was_equalize = gui->button_equalize.pressed;
            bool was_resolution = gui->button_resolution.pressed;
            gui->button_equalize.pressed = false;
            gui->button_resolution.pressed = false;
            gui->dirty = true;

            if (event->button.windowID == gui->side_id) {
                SDL_ConvertEventToRenderCoordinates(gui->side_renderer, event);
                update_hover(gui, event->button.x, event->button.y);
                /* o clique vale se o botão foi solto sobre o mesmo botão em que foi pressionado */
                if (was_equalize && gui->button_equalize.hover) {
                    *action = GUI_ACTION_TOGGLE_EQUALIZE;
                    return true;
                }
                if (was_resolution && gui->button_resolution.hover) {
                    *action = GUI_ACTION_TOGGLE_RESOLUTION;
                    return true;
                }
            } else {
                clear_hover(gui);
            }
        }
        break;

    default:
        break;
    }
    return false;
}

GuiAction gui_wait_action(Gui *gui)
{
    for (;;) {
        if (gui->dirty) {
            render_main_window(gui);
            render_side_window(gui);
            gui->dirty = false;
        }

        SDL_Event event;
        if (!SDL_WaitEvent(&event)) {
            return GUI_ACTION_QUIT;
        }
        GuiAction action;
        if (handle_event(gui, &event, &action)) {
            return action;
        }
    }
}
