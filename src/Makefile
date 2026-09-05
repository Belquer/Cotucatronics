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
# 94% dos 128 KB internos com um unico modulo externo — o segundo nao
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
# ANTES do include — mesmo motivo do LDFLAGS mais abaixo.
#
# Existe porque escrever na QSPI exige tira-la do modo memory-mapped, e
# o app executa DE LA — inclusive a tabela de vetores e os 51 handlers.
# A secao `.itcm_text` poe a rotina de escrita numa RAM que nao depende
# da QSPI. Certidao de origem e o porque completo no topo do .lds.
LDSCRIPT = $(shell pwd)/cotucatronics_qspi.lds

# O ATRIBUTO E O LINKER SAO UMA COISA SO. Quem usa `.itcm_text` precisa
# de um linker que a defina; separa-los quebra o build com "undefined
# reference to _sitcm", que foi como eu descobri na rodada do .axi_bss.
C_DEFS += -DCOTUCA_TEM_ITCM

# Clone da libDaisy. Sobrescreva na linha de comando se estiver
# em outro lugar:  make LIBDAISY_DIR=/caminho/pra/libDaisy
LIBDAISY_DIR ?= $(HOME)/dev/libDaisy

HAL_SRC = hal_$(BOARD).cpp

# O SISTEMA DE MODULOS EXTERNOS SAIU INTEIRO A 20/08, quando o delay —
# o ultimo consumidor — virou C++ no shell (delay.h). NAO ha aqui
# nenhum MODULE_SRC nem diretorio de export, e os .cpp de modulo nao
# existem no repo: ate 04/09 este bloco mandava devolve-los, o que era
# receita para ficheiros que nao estao ca.
#
# O que sobrevive como decisao:
#   - block_size 48. O 256 era exigencia da FFT de um modulo externo e
#     saiu com ele. Bloco e manifest mudam JUNTOS.
#   - O freeze de hoje e o freeze.h, granular em C++ no shell. O de
#     motor externo foi reprovado na etapa 5 do IFA10000 (7 349 us
#     contra 2 667 de orcamento) e nunca voltou.
#   - O slot Fx2 fica VAZIO: passthrough silencioso, nao erro.
#
# Reviver exige ir ao git buscar tudo — ver FEATURES.md, que tem o
# caminho de volta e o commit.

CPP_SOURCES = \
	qspi_raw.cpp \
main.cpp \
$(HAL_SRC)

# O -I e global. Ja nao ha include de motor externo: o runtime saiu
# inteiro em 20/08 quando o delay virou C++ no shell. O instrumento
# nao depende de nenhum motor de fora.
C_INCLUDES += -I.

# O shell usa <cmath>/<algorithm> com a STL; nada de excecoes nem RTTI,
# que o Makefile da libDaisy ja desliga.
CPPFLAGS += -Wno-unused-parameter -Wno-sign-compare

$(info Cotucatronics Makefile v0.4.1 — BOARD=$(BOARD) APP_TYPE=$(APP_TYPE) LIBDAISY_DIR=$(LIBDAISY_DIR))

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
# modulos externos: separar por arquivo exigiria sobrepor a regra de
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
# o cabecalho de alocacao do modulo ficou meses fora do repo sem ninguem notar — o
# build local funcionava porque o arquivo estava na arvore. Conferir
# a arvore nao e conferir o repositorio.
.PHONY: check print-cpp-sources
check:
	@./check_repo.sh

print-cpp-sources:
	@echo $(CPP_SOURCES)

# ---- check-docs: da testemunha a documentacao ----
#
# O compilador cobra os simbolos; ate 22/08 nada cobrava a doc. O audit
# de deriva achou DOZE casos — params fantasmas citados por meses,
# contadores lidos-sem-escritor e escritos-sem-leitor, vestigios de uma
# reestruturacao grande.
#
# A causa raiz vale a regra: TODA REESTRUTURACAO TEM TRES FRENTES —
# codigo, documentacao e simbolos. O compilador cobra a primeira; nada
# cobra as outras duas, entao o `make check-docs` cobra.
#
# Habito automatizado vence habito lembrado: a disciplina ja falhou
# aqui, e a checagem manual recem-escrita nao teria pego o segundo caso.
check-docs:
	@python3 $(shell pwd)/check_docs.py

.PHONY: check-docs
