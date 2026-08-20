#!/bin/bash
# ccmap-pdf v0.1.0 — gera docs/ccmap.pdf a partir de docs/ccmap.html
#
# Usa o Chrome headless porque e o unico conversor HTML->PDF que esta
# nesta maquina. Sem dependencia de toolchain: o PDF fica no repo, entao
# ninguem precisa rodar isto pra ter a folha na mao.
#
# A FONTE DA VERDADE E A PLACA, nao este arquivo. Ao mudar o mapa de CC,
# rodar `midi` na serial, conferir contra o ccmap.html, e so entao gerar.
set -e
echo "ccmap-pdf v0.1.0"
cd "$(dirname "$0")"
CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
[ -x "$CHROME" ] || { echo "ERRO: Chrome nao encontrado em $CHROME"; exit 1; }
[ -f ccmap.html ] || { echo "ERRO: ccmap.html nao encontrado"; exit 1; }
"$CHROME" --headless=new --disable-gpu --no-pdf-header-footer \
  --print-to-pdf="$PWD/ccmap.pdf" "file://$PWD/ccmap.html" 2>/dev/null
PAGS=$(python3 -c "import re;print(len(re.findall(rb'/Type\s*/Page[^s]',open('ccmap.pdf','rb').read())))")
echo "gerado: $PWD/ccmap.pdf  ($PAGS pagina(s))"
[ "$PAGS" = "1" ] || echo "AVISO: passou de uma folha — o cartao de referencia deveria caber em uma"
