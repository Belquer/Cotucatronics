# ============================================================
# Cotucatronics OS — Makefile v0.2.0
#
#   make BOARD=daisy_seed      compila
#   make program-boot          grava o bootloader (Seed em DFU da STM)
#   make program-dfu           grava o app na QSPI (Seed no bootloader)
#   make clean
#
# Trocar de placa = escrever hal_<board>.cpp e passar BOARD=<board>.
# Nada de DaisySP aqui: o projeto não usa.
# ============================================================

TARGET ?= cotucatronics
BOARD  ?= daisy_seed

# ---- onde o app roda ----
# BOOT_QSPI: o app vive nos 8 MB da QSPI e os 128 KB de flash interna
# ficam pro bootloader da Electrosmith. Migramos porque o app já ocupava
# 94% dos 128 KB internos com um único módulo RNBO — o segundo não
# caberia. Na QSPI o teto some como problema.
#
# Custo: o M7 executa da QSPI mais devagar que da flash interna (mitigado
# pelo I-cache) e gravar passa a ter dois passos, um deles único
# (program-boot). Volte pra flash interna com:
#     make APP_TYPE=BOOT_NONE
# — mas aí é preciso regravar por DFU da STM (BOOT+RESET), porque o
# bootloader some junto.
APP_TYPE ?= BOOT_QSPI

# Clone da libDaisy. Sobrescreva na linha de comando se estiver
# em outro lugar:  make LIBDAISY_DIR=/caminho/pra/libDaisy
LIBDAISY_DIR ?= $(HOME)/dev/libDaisy

HAL_SRC = hal_$(BOARD).cpp

# Motor de DSP. module_rnbo.cpp e o UNICO que ve headers do RNBO;
# ele inclui o .cpp gerado como fonte, entao o export nao entra
# aqui como unidade de compilacao separada.
MODULE_SRC ?= module_rnbo.cpp
RNBO_DIR   ?= modules/delay/rnbo_export

CPP_SOURCES = \
main.cpp \
$(MODULE_SRC) \
$(HAL_SRC)

# Headers do RNBO. So o module_rnbo.cpp usa, mas o -I e global.
C_INCLUDES += -I$(RNBO_DIR)/rnbo -I$(RNBO_DIR)/rnbo/common -I$(RNBO_DIR)

# O RNBO usa <cmath>/<algorithm> com a STL; nada de excecoes nem RTTI,
# que o Makefile da libDaisy ja desliga.
CPPFLAGS += -Wno-unused-parameter -Wno-sign-compare

$(info Cotucatronics Makefile v0.2.0 — BOARD=$(BOARD) APP_TYPE=$(APP_TYPE) LIBDAISY_DIR=$(LIBDAISY_DIR))

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
