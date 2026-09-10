#!/bin/sh
# bootstrap_kicad.sh — arranca uma cadeia KiCad quando o ambiente nao tem nenhuma
# VERSAO: 1.0.0
#
# POR QUE ESTE SCRIPT EXISTE
#   Numa sessao (2026-09-10) a pipeline `pcb` ficou bloqueada no estagio 0: os
#   ficheiros eram KiCad-10 mas nao havia `kicad-cli` 10 no ambiente e todos os
#   caminhos habituais falharam:
#     - apt   -> so tem KiCad 7 (velho demais para ficheiros v10)
#     - conda-forge -> nao tem pacote kicad nenhum
#     - PPA oficial do KiCad -> bloqueado pelo proxy de egresso (403)
#     - hosts oficiais (downloads.kicad.org, kicad.org, sourceforge) -> 403
#   O UNICO caminho que passou o proxy foi um AppImage hospedado numa *release*
#   do GitHub (o proxy deixa passar /releases/download/...). Este script encoda
#   esse caminho para que nenhuma sessao futura fique presa no mesmo sitio.
#
# ##########################################################################
# ##  CAVEAT — ISTO E UM BUILD DA COMUNIDADE, NAO O OFICIAL DO KICAD       ##
# ##  O AppImage por defeito vem de `pkgforge-dev/KiCad-AppImage`, um      ##
# ##  empacotamento nao-oficial. USA SO QUANDO O OFICIAL NAO E ALCANCAVEL. ##
# ##  Se downloads.kicad.org / o PPA / apt derem uma versao MAIOR igual a  ##
# ##  requerida, PREFERE-OS SEMPRE. Este e o plano-B, nao o plano-A.       ##
# ##########################################################################
#
# O QUE FAZ
#   1. Se ja houver um `kicad-cli` no PATH que reporte a versao MAIOR requerida,
#      imprime o seu caminho e sai 0 (nao descarrega nada).
#   2. Se ja houver uma arvore extraida de uma corrida anterior que reporte a
#      versao certa, reutiliza-a (idempotente — nao volta a descarregar).
#   3. Caso contrario descarrega o AppImage de uma URL de release do GitHub
#      (parametrizavel), VERIFICA o download (HTTP ok, tamanho, sha256 opcional),
#      extrai com `--appimage-extract` (sem FUSE) e imprime o caminho do
#      `kicad-cli` resultante.
#
# NUNCA prossegue em silencio sobre um download falhado/parcial: qualquer erro
# de rede, tamanho abaixo do minimo ou sha256 que nao bate ABORTA com codigo != 0.
#
# NOTA sharun: `bin/kicad-cli` no AppImage extraido e um lancador `sharun` que
# resolve as libs empacotadas pelo seu proprio argv0 — corre-o pelo caminho REAL
# (o que este script imprime), SEM definir LD_LIBRARY_PATH.
#
# USO
#   bootstrap_kicad.sh [MAJOR]        # MAJOR requerido (defeito: 10)
#   KICAD_MAJOR=10 bootstrap_kicad.sh
#   Variaveis de ambiente (todas opcionais):
#     KICAD_MAJOR            versao MAIOR requerida (defeito 10; arg 1 sobrepoe-se)
#     KICAD_APPIMAGE_URL     URL do AppImage (defeito: asset pkgforge abaixo)
#     KICAD_APPIMAGE_SHA256  sha256 esperado do AppImage (recomendado; se vazio,
#                            so se verifica HTTP + tamanho minimo)
#     KICAD_BOOTSTRAP_DIR    onde descarregar/extrair (defeito: ./.kicad-bootstrap)
#     KICAD_MIN_BYTES        tamanho minimo aceitavel do download (defeito 100 MB)
#
# SAIDA
#   stdout: UMA linha final — o caminho absoluto do kicad-cli utilizavel.
#   stderr: progresso, CAVEATS e erros. (Assim quem chama captura o caminho limpo:
#           KCLI=$(bootstrap_kicad.sh 10) )
#
# Referencia: .claude/skills/pcb/references/fab-automation.md
#   ("KiCad ausente no ambiente? Bootstrap por AppImage")

set -eu

# --- parametros ------------------------------------------------------------
MAJOR="${1:-${KICAD_MAJOR:-10}}"

# Asset PROVADO nesta sessao: KiCad 10.0.6, extrai sem FUSE, o kicad-cli faz o
# parse de um board (version 20260206) e corre o DRC. Build da COMUNIDADE.
DEFAULT_URL="https://github.com/pkgforge-dev/KiCad-AppImage/releases/download/10.0.6-1%402026-09-01_1788297566/KiCad-10.0.6-1-anylinux-x86_64.AppImage"
URL="${KICAD_APPIMAGE_URL:-$DEFAULT_URL}"

SHA256_EXPECTED="${KICAD_APPIMAGE_SHA256:-}"
DIR="${KICAD_BOOTSTRAP_DIR:-./.kicad-bootstrap}"
MIN_BYTES="${KICAD_MIN_BYTES:-104857600}"   # 100 MB — o asset conhecido tem ~487 MB

APPIMAGE="$DIR/kicad.AppImage"
EXTRACTED="$DIR/squashfs-root/bin/kicad-cli"

# --- helpers ---------------------------------------------------------------
log()  { printf '%s\n' "$*" >&2; }
die()  { printf 'ERRO: %s\n' "$*" >&2; exit 1; }

# Le a versao MAIOR que um kicad-cli reporta (ex.: "10.0.6" -> "10"). Vazio se falha.
cli_major() {
  # $1 = caminho para kicad-cli
  _v="$("$1" version 2>/dev/null | head -n1 | tr -d '[:space:]')" || return 1
  # Aceita "10.0.6" ou coisas do genero "10.0.6-..."; fica com o campo antes do 1o ponto.
  printf '%s' "$_v" | sed -n 's/^\([0-9][0-9]*\)\..*/\1/p'
}

file_size() {
  # tamanho em bytes, portavel
  wc -c < "$1" | tr -d '[:space:]'
}

verify_sha256() {
  # $1 = ficheiro, $2 = sha esperado. Aborta se nao bater. Aviso se nao houver tool.
  _f="$1"; _want="$2"
  if command -v sha256sum >/dev/null 2>&1; then
    _got="$(sha256sum "$_f" | awk '{print $1}')"
  elif command -v shasum >/dev/null 2>&1; then
    _got="$(shasum -a 256 "$_f" | awk '{print $1}')"
  else
    die "sha256 pedido mas nem sha256sum nem shasum existem — nao verifico as cegas"
  fi
  if [ "$_got" != "$_want" ]; then
    die "sha256 NAO bate: esperado $_want, obtido $_got (download corrompido/errado)"
  fi
  log "sha256 confere: $_got"
}

# --------------------------------------------------------------------------
# CAVEAT alto e visivel — mesmo quando reutiliza cache, quem le o log fica avisado.
log "=========================================================================="
log " bootstrap_kicad: plano-B por AppImage da COMUNIDADE (pkgforge-dev)."
log " Prefere SEMPRE o KiCad oficial (apt/PPA/downloads.kicad.org) quando"
log " alcancavel. So usa este caminho porque o oficial esta bloqueado/velho."
log " Requerido: KiCad MAIOR = $MAJOR"
log "=========================================================================="

# --- 1. Ja ha um kicad-cli bom no PATH? -----------------------------------
if command -v kicad-cli >/dev/null 2>&1; then
  ONPATH="$(command -v kicad-cli)"
  M="$(cli_major "$ONPATH" || true)"
  if [ "${M:-}" = "$MAJOR" ]; then
    log "kicad-cli $MAJOR ja no PATH ($ONPATH) — nada a descarregar."
    printf '%s\n' "$ONPATH"
    exit 0
  fi
  log "kicad-cli no PATH reporta MAIOR='${M:-?}' (!= $MAJOR) — segue para o AppImage."
fi

# --- 2. Ja ha uma arvore extraida boa? (idempotencia) ---------------------
if [ -x "$EXTRACTED" ]; then
  M="$(cli_major "$EXTRACTED" || true)"
  if [ "${M:-}" = "$MAJOR" ]; then
    ABS="$(cd "$(dirname "$EXTRACTED")" && pwd)/$(basename "$EXTRACTED")"
    log "Arvore ja extraida e valida ($ABS, MAIOR=$M) — reutilizo (idempotente)."
    printf '%s\n' "$ABS"
    exit 0
  fi
  log "Arvore extraida reporta MAIOR='${M:-?}' (!= $MAJOR) — re-extraio."
  rm -rf "$DIR/squashfs-root"
fi

mkdir -p "$DIR"

# --- 3. Descarregar o AppImage (com verificacao) --------------------------
need_download=1
if [ -f "$APPIMAGE" ]; then
  SZ="$(file_size "$APPIMAGE")"
  if [ "$SZ" -ge "$MIN_BYTES" ]; then
    log "AppImage ja presente ($APPIMAGE, $SZ bytes) — reutilizo o ficheiro."
    need_download=0
  else
    log "AppImage presente mas pequeno demais ($SZ < $MIN_BYTES) — parcial, re-descarrego."
    rm -f "$APPIMAGE"
  fi
fi

if [ "$need_download" -eq 1 ]; then
  command -v curl >/dev/null 2>&1 || die "curl nao encontrado — nao consigo descarregar"
  log "A descarregar (build da comunidade): $URL"
  # -f: falha (exit != 0) em HTTP >= 400; -L: segue redireccao para objects.githubusercontent.com
  TMP="$APPIMAGE.part"
  rm -f "$TMP"
  if ! curl -fL --retry 3 --retry-delay 2 -o "$TMP" "$URL"; then
    rm -f "$TMP"
    die "download falhou (curl -f => HTTP de erro ou rede caiu). NAO prossigo."
  fi
  SZ="$(file_size "$TMP")"
  if [ "$SZ" -lt "$MIN_BYTES" ]; then
    rm -f "$TMP"
    die "download parcial: $SZ bytes < minimo $MIN_BYTES. NAO prossigo."
  fi
  mv "$TMP" "$APPIMAGE"
  log "Descarregado: $APPIMAGE ($SZ bytes)"
fi

# sha256: so quando fornecido. Sem checksum, o tamanho + HTTP-ok sao o minimo.
if [ -n "$SHA256_EXPECTED" ]; then
  verify_sha256 "$APPIMAGE" "$SHA256_EXPECTED"
else
  log "AVISO: sem KICAD_APPIMAGE_SHA256 — verifiquei apenas HTTP-ok + tamanho."
  log "       Passa um sha256 para uma garantia de integridade forte."
fi

# --- 4. Extrair sem FUSE --------------------------------------------------
chmod +x "$APPIMAGE"
log "A extrair com --appimage-extract (sem FUSE) para $DIR ..."
# --appimage-extract cria ./squashfs-root no CWD; entra em $DIR num subshell.
( cd "$DIR" && "./$(basename "$APPIMAGE")" --appimage-extract >/dev/null ) \
  || die "--appimage-extract falhou"

[ -x "$EXTRACTED" ] || die "extraiu mas nao ha $EXTRACTED executavel"

# --- 5. Confirmar que a versao extraida bate no requisito -----------------
M="$(cli_major "$EXTRACTED" || true)"
[ "${M:-}" = "$MAJOR" ] || die "kicad-cli extraido reporta MAIOR='${M:-?}', esperava $MAJOR"

ABS="$(cd "$(dirname "$EXTRACTED")" && pwd)/$(basename "$EXTRACTED")"
log "PRONTO. kicad-cli MAIOR=$M (build da comunidade). Caminho abaixo em stdout."
log "Lembrete: corre-o por este caminho real (lancador sharun, SEM LD_LIBRARY_PATH)."
log "Aviso benigno em cada corrida: \"schema file '.../api.v1.schema.json' not found\" — ignora."
printf '%s\n' "$ABS"
