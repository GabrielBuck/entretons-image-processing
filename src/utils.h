/*
 * utils.h - Funções auxiliares do Entretons.
 *
 * Mensagens no terminal, manipulação de texto/caminhos e localização de
 * arquivos de recursos (assets) sem depender do diretório de trabalho.
 */
#ifndef ENTRETONS_UTILS_H
#define ENTRETONS_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Nome do programa, usado em mensagens e títulos de janela. */
#define APP_NAME "Entretons"

/* Arquivo gerado ao salvar a imagem exibida. */
#define OUTPUT_FILENAME "output_image.png"

/* Configura o console do Windows para UTF-8 (acentos corretos). No Linux não faz nada. */
void utils_enable_utf8_console(void);

/* Mensagens no terminal. utils_error escreve em stderr. */
void utils_info(const char *fmt, ...);
void utils_error(const char *fmt, ...);

/* Limita v ao intervalo [lo, hi]. */
int utils_clamp_int(int v, int lo, int hi);

/* Converte um valor de ponto flutuante em intensidade 0..255 (arredonda e satura). */
uint8_t utils_to_u8(double v);

/* Retorna o nome do arquivo de um caminho (aceita '/' e '\'). */
const char *utils_basename(const char *path);

/*
 * Copia src em dst (tamanho dst_size); se tiver mais de max_chars caracteres
 * UTF-8, mantém o início e o fim e coloca "..." no meio.
 */
void utils_ellipsize_middle(char *dst, size_t dst_size, const char *src, size_t max_chars);

/* true se path existe e é um arquivo regular. */
bool utils_is_regular_file(const char *path);

/*
 * Procura um recurso (ex.: "assets/fonts/DejaVuSans.ttf") ao lado do
 * executável, na pasta acima dele e no diretório atual. Grava o caminho
 * encontrado em out e retorna true; retorna false se não achar.
 */
bool utils_find_asset(const char *relative_path, char *out, size_t out_size);

#endif /* ENTRETONS_UTILS_H */
