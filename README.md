# Entretons

Entretons é o Projeto 1 da disciplina de Computação Visual (07G), do curso de Ciência da Computação da Universidade Presbiteriana Mackenzie, com o professor André Kishimoto. É uma ferramenta de linha de comando para analisar e processar imagens em **C**, usando **SDL3**, **SDL_image** e **SDL_ttf**.

Autores: Gabriel Nottoli Buck e Julia Andrade.

O programa recebe uma imagem, converte para escala de cinza quando necessário, mostra o histograma e as estatísticas em uma segunda janela e deixa equalizar o histograma, voltar à imagem original, alternar entre a resolução original e 1024 x 768, e salvar o resultado.

| Imagem original | Após equalizar |
|---|---|
| ![Interface com a imagem original](docs/interface_original.png) | ![Interface com a imagem equalizada](docs/interface_equalizada.png) |

## Funcionalidades

1. Recebe o caminho da imagem pela linha de comando: `entretons caminho_da_imagem.ext`.
2. Valida o argumento, a existência do arquivo e o formato, e mostra uma mensagem de erro clara no terminal quando algo dá errado.
3. Detecta se a imagem é colorida ou já está em escala de cinza. Se for colorida, converte com `Y = 0,2125 R + 0,7154 G + 0,0721 B`.
4. Abre duas janelas:
   - **principal**: exibe a imagem atual. Começa em 1024 x 768 px, centralizada no monitor principal; se a imagem exceder a resolução da tela nesse tamanho, o canto superior esquerdo vai para (0, 0).
   - **secundária** (filha da principal, 440 x 720 px, tamanho fixo, em (0, 0)): histograma, estatísticas e os dois botões de ação.
5. Histograma de 256 níveis com barras proporcionais e uma marca visual da média, além de média, desvio padrão e a classificação da imagem.
6. **Equalizar** / **Restaurar original**: a versão original em cinza fica guardada em memória, então restaurar não recarrega o arquivo.
7. **Exibir 1024 x 768** / **Tamanho original**: alterna entre a resolução original e 1024 x 768 (interpolação bilinear). A cada troca, a janela principal é redimensionada e recentralizada, ou movida para (0, 0) se não couber na tela.
8. Imagem, histograma e estatísticas são sempre atualizados juntos.
9. Tecla **S** salva a imagem exibida em `output_image.png`, sobrescrevendo o arquivo se ele já existir, e informa o resultado no terminal e na própria janela.

Controles: mouse nos botões da janela secundária, **S** para salvar, **Esc** (ou fechar qualquer janela) para sair.

Formatos aceitos: qualquer um que a SDL_image consiga abrir (PNG, JPEG, BMP, GIF, entre outros).

## Integrantes e contribuições

| Integrante | RA |
|---|---|
| Gabriel Nottoli Buck | 10425384 |
| Julia Andrade | 10427829 |

O projeto foi desenvolvido em conjunto pelos dois integrantes, que participaram do design da interface, da implementação das funcionalidades exigidas pelo enunciado e dos testes, revisando o trabalho um do outro ao longo do desenvolvimento.

## Requisitos

- GCC com suporte a C11 (o projeto é compilado com `-std=c11`; o enunciado pede C99 ou mais recente) e GNU Make.
- SDL3, SDL_image e SDL_ttf. Versões usadas: **SDL 3.4.16**, **SDL_image 3.4.6**, **SDL_ttf 3.2.2** — as mais recentes e estáveis disponíveis quando o projeto foi desenvolvido.

Ambientes de avaliação da disciplina: Windows 10/11 com GCC 15.1.0 e WSL Ubuntu 26.04 com GCC 15.2.0. O projeto foi de fato compilado e testado em Windows 11 (GCC 15.2.0) e em WSL Ubuntu 24.04.1 (GCC 13.3.0); veja a seção [Testes](#testes) para os detalhes.

## Compilação

O `Makefile` procura as bibliotecas nesta ordem:

1. pastas informadas por você (`SDL_PREFIX`, ou `SDL3_DIR` + `SDL3_IMAGE_DIR` + `SDL3_TTF_DIR`), cada uma com `include/` e `lib/`;
2. `pkg-config` (`sdl3`, `sdl3-image`, `sdl3-ttf`);
3. os caminhos padrão do GCC (`-lSDL3 -lSDL3_image -lSDL3_ttf`).

### Windows (GCC / MinGW-w64)

Baixe os pacotes de desenvolvimento para MinGW nas páginas de releases da SDL e informe a pasta `x86_64-w64-mingw32` de cada um. Para não digitar isso toda vez, crie um arquivo `config.mk` na raiz do projeto (ele não é versionado):

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

Se não tiver — foi o caso do Ubuntu 24.04, que ainda não empacota o SDL3 —, compile as bibliotecas do código-fonte para uma pasta local:

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

Algumas exigências do enunciado moldaram diretamente a implementação — o tamanho e a posição da janela principal (1024 x 768 inicial, centralizada, com fallback em (0, 0)) e a posição fixa da janela secundária em (0, 0) vêm de lá, não são escolhas do grupo.

O que ficou em aberto e o grupo decidiu por conta própria:

**Classificação por brilho** (média das intensidades, faixa dividida em terços): `< 85` escura, `85 a 169` média, `>= 170` clara.

**Classificação por contraste** (desvio padrão): `< 40` baixo, `40 a 69` médio, `>= 70` alto. Uma imagem com todos os níveis igualmente frequentes tem desvio de cerca de 74, então uma imagem bem equalizada cai em "alto".

Os limiares estão em constantes documentadas em `src/histogram.h` e são fáceis de ajustar.

**Equalização**: equalização clássica de histograma pela função de distribuição acumulada,
`s(k) = round(255 * (cdf(k) - cdf_min) / (N - cdf_min))`, onde `N` é o total de pixels e `cdf_min` o menor valor não nulo da CDF. Uma imagem com um único nível de cinza não é alterada. A versão equalizada é calculada uma vez e guardada.

**1024 x 768**: a imagem é interpolada bilinearmente para exatamente 1024 x 768, preenchendo toda a janela. Se a imagem não tiver proporção 4:3, ela fica esticada.

**Tamanho e identidade visual da janela secundária**: 440 x 720 px foi uma escolha do grupo (o enunciado só exige tamanho fixo). O enunciado sugere cores em tons de azul para os estados dos botões como exemplo; o grupo optou por um fundo grafite, textos em creme e acentos âmbar para diferenciar neutro, mouse em cima e clicado sem deixar a interface carregada.

**Transparência**: o canal alfa de PNGs com transparência é ignorado; vale o valor RGB do pixel.

**PNG cinza de 8 bits**: a SDL_image 3.4.6 entrega esses arquivos como índices com uma paleta deslocada em 1 nível (255 virava 254). O carregamento detecta esse caso e usa o índice diretamente, preservando os níveis originais. Há um teste para isso.

**Fonte**: DejaVu Sans, dentro do projeto (`assets/fonts`), localizada a partir da pasta do executável. O programa funciona independentemente das fontes instaladas no sistema operacional.

## Testes

```
make test
```

Roda 137 verificações: fórmula de conversão para cinza, detecção colorida/cinza, erros de carregamento (arquivo inexistente, pasta, arquivo que não é imagem), histograma e limiares, equalização (inclusive restaurar a original), redimensionamento, PNG salvo e recarregado e funções auxiliares. Rode a partir da pasta do projeto, pois alguns testes usam `assets/samples`.

Situação da validação: compilado sem avisos (`-Wall -Wextra -Wpedantic`) em três ambientes:

- **Linux** (Ubuntu 24.04, GCC 13.3): 137/137 testes, janelas em display virtual com cliques simulados, ASan/UBSan sem erros.
- **Windows 11** (GCC 15.2.0, MinGW-w64 via MSYS2, `mingw32-make`): 137/137 testes, `entretons.exe` executado com `assets/samples/paisagem_cores.png`, janela principal centralizada em 1024 x 768, janela secundária em (0, 0) com 440 x 720, acentos e "×" corretos com a fonte DejaVu Sans embutida, tecla S salvando `output_image.png`. As bibliotecas SDL3 3.4.16, SDL_image 3.4.6 e SDL_ttf 3.2.2 foram usadas nas versões `-devel-mingw` oficiais, testadas rodando `mingw32-make` tanto direto do PowerShell/cmd quanto do Git Bash/MSYS2.
- **WSL Ubuntu 24.04.1** (GCC 13.3.0; não foi possível confirmar o Ubuntu 26.04/GCC 15.2.0 usado na avaliação, pois é a distribuição instalada na máquina testada): SDL3, SDL_image e SDL_ttf compilados do fonte (não há pacotes `libsdl3-dev` no APT do 24.04), 137/137 testes, janela aberta via WSLg.

Ainda não testado: escala do Windows em 125%/150% e imagem com acento no caminho.

## Licenças

- Código do projeto: trabalho acadêmico dos autores.
- Fonte DejaVu Sans: licença Bitstream Vera / domínio público, em `assets/fonts/LICENSE-DejaVu.txt`.
- SDL3, SDL_image e SDL_ttf: licença zlib.
