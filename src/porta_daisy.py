#!/usr/bin/env python3
"""porta_daisy v0.1.0 — acha a Daisy PERGUNTANDO, nunca por posicao.

    import porta_daisy; p = porta_daisy.achar()
    ./porta_daisy.py            # imprime a porta e a versao

POR QUE EXISTE (2026-08-25). Todo script desta bancada fazia
`glob.glob('/dev/cu.usbmodem*')[-1]`. Funcionou enquanto havia UMA placa.
No dia em que o ESP32 entrou na USB passaram a existir dois `usbmodem`, e
a ordem que o `glob` devolve e a do sistema de ficheiros — nao alfabetica,
nao estavel. A escolha certa virou SORTE, e o modo de falha seria mandar
comandos da Daisy para o ESP32: sem erro, sem resposta, e um diagnostico
que procuraria o defeito no lado errado.

O conserto nao e ordenar a lista nem fixar o nome: e parar de INFERIR
identidade por posicao. A placa sabe dizer quem e — `board` responde
"Cotucatronics OS vX.Y.Z" — entao pergunta-se.

E a mesma familia do endereco posicional dos bindings do Loopy, que num
mesmo dia desenderecou dois deles em silencio ao mover um widget.
"""
import glob, sys, time

VERSAO = "0.1.0"


def achar(timeout_por_porta=3.0, verboso=False):
    """Devolve (porta, versao) da Daisy, ou levanta RuntimeError."""
    import serial
    candidatas = sorted(glob.glob('/dev/cu.usbmodem*'))
    if not candidatas:
        raise RuntimeError("nenhuma /dev/cu.usbmodem* — a placa esta ligada?")
    tentadas = []
    for p in candidatas:
        try:
            s = serial.Serial(p, 115200, timeout=2)
            time.sleep(2.5)
            s.reset_input_buffer()
            s.write(b"board\n")
            time.sleep(timeout_por_porta)
            t = s.read(9000).decode('utf-8', 'replace')
            s.close()
            for l in t.splitlines():
                if 'Cotucatronics OS v' in l:
                    v = l.split('Cotucatronics OS v')[1].split()[0]
                    if verboso:
                        print(f"   {p}: Cotucatronics OS v{v}")
                    return p, v
            tentadas.append((p, "respondeu, mas nao e a Daisy"))
        except Exception as e:
            tentadas.append((p, f"{type(e).__name__}"))
    det = " · ".join(f"{p} ({m})" for p, m in tentadas)
    raise RuntimeError(f"Daisy nao encontrada. Tentei: {det}")


if __name__ == "__main__":
    print(f"porta_daisy v{VERSAO}")
    try:
        p, v = achar(verboso=True)
        print(f"   PORTA: {p}")
        print(f"   PLACA: v{v}")
    except RuntimeError as e:
        print(f"   {e}")
        sys.exit(1)
