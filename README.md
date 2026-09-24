# Entretons

Protótipo de ferramenta de análise e processamento de imagens em **C**, com **SDL3**, **SDL_image** e **SDL_ttf**.
Projeto 1 de Computação Visual (07G), Ciência da Computação, Universidade Presbiteriana Mackenzie, com o professor André Kishimoto.

Autores: Gabriel Nottoli Buck e Julia Andrade.

O programa carrega uma imagem, converte para escala de cinza quando necessário, mostra o histograma e as estatísticas em uma segunda janela e permite equalizar o histograma, voltar à imagem original, alternar a resolução e salvar o resultado.

| Imagem original | Após equalizar |
|---|---|
| ![Interface com a imagem original](docs/interface_original.png) | ![Interface com a imagem equalizada](docs/interface_equalizada.png) |

## Funcionalidades

1. Recebe o caminho da imagem pela linha de comando: `entretons caminho_da_imagem.ext`.
2. Valida o argumento, a existência do arquivo e o formato. Erros aparecem no terminal com uma mensagem clara.
3. Detecta se a imagem é colorida ou já está em escala de cinza. Se for colorida, converte com `Y = 0,2125 R + 0,7154 G + 0,0721 B`.
4. Abre duas janelas:
   - **principal**: a imagem atual;
   - **secundária** (filha da principal, 440 x 720 px, tamanho fixo, em (0, 0)): histograma, estatísticas e botões.
5. Histograma de 256 níveis com barras proporcionais e marca da média, mais média, desvio padrão e classificação da imagem.
6. **Equalizar** / **Restaurar original**: a original fica guardada em memória, então restaurar não recarrega o arquivo.
7. **Exibir 1024 x 768** / **Tamanho original**: alterna a resolução (interpolação bilinear) e ajusta tamanho e posição da janela.
8. Imagem, histograma e estatísticas são atualizados juntos a cada mudança.
9. Tecla **S** salva a imagem exibida em `output_image.png` (sobrescreve o arquivo existente e informa o resultado no terminal e na janela).

Controles: mouse nos botões da janela secundária, **S** para salvar, **Esc** (ou fechar qualquer janela) para sair.

Formatos aceitos: os que a SDL_image abrir (PNG, JPEG, BMP, GIF, entre outros).

## Requisitos

- GCC com suporte a C11 (o projeto é compilado com `-std=c11`) e GNU Make
- SDL3, SDL_image e SDL_ttf. Versões usadas nos testes: **SDL 3.4.16**, **SDL_image 3.4.6**, **SDL_ttf 3.2.2**

Ambientes de destino: Windows 10/11 com GCC (MinGW-w64) e WSL Ubuntu.

## Compilação

O `Makefile` procura as bibliotecas nesta ordem:

1. pastas informadas por você (`SDL_PREFIX`, ou `SDL3_DIR` + `SDL3_IMAGE_DIR` + `SDL3_TTF_DIR`), cada uma com `include/` e `lib/`;
2. `pkg-config` (`sdl3`, `sdl3-image`, `sdl3-ttf`);
3. os caminhos padrão do GCC (`-lSDL3 -lSDL3_image -lSDL3_ttf`).

### Windows (GCC / MinGW-w64)

Com os pacotes de desenvolvimento para MinGW baixados nas páginas de releases da SDL, informe a pasta `x86_64-w64-mingw32` de cada um. Para não digitar isso toda vez, crie um arquivo `config.mk` na raiz do projeto (ele não é versionado):

```make
SDL3_DIR       = C:/libs/SDL3-3.4.16/x86_64-w64-mingw32
SDL3_IMAGE_DIR = C:/libs/SDL3_image-3.4.6/x86_64-w64-mingw32
SDL3_TTF_DIR   = C:/libs/SDL3_ttf-3.2.2/x86_64-w64-mingw32
```

Depois:

```
mingw32-make          (ou make, conforme sua instalação)
mingw32-make dlls     copia as DLLs do SDL para a pasta do projeto
entretons.exe assets\samples\paisagem_cores.png
```

No MSYS2, se os pacotes `sdl3`, `sdl3-image` e `sdl3-ttf` estiverem instalados, o `pkg-config` resolve tudo sozinho e basta `make`.

### WSL Ubuntu / Linux

Se sua distribuição tiver os pacotes de desenvolvimento do SDL3 (`libsdl3-dev`, `libsdl3-image-dev`, `libsdl3-ttf-dev`), basta:

```
make
./entretons assets/samples/paisagem_cores.png
```

Se não tiver, compile as bibliotecas do código-fonte para uma pasta local (as versões acima foram compiladas assim nos testes):

```
sudo apt install build-essential cmake ninja-build pkg-config libfreetype-dev \
  libx11-dev libxcursor-dev libxi-dev libxrandr-dev libxext-dev libxfixes-dev \
  libxss-dev libxtst-dev libxrender-dev libxkbcommon-dev libgl1-mesa-dev libegl1-mesa-dev

P=$HOME/sdl3
git clone --depth 1 --branch release-3.4.16 https://github.com/libsdl-org/SDL
cmake -S SDL -B SDL/b -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P && cmake --build SDL/b && cmake --install SDL/b

git clone --depth 1 --branch release-3.4.6 https://github.com/libsdl-org/SDL_image
cmake -S SDL_image -B SDL_image/b -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P -DSDLIMAGE_VENDORED=OFF && cmake --build SDL_image/b && cmake --install SDL_image/b

git clone --depth 1 --branch release-3.2.2 https://github.com/libsdl-org/SDL_ttf
cmake -S SDL_ttf -B SDL_ttf/b -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P -DSDLTTF_VENDORED=OFF && cmake --build SDL_ttf/b && cmake --install SDL_ttf/b

make SDL_PREFIX=$P
```

O WSL precisa de WSLg (Windows 11 ou Windows 10 recente) para abrir as janelas.

### Alvos do Makefile

| Comando | O que faz |
|---|---|
| `make` | compila o executável `entretons` (`entretons.exe` no Windows) |
| `make run IMG=caminho.png` | compila e executa |
| `make test` | roda os testes automáticos (sem abrir janelas) |
| `make dlls` | (Windows) copia as DLLs do SDL para a pasta do projeto |
| `make clean` | remove objetos e executáveis |

## Arquitetura

```
src/
  main.c          fluxo principal: argumentos, carregamento, laço de ações
  image.c/h       carregar e validar, detectar RGB/cinza, converter, copiar, redimensionar, salvar PNG
  histogram.c/h   histograma de 256 níveis, média, desvio padrão, classificação
  equalization.c/h equalização, imagem processada, restauração da original
  gui.c/h         janelas, renderização, botões, eventos
  utils.c/h       mensagens, texto, caminhos, busca de assets
assets/
  fonts/          DejaVu Sans (fonte dentro do projeto, com a licença)
  samples/        imagens de exemplo (uma colorida e três em cinza)
tests/
  test_core.c     testes automáticos
docs/             capturas de tela
```

Como as partes conversam:

- `image` só conhece pixels e arquivos. Toda imagem em processamento é uma `GrayImage` (8 bits por pixel).
- `histogram` e `equalization` trabalham só com `GrayImage`, sem depender da SDL de janelas.
- `gui` desenha o que recebe (`GuiView`) e devolve ações (`GuiAction`). Não processa imagem.
- `main` liga tudo: recebe a ação, atualiza o estado, recalcula a imagem exibida e o histograma e chama `gui_update`.

Estado da aplicação (equalização e resolução são independentes): a imagem exibida sempre é `original` ou `equalizada`, no tamanho original ou em 1024 x 768. O histograma e as estatísticas são calculados sobre a imagem exibida, que é a mesma salva em `output_image.png`.

## Decisões de projeto

O enunciado deixa alguns pontos em aberto. O que foi adotado:

**Classificação por brilho** (média das intensidades, faixa dividida em terços): `< 85` escura, `85 a 169` média, `>= 170` clara.

**Classificação por contraste** (desvio padrão): `< 40` baixo, `40 a 69` médio, `>= 70` alto. Uma imagem com todos os níveis igualmente frequentes tem desvio de cerca de 74, então uma imagem bem equalizada cai em "alto".

Os limiares estão em constantes documentadas em `src/histogram.h` e são fáceis de ajustar.

**Equalização**: equalização clássica de histograma pela função de distribuição acumulada,
`s(k) = round(255 * (cdf(k) - cdf_min) / (N - cdf_min))`, onde `N` é o total de pixels e `cdf_min` o menor valor não nulo da CDF. Uma imagem com um único nível de cinza não é alterada. A versão equalizada é calculada uma vez e guardada.

**1024 x 768**: a imagem é interpolada bilinearmente para exatamente 1024 x 768, preenchendo toda a janela. Se a imagem não tiver proporção 4:3, ela fica esticada. A troca é feita em `refresh_view` (`src/main.c`) chamando `image_resize`.

**Tamanho e posição das janelas**: a janela principal usa o tamanho da imagem e é centralizada à direita da secundária. Só se a imagem não couber na tela ela é reduzida proporcionalmente. A secundária é posicionada considerando a barra de título para ficar encostada no canto (0, 0) sem esconder a barra.

**Transparência**: o canal alfa de PNGs com transparência é ignorado; vale o valor RGB do pixel.

**PNG cinza de 8 bits**: a SDL_image 3.4.6 entrega esses arquivos como índices com uma paleta deslocada em 1 nível (255 virava 254). O carregamento detecta esse caso e usa o índice diretamente, preservando os níveis originais. Há um teste para isso.

**Fonte**: DejaVu Sans, dentro do projeto (`assets/fonts`), localizada a partir da pasta do executável. O programa funciona independentemente das fontes instaladas no sistema.

## Testes

```
make test
```

Roda 137 verificações: fórmula de conversão para cinza, detecção colorida/cinza, erros de carregamento (arquivo inexistente, pasta, arquivo que não é imagem), histograma e limiares, equalização (inclusive restaurar a original), redimensionamento, PNG salvo e recarregado e funções auxiliares. Rode a partir da pasta do projeto, pois alguns testes usam `assets/samples`.

Situação da validação: compilado sem avisos (`-Wall -Wextra -Wpedantic`) em três ambientes:

- **Linux** (Ubuntu 24.04, GCC 13.3): 137/137 testes, janelas em display virtual com cliques simulados, ASan/UBSan sem erros.
- **Windows 11** (GCC 15.2.0, MinGW-w64 via MSYS2, `mingw32-make`): 137/137 testes, `entretons.exe` executado com `assets/samples/paisagem_cores.png`, janela secundária em (0, 0) com 440 x 720, acentos e "×" corretos com a fonte DejaVu Sans embutida, tecla S salvando `output_image.png`. As bibliotecas SDL3 3.4.16, SDL_image 3.4.6 e SDL_ttf 3.2.2 foram usadas nas versões `-devel-mingw` oficiais. Testado tanto rodando `mingw32-make` direto do PowerShell/cmd (sem MSYS2 no PATH) quanto do Git Bash/MSYS2.
- **WSL Ubuntu 24.04.1** (GCC 13.3.0; não foi possível confirmar o Ubuntu 26.04/GCC 15.2.0 mencionado no enunciado do grupo, pois a distribuição instalada na máquina testada é a 24.04): SDL3, SDL_image e SDL_ttf compilados do fonte (não há pacotes `libsdl3-dev` no APT do 24.04), 137/137 testes, janela aberta via WSLg.

Ainda não testado: escala do Windows em 125%/150%, cliques reais nos botões Equalizar/1024x768 no Windows (só a inicialização e o salvamento com a tecla S foram conferidos visualmente), e imagem com acento no caminho.

## Licenças

- Código do projeto: trabalho acadêmico dos autores.
- Fonte DejaVu Sans: licença Bitstream Vera / domínio público, em `assets/fonts/LICENSE-DejaVu.txt`.
- SDL3, SDL_image e SDL_ttf: licença zlib.
