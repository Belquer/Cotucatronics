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

# LINKER PROPRIO. O `?=` da libDaisy aceita sobrescrita, e tem que vir
# ANTES do include — mesmo motivo do LDFLAGS abaixo.
#
# Existe porque no BOOT_SRAM o `.bss` vai pra DTCMRAM (128 KB) e o
# `reverb_mem` sozinho tem 220 KB. A secao `.axi_bss` devolve dados
# grandes pra SRAM, que e onde ja moravam em BOOT_QSPI. Certidao de
# origem e o porque completo no topo do proprio .lds.
#LDSCRIPT = $(shell pwd)/cotucatronics_sram.lds
# O ATRIBUTO E O LINKER SAO UMA COISA SO. Quem usa `.axi_bss` precisa de
# um linker que a defina; sem este par, o build do BOOT_QSPI quebra com
# "undefined reference to _saxi_bss" — que foi como eu descobri.
#C_DEFS += -DCOTUCA_TEM_AXI_BSS

# Clone da libDaisy. Sobrescreva na linha de comando se estiver
# em outro lugar:  make LIBDAISY_DIR=/caminho/pra/libDaisy
LIBDAISY_DIR ?= $(HOME)/dev/libDaisy

HAL_SRC = hal_$(BOARD).cpp

# Motores de DSP. Cada module_rnbo_*.cpp inclui o .cpp gerado como
# fonte, dentro de um namespace proprio — os exports nao entram aqui
# como unidades de compilacao separadas.
# O freeze saiu do build: nao cabe no prazo do bloco (7349 us medidos
# contra 2667 de orcamento) e ocupava 89% do pool de SRAM, que o
# vocoder precisa. O arquivo continua no repo — module_rnbo_freeze.cpp
# aponta pra modules/xxxfreeze/ — pra voltar quando o patch emagrecer.
# VOCODER FORA (2026-08-18, decisao de ouvido do dB). Sobra o delay no
# slot Instrument, e o slot Fx2 fica VAZIO — passthrough silencioso,
# nao erro. O block_size voltou pra 48 no manifest na mesma leva: o 256
# era exigencia da FFT do vocoder e saiu junto com ela. Os dois
# arquivos mudam JUNTOS, e e por isso que a nota esta nos dois.
#
# Pra reviver: devolve module_rnbo_vocoder.cpp aqui e poe 256 la. O
# export, o wrapper, os binds e a cerca de La menor dos buzios seguem
# no repo, comentados e conferidos — a volta e de minutos.
#
# O FREEZE que entrou no lugar NAO e este module_rnbo_freeze.cpp: e o
# freeze.h, granular em C++ no shell. O RNBO foi reprovado na etapa 5
# do IFA10000 (7 349 us contra 2 667 de orcamento) e segue fora.

CPP_SOURCES = \
main.cpp \
$(HAL_SRC)

# So os modules_rnbo_*.cpp usam, mas o -I e global.
# Sem -I do RNBO: o runtime saiu inteiro em 20/08 quando o delay
# virou C++ no shell. O instrumento nao depende mais de RNBO.
C_INCLUDES += -I.

# O RNBO usa <cmath>/<algorithm> com a STL; nada de excecoes nem RTTI,
# que o Makefile da libDaisy ja desliga.
CPPFLAGS += -Wno-unused-parameter -Wno-sign-compare

$(info Cotucatronics Makefile v0.4.0 — BOARD=$(BOARD) APP_TYPE=$(APP_TYPE) LIBDAISY_DIR=$(LIBDAISY_DIR))

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
# ============================================================
# MODO BANCADA — alavanca 1 de 3
# O core/Makefile da libDaisy usa "OPT ?= -O2", entao esta
# atribuicao vence. -O3 vale pro projeto INTEIRO, nao so pros
# modulos RNBO: separar por arquivo exigiria sobrepor a regra de
# padrao do Makefile da libDaisy, que nao expoe gancho pra isso.
# O delta medido no delay serve de calibracao do que -O3 rende.
# ============================================================
OPT = -O3

# Definido ANTES do include porque o core/Makefile usa "LDFLAGS ?=".
LDFLAGS = -u _printf_float

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# ---- checagem de repo ----
# `make check` responde: um CLONE LIMPO compila? Existe porque
# module_rnbo_alloc.h ficou meses fora do repo sem ninguem notar — o
# build local funcionava porque o arquivo estava na arvore. Conferir
# a arvore nao e conferir o repositorio.
.PHONY: check print-cpp-sources
check:
	@./check_repo.sh

print-cpp-sources:
	@echo $(CPP_SOURCES)
