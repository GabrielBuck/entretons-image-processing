/*
 * gui.h - Interface gráfica do Entretons (SDL3 + SDL_ttf).
 *
 * Duas janelas:
 *   - janela principal: exibe a imagem atual (tamanho original ou 1024 x 768);
 *   - janela secundária (filha da principal): 440 x 720 px, fixa no canto
 *     superior esquerdo, com histograma, estatísticas e dois botões.
 *
 * A GUI não processa imagens: ela desenha o que recebe em GuiView e devolve
 * ações do usuário (GuiAction) para o main decidir o que fazer.
 */
#ifndef ENTRETONS_GUI_H
#define ENTRETONS_GUI_H

#include <stdbool.h>
#include <stddef.h>

#include "histogram.h"
#include "image.h"

/* Tamanho fixo da janela secundária, em pixels. */
#define GUI_SIDE_WIDTH 440
#define GUI_SIDE_HEIGHT 720

typedef enum {
    GUI_ACTION_QUIT,              /* fechar janela ou Esc */
    GUI_ACTION_TOGGLE_EQUALIZE,   /* botão 1: Equalizar / Restaurar original */
    GUI_ACTION_TOGGLE_RESOLUTION, /* botão 2: Tamanho original / Exibir 1024 x 768 */
    GUI_ACTION_SAVE               /* tecla S: salvar output_image.png */
} GuiAction;

/* Estado a ser exibido. Os ponteiros pertencem ao chamador e devem continuar válidos. */
typedef struct {
    const GrayImage *image;     /* imagem exibida no momento */
    const Histogram *histogram; /* histograma e estatísticas dessa imagem */
    bool equalized;             /* a imagem exibida é a equalizada */
    bool alt_resolution;        /* a imagem exibida está em 1024 x 768 */
} GuiView;

typedef struct Gui Gui;

/*
 * Cria as janelas (ainda ocultas) e carrega a fonte. info descreve o arquivo
 * aberto. Requer SDL_Init(SDL_INIT_VIDEO). Retorna NULL e preenche err se falhar.
 */
Gui *gui_create(const char *file_name, const ImageInfo *info, char *err, size_t err_size);

/* Atualiza imagem, histograma e textos; ajusta tamanho/posição da janela principal. */
bool gui_update(Gui *gui, const GuiView *view);

/* Mensagem de retorno mostrada na janela secundária (ex.: resultado do salvamento). */
void gui_set_status(Gui *gui, bool is_error, const char *message);

/* Redesenha quando preciso e bloqueia até haver uma ação do usuário. */
GuiAction gui_wait_action(Gui *gui);

void gui_destroy(Gui *gui);

#endif /* ENTRETONS_GUI_H */
