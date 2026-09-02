#!/bin/bash
# flash v0.2.0 — grava SO se o build passou, e confere a versao na placa.
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
echo "flash v0.1.4 — fonte diz v$VER"
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
# A placa pode JA estar em DFU (de uma tentativa anterior que morreu no
# meio). Perguntar antes de mandar `dfu go` numa serial que nao existe.
if dfu-util -l 2>/dev/null | grep -q "Found DFU"; then
    echo "  ja esta em DFU"
else
    python3 - <<'PY' 2>/dev/null || true
import glob,time,serial
# ⚠️ A PORTA CERTA E A QUE RESPONDE, nunca a que tem o nome certo.
#
# Isto era `p[-1]` — ordem alfabetica. Em 01/09 a ponte ESP32 enumerou
# como `usbmodem101` e comandos foram parar la enquanto o script dizia
# que tinham sido aplicados. Mandar `dfu go` a ponte nao a poe em DFU e
# nao poe a Daisy tambem: a gravacao falha por um motivo que nao aparece
# em lado nenhum. Ver battle_scars 15ax.
for cand in sorted(glob.glob('/dev/cu.usbmodem*')):
    try:
        s=serial.Serial(cand,115200,timeout=1); time.sleep(0.9)
        s.reset_input_buffer(); s.write(b'board\n'); time.sleep(1.2)
        if b'Cotucatronics OS' in s.read(400):
            print('  dfu go -> ' + cand)
            s.reset_input_buffer(); s.write(b'dfu go\n'); time.sleep(2.0)
            s.close(); break
        s.close()
    except Exception:
        pass
else:
    print('  nenhuma porta respondeu Cotucatronics OS — a placa pode ja'
          ' estar em DFU, ou presa')
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
p=[]
for _ in range(40):
    p=glob.glob('/dev/cu.usbmodem*')
    if p: break
    time.sleep(1.0)
if not p:
    print("  a porta serial nao voltou — nao da pra conferir a versao"); sys.exit(1)
time.sleep(2.0)
s=serial.Serial(p[-1],115200,timeout=1); time.sleep(2.0); s.reset_input_buffer()
s.write(b"log on\n"); time.sleep(1.2); s.read(3000)
s.write(b"board\n"); time.sleep(2.0)
t=s.read(4000).decode('utf-8','replace'); s.close()
m=re.search(r'Cotucatronics OS v([0-9.]+)', t)
viu = m.group(1) if m else "?"
print(f"  placa diz v{viu}")
print("  CONFERE" if viu==esp else f"  *** NAO CONFERE: fonte v{esp}, placa v{viu} ***")
sys.exit(0 if viu==esp else 1)
PY
