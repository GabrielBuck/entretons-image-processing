# =============================================================================
# Entretons - Makefile
#
# GCC + GNU Make, para Windows 10/11 (MinGW-w64) e Linux / WSL Ubuntu.
#
# Uso rápido:
#   make                 compila o executável "entretons" (entretons.exe no Windows)
#   make run IMG=foto.png  compila e executa com a imagem indicada
#   make test            compila e roda os testes automáticos (sem abrir janelas)
#   make dlls            (Windows) copia as DLLs do SDL para a pasta do projeto
#   make clean           remove objetos e executáveis
#
# Como o Makefile encontra as bibliotecas SDL3, SDL3_image e SDL3_ttf:
#   1) Se você informar SDL_PREFIX (ou SDL3_DIR / SDL3_IMAGE_DIR / SDL3_TTF_DIR),
#      ele usa <pasta>/include e <pasta>/lib. Exemplo:
#        make SDL_PREFIX=C:/libs/SDL3
#        make SDL3_DIR=C:/sdl3 SDL3_IMAGE_DIR=C:/sdl3_image SDL3_TTF_DIR=C:/sdl3_ttf
#   2) Senão, se o pkg-config enxergar sdl3, sdl3-image e sdl3-ttf, ele os usa
#      (caso típico do MSYS2 e de instalações em /usr ou /usr/local no Linux).
#   3) Senão, tenta -lSDL3 -lSDL3_image -lSDL3_ttf direto (caminhos padrão do GCC).
#
# Para não digitar os caminhos a cada vez, crie um arquivo "config.mk" (não é
# versionado) ao lado deste Makefile com, por exemplo:
#   SDL3_DIR       = C:/libs/SDL3-3.4.16/x86_64-w64-mingw32
#   SDL3_IMAGE_DIR = C:/libs/SDL3_image-3.4.6/x86_64-w64-mingw32
#   SDL3_TTF_DIR   = C:/libs/SDL3_ttf-3.2.2/x86_64-w64-mingw32
# =============================================================================

TARGET := entretons
CC     := gcc

ifeq ($(OS),Windows_NT)
  EXE  := .exe
  NULL := nul
else
  EXE  :=
  NULL := /dev/null
endif

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(OBJ:.o=.d)

# Módulos usados pelos testes (tudo, menos a GUI e o main)
CORE_SRC := src/image.c src/histogram.c src/equalization.c src/utils.c
TEST_BIN := tests/test_core$(EXE)

CFLAGS  ?= -O2
override CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -MMD -MP
override CPPFLAGS += -Isrc
LDLIBS  := -lm

# ----- Localização das bibliotecas SDL ---------------------------------------
-include config.mk

ifdef SDL_PREFIX
  SDL3_DIR       ?= $(SDL_PREFIX)
  SDL3_IMAGE_DIR ?= $(SDL_PREFIX)
  SDL3_TTF_DIR   ?= $(SDL_PREFIX)
endif

SDL_DIRS := $(sort $(SDL3_DIR) $(SDL3_IMAGE_DIR) $(SDL3_TTF_DIR))
SDL_PC   := sdl3 sdl3-image sdl3-ttf

ifneq ($(strip $(SDL_DIRS)),)
  override CPPFLAGS += $(foreach d,$(SDL_DIRS),-I$(d)/include)
  override LDFLAGS  += $(foreach d,$(SDL_DIRS),-L$(d)/lib)
  ifneq ($(OS),Windows_NT)
    override LDFLAGS += $(foreach d,$(SDL_DIRS),-Wl,-rpath,$(d)/lib)
  endif
  LDLIBS := -lSDL3_ttf -lSDL3_image -lSDL3 $(LDLIBS)
else ifeq ($(shell pkg-config --exists $(SDL_PC) 2>$(NULL) && echo yes),yes)
  override CPPFLAGS += $(shell pkg-config --cflags $(SDL_PC))
  LDLIBS := $(shell pkg-config --libs $(SDL_PC)) $(LDLIBS)
else
  LDLIBS := -lSDL3_ttf -lSDL3_image -lSDL3 $(LDLIBS)
endif

# ----- Regras ----------------------------------------------------------------
.PHONY: all run test dlls clean

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)$(EXE)
	./$(TARGET)$(EXE) $(IMG)

$(TEST_BIN): tests/test_core.c $(CORE_SRC)
	$(CC) $(CPPFLAGS) $(filter-out -MMD -MP,$(CFLAGS)) $^ -o $@ $(LDFLAGS) $(LDLIBS)

test: $(TEST_BIN)
	$(TEST_BIN)

# Windows: o executável precisa encontrar as DLLs do SDL. Copia as que estão em
# <pasta>/bin (ou <pasta>/lib) das pastas informadas em SDL_PREFIX / SDL3_*_DIR.
dlls:
ifeq ($(strip $(SDL_DIRS)),)
	@echo Informe SDL_PREFIX, ou SDL3_DIR + SDL3_IMAGE_DIR + SDL3_TTF_DIR, para copiar as DLLs.
else ifeq ($(findstring cmd,$(SHELL)),cmd)
	-$(foreach d,$(SDL_DIRS),copy /Y "$(subst /,\,$(d))\bin\*.dll" . >nul & ) rem
else
	-$(foreach d,$(SDL_DIRS),cp -f $(d)/bin/*.dll . 2>$(NULL) ; ) true
endif

clean:
ifeq ($(findstring cmd,$(SHELL)),cmd)
	-del /Q /F src\*.o src\*.d $(TARGET).exe tests\test_core.exe 2>nul
else
	-rm -f src/*.o src/*.d $(TARGET) $(TARGET).exe $(TEST_BIN)
endif

-include $(DEP)
