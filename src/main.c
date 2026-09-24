/*
 * main.c - Ponto de entrada do Entretons.
 *
 * Fluxo: valida os argumentos -> carrega a imagem (convertendo para cinza se
 * for colorida) -> abre as duas janelas -> trata as ações do usuário
 * (equalizar/restaurar, alternar resolução, salvar) até o programa ser fechado.
 *
 * Os módulos fazem o trabalho pesado: image (arquivos e pixels), histogram,
 * equalization e gui. Aqui fica só o controle geral.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "equalization.h"
#include "gui.h"
#include "histogram.h"
#include "image.h"
#include "utils.h"

/* Códigos de saída. */
enum {
    EXIT_CODE_OK = 0,
    EXIT_CODE_USAGE = 1,
    EXIT_CODE_IMAGE = 2,
    EXIT_CODE_GUI = 3
};

/* Tudo o que muda durante a execução. */
typedef struct {
    EqualizationState equalization; /* original em cinza + equalizada */
    bool alt_resolution;            /* true: exibindo em 1024 x 768 */
    GrayImage resized;              /* imagem em 1024 x 768 (só usada quando alt_resolution) */
    const GrayImage *shown;         /* imagem exibida agora */
    Histogram histogram;            /* histograma da imagem exibida */
} App;

static void print_usage(const char *program)
{
    printf("Uso: %s caminho_da_imagem.ext\n", program);
    printf("Exemplo: %s assets/samples/paisagem_cores.png\n", program);
}

static void print_stats(const char *label, const Histogram *hist)
{
    char mean[32];
    char std_dev[32];
    utils_format_number(mean, sizeof mean, hist->mean, 2);
    utils_format_number(std_dev, sizeof std_dev, hist->std_dev, 2);
    utils_info("%s: média %s, desvio padrão %s -> imagem %s, contraste %s.", label, mean, std_dev,
               histogram_brightness_label(hist->brightness), histogram_contrast_label(hist->contrast));
}

/*
 * Recalcula a imagem exibida (equalizada ou não, no tamanho original ou em
 * 1024 x 768), o histograma e as estatísticas, e atualiza a interface.
 * Retorna false, sem alterar nada, se faltar memória para o redimensionamento.
 * (Se a GUI não conseguir exibir a imagem, ela mesma mostra o erro na janela.)
 */
static bool refresh_view(App *app, Gui *gui)
{
    const GrayImage *current = equalization_current(&app->equalization);
    GrayImage resized = {0};

    if (app->alt_resolution) {
        if (!image_resize(current, IMAGE_ALT_WIDTH, IMAGE_ALT_HEIGHT, &resized)) {
            return false;
        }
    }
    gray_image_free(&app->resized);
    app->resized = resized;
    app->shown = app->alt_resolution ? &app->resized : current;

    histogram_compute(&app->histogram, app->shown);

    GuiView view = {
        .image = app->shown,
        .histogram = &app->histogram,
        .equalized = app->equalization.active,
        .alt_resolution = app->alt_resolution,
    };
    gui_update(gui, &view);
    return true;
}

static void on_toggle_equalize(App *app, Gui *gui)
{
    if (!equalization_toggle(&app->equalization)) {
        utils_error("memória insuficiente para equalizar a imagem.");
        gui_set_status(gui, true, "Memória insuficiente para equalizar.");
        return;
    }

    /* a mensagem vai antes da atualização: se a GUI falhar, o erro dela prevalece */
    const bool equalized = app->equalization.active;
    gui_set_status(gui, false, equalized ? "Imagem equalizada." : "Imagem original restaurada.");
    if (!refresh_view(app, gui)) {
        equalization_toggle(&app->equalization); /* desfaz: a versão equalizada já existe, não aloca */
        utils_error("memória insuficiente para redimensionar a imagem.");
        gui_set_status(gui, true, "Memória insuficiente para redimensionar.");
        return;
    }
    print_stats(equalized ? "Imagem equalizada" : "Imagem original restaurada", &app->histogram);
}

static void on_toggle_resolution(App *app, Gui *gui)
{
    app->alt_resolution = !app->alt_resolution;

    char message[128];
    if (app->alt_resolution) {
        snprintf(message, sizeof message, "Resolução: %d x %d px.", IMAGE_ALT_WIDTH, IMAGE_ALT_HEIGHT);
    } else {
        const GrayImage *current = equalization_current(&app->equalization);
        snprintf(message, sizeof message, "Resolução original: %d x %d px.", current->width,
                 current->height);
    }
    gui_set_status(gui, false, message);

    if (!refresh_view(app, gui)) {
        app->alt_resolution = !app->alt_resolution;
        utils_error("memória insuficiente para redimensionar a imagem.");
        gui_set_status(gui, true, "Memória insuficiente para redimensionar.");
        return;
    }
    utils_info("%s", message);
    print_stats("Histograma", &app->histogram);
}

static void on_save(const App *app, Gui *gui)
{
    char error[512];
    if (!image_save_png(app->shown, OUTPUT_FILENAME, error, sizeof error)) {
        utils_error("%s", error);
        gui_set_status(gui, true, "Erro ao salvar " OUTPUT_FILENAME ". Veja o terminal.");
        return;
    }

    char *cwd = SDL_GetCurrentDirectory();
    utils_info("Imagem salva em \"%s\" (%d x %d px). Pasta: %s", OUTPUT_FILENAME, app->shown->width,
               app->shown->height, cwd != NULL ? cwd : "(desconhecida)");
    SDL_free(cwd);

    char message[128];
    snprintf(message, sizeof message, "Imagem salva em " OUTPUT_FILENAME " (%d x %d px).",
             app->shown->width, app->shown->height);
    gui_set_status(gui, false, message);
}

int main(int argc, char *argv[])
{
    utils_enable_utf8_console();

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return EXIT_CODE_OK;
    }
    if (argc != 2) {
        utils_error("informe exatamente um caminho de imagem.");
        print_usage(argv[0]);
        return EXIT_CODE_USAGE;
    }

    /* 1) carregar e validar a imagem, convertendo para cinza quando necessário */
    GrayImage gray = {0};
    ImageInfo info = {0};
    char error[512];
    if (!image_load(argv[1], &gray, &info, error, sizeof error)) {
        utils_error("%s", error);
        return EXIT_CODE_IMAGE;
    }
    const char *file_name = utils_basename(argv[1]);
    utils_info("Imagem \"%s\": %d x %d pixels.", file_name, info.width, info.height);
    utils_info("%s", info.was_color ? "Imagem colorida: convertida para escala de cinza."
                                    : "Imagem já está em escala de cinza.");

    /* 2) estado da aplicação: a original em cinza fica guardada para restaurar sem recarregar */
    App app = {0};
    equalization_state_init(&app.equalization, &gray);

    /* 3) interface gráfica */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        utils_error("não foi possível inicializar a SDL: %s", SDL_GetError());
        equalization_state_free(&app.equalization);
        return EXIT_CODE_GUI;
    }
    Gui *gui = gui_create(file_name, &info, error, sizeof error);
    if (gui == NULL) {
        utils_error("%s", error);
        equalization_state_free(&app.equalization);
        SDL_Quit();
        return EXIT_CODE_GUI;
    }
    if (!refresh_view(&app, gui)) {
        utils_error("não foi possível exibir a imagem.");
    }
    print_stats("Histograma da imagem", &app.histogram);
    utils_info("Botões na janela secundária. S: salvar %s | Esc: sair.", OUTPUT_FILENAME);

    /* 4) laço principal: espera uma ação do usuário e trata */
    bool running = true;
    while (running) {
        switch (gui_wait_action(gui)) {
        case GUI_ACTION_TOGGLE_EQUALIZE:
            on_toggle_equalize(&app, gui);
            break;
        case GUI_ACTION_TOGGLE_RESOLUTION:
            on_toggle_resolution(&app, gui);
            break;
        case GUI_ACTION_SAVE:
            on_save(&app, gui);
            break;
        case GUI_ACTION_QUIT:
            running = false;
            break;
        }
    }

    gui_destroy(gui);
    gray_image_free(&app.resized);
    equalization_state_free(&app.equalization);
    SDL_Quit();
    return EXIT_CODE_OK;
}
