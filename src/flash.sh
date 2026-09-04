#!/bin/bash
# flash v0.4.0 — grava SO se o build passou, confere a versao na placa, e
# REPOE o estado que a gravacao apagou (ver `estado.py`).
#
# Existe porque `make program-dfu` grava o BINARIO EM DISCO, que pode ser
# de um build anterior se o atual falhou. Em 20/08 isso me fez medir o
# gate no firmware velho e ler o defeito como se fosse o conserto.
# Ferramenta que reporta sucesso sobre artefato velho e instrumento que
# mente — e o antidoto e a regra da casa: conferir a versao NO FONTE E
# NA PLACA.
set -e
cd "$(dirname "$0")"
VER=$(grep -oE '"[0-9]+\.[0-9]+\.[0-9]+"' main.cpp | head -1 | tr -d '"')
# ⚠️ UMA VERSAO SO. Este `echo` ficou em v0.1.4 enquanto o cabecalho ia em
# v0.3.0, e o `check_docs` nao apanhou porque nao varria `.sh`. Duas
# versoes no mesmo ficheiro sao duas afirmacoes sobre a mesma coisa.
echo "flash v0.4.0 — fonte diz v$VER"
# CODIGO DE SAIDA, nao presenca de linha. A primeira versao procurava
# "objcopy -O binary" na saida e dava BUILD FALHOU num build ja
# ATUALIZADO, que nao reimprime nada. A guarda contra instrumento que
# mente virou instrumento que mente.
set +e
make -j8 > /tmp/build.log 2>&1
RC=$?
set -e
if [ $RC -ne 0 ]; then
    echo "BUILD FALHOU (rc=$RC) — nao gravo nada:"
    grep -E " error|Error" /tmp/build.log | head -5
    exit 1
fi
echo "  build ok"
# GUARDAR O ESTADO ANTES DO DFU, nao depois: dali a dois passos a placa
# esta em bootloader e nao responde a `list`. Pedido do dB, 04/09 —
# "ao gravar reponha os valores" — e a §2c da bancada, que dizia isto
# desde 20/08 e vivia so na minha cabeca. Agora vive no script.
ESTADO=build/estado_antes.txt
python3 estado.py guardar "$ESTADO" || true

# A placa pode JA estar em DFU (de uma tentativa anterior que morreu no
# meio). Perguntar antes de mandar `dfu go` numa serial que nao existe.
if dfu-util -l 2>/dev/null | grep -q "Found DFU"; then
    echo "  ja esta em DFU"
else
    python3 - <<'PY' 2>/dev/null || true
import sys, time, serial
# A porta certa e a que RESPONDE, e quem sabe perguntar e o `porta_daisy`,
# que existe desde 25/08 para isto. Uma copia local aqui seria a terceira
# implementacao da mesma coisa. Ver battle_scars 15bd.
sys.path.insert(0, '.')
try:
    import porta_daisy
    p, _v = porta_daisy.achar()   # devolve o PAR (porta, versao)
except Exception as e:
    p = None
    print('  porta_daisy: %s (a placa pode ja estar em DFU, ou presa)' % e)
if p:
    print('  dfu go -> ' + p)
    s = serial.Serial(p, 115200, timeout=1); time.sleep(0.4)
    s.write(b'dfu go\n'); time.sleep(2.0); s.close()
PY
    sleep 3
fi
OK=0
for i in $(seq 1 40); do
    OUT=$(make program-dfu 2>&1) || true
    case "$OUT" in
        *"File downloaded successfully"*) OK=1; echo "  gravado (tentativa $i)"; break;;
        *"No DFU capable USB device"*)    : ;;   # ainda nao entrou; espera
        *) echo "  tentativa $i: $(echo "$OUT" | grep -iE 'error|dfu-util:' | tail -1)";;
    esac
    sleep 1
done
[ "$OK" = 1 ] || { echo "DFU FALHOU depois de 40 tentativas"; exit 1; }
python3 - "$VER" <<'PY'
import glob,time,serial,sys,re
esp=sys.argv[1]
# ESPERA A PORTA VOLTAR em vez de supor um sleep fixo. A placa
# re-enumera depois do DFU e o tempo varia; um `sleep 9` que quase
# sempre basta falha silenciosamente na vez em que nao basta.
# ⚠️ TAMBEM AQUI, e nao so no `dfu go` la em cima.
#
# Em 02/09 consertei o bloco do `dfu go` para usar o `porta_daisy` e
# DEIXEI ESTE no `p[-1]`. E a 15bd a repetir-se dentro do mesmo ficheiro,
# no mesmo dia: um conserto nao adoptado por TODOS os chamadores nao esta
# feito.
#
# E o grep com que declarei "nenhum caminho velho sobrou" exigia
# `glob(...)[-1]` numa linha so — aqui estao em linhas separadas. Verifiquei
# com um padrao incapaz de casar a forma real do codigo, que e a familia da
# 15av no suporte da busca.
sys.path.insert(0, '.')
import porta_daisy
p = None
for _ in range(40):
    try:
        p, _v = porta_daisy.achar()
        break
    except Exception:
        # ⚠️ `Exception` e nao `SystemExit`: o `achar()` levanta
        #    RuntimeError. Errei o contrato desta funcao DUAS vezes hoje
        #    ao adopta-la — primeiro tratei o retorno (par) como string,
        #    agora o tipo da excepcao. Adoptar funcao partilhada exige LER
        #    a assinatura e o que ela levanta, e eu deduzi as duas.
        time.sleep(1.0)
if not p:
    print("  nenhuma placa respondeu `board` — nao da pra conferir a versao")
    sys.exit(1)
s=serial.Serial(p,115200,timeout=1); time.sleep(1.5); s.reset_input_buffer()
s.write(b"log on\n"); time.sleep(1.2); s.read(3000)
s.write(b"board\n"); time.sleep(2.0)
t=s.read(4000).decode('utf-8','replace'); s.close()
m=re.search(r'Cotucatronics OS v([0-9.]+)', t)
viu = m.group(1) if m else "?"
print(f"  placa diz v{viu}")
print("  CONFERE" if viu==esp else f"  *** NAO CONFERE: fonte v{esp}, placa v{viu} ***")
sys.exit(0 if viu==esp else 1)
PY

# ⚠️ Gravar NAO termina no "File downloaded successfully" nem na
# versao conferida. Termina em repor o estado de operacao e DIZER
# qual foi. O `set -e` no topo faz uma reposicao falhada derrubar o
# script — reposicao que falha em silencio seria pior que nenhuma.
python3 estado.py repor "$ESTADO"
