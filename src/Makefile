# ============================================================
# Cotucatronics OS — Makefile v0.1.0
#
#   make BOARD=daisy_seed      compila
#   make program-dfu           grava (Seed em modo bootloader)
#   make clean
#
# Trocar de placa = escrever hal_<board>.cpp e passar BOARD=<board>.
# Nada de DaisySP aqui: o projeto não usa.
# ============================================================

TARGET ?= cotucatronics
BOARD  ?= daisy_seed

# Clone da libDaisy. Sobrescreva na linha de comando se estiver
# em outro lugar:  make LIBDAISY_DIR=/caminho/pra/libDaisy
LIBDAISY_DIR ?= $(HOME)/dev/libDaisy

HAL_SRC = hal_$(BOARD).cpp

CPP_SOURCES = \
main.cpp \
$(HAL_SRC)

$(info Cotucatronics Makefile v0.1.0 — BOARD=$(BOARD) LIBDAISY_DIR=$(LIBDAISY_DIR))

# ---- checagens com mensagem legível, em vez de erro críptico ----
ifeq ($(wildcard $(HAL_SRC)),)
$(error HAL nao encontrado: $(HAL_SRC). Placas disponiveis: $(patsubst hal_%.cpp,%,$(wildcard hal_*.cpp)))
endif

ifeq ($(wildcard $(LIBDAISY_DIR)/core/Makefile),)
$(error libDaisy nao encontrada em $(LIBDAISY_DIR). Clone com: git clone --recurse-submodules https://github.com/electro-smith/libDaisy $(LIBDAISY_DIR))
endif

ifeq ($(wildcard $(LIBDAISY_DIR)/build/libdaisy.a),)
$(error libdaisy.a nao existe. Compile a lib antes: make -C $(LIBDAISY_DIR))
endif

# A libDaisy linka com --specs=nano.specs, e o printf do newlib-nano vem
# SEM suporte a float: todo %f vira string vazia, calado, sem warning.
# O manifest e o painel serial são todos float, então sem isto o "list"
# imprime "norm= real=". -u força a entrada da versão com float.
# Custa ~7 KB de flash.
# Definido ANTES do include porque o core/Makefile usa "LDFLAGS ?=".
LDFLAGS = -u _printf_float

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
