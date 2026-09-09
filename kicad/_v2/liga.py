#!/usr/bin/env python3
"""liga — acha o KiCad certo e devolve a placa. (rev C, API IPC oficial)

⚠️⚠️ O SOCKET E POR PROCESSO, E O GENERICO E O ERRADO.

O KiCad 10 abre `/tmp/kicad/api.sock` para o **gestor de projecto** e um
`/tmp/kicad/api-<PID>.sock` para CADA editor. O `kipy` sem argumentos vai ao
generico — e o gestor responde ao `get_version()` alegremente e depois recusa
tudo o que interessa com:

    no handler available for request of type kiapi.common.commands.GetOpenDocuments

Isso NAO quer dizer "a API nao funciona" nem "activa a API nas preferencias".
Quer dizer "estas a falar com a janela errada". Custou cinco tentativas e uma
reinstalacao mental do problema antes de eu olhar para os nomes dos ficheiros.

⚠️ E ficam sockets ORFAOS de instancias mortas. Um `api-86732.sock` de um
KiCad que ja fechou continua no disco e recusa a ligacao. Portanto: tenta
todos, fica com o que responder COM PLACA, e diz quantos viu.

⚠️ E o `open -a KiCad ficheiro.kicad_pcb` NAO abre o editor de placa se o
KiCad ja estiver aberto — o macOS so traz a app para a frente. O que abre e
o bundle proprio:

    open -a "/Applications/KiCad/PCB Editor.app" --args <caminho absoluto>

(E e ele que deixa o `<ficheiro>.kicad_pcb.kicad_pro` ao lado, que esta no
`.gitignore` desde 08/09.)
"""
import glob
import sys

VERSAO = "0.1.0"


def placa(exigir=True):
    """Devolve (KiCad, Board) do editor de placa vivo. Levanta se nao houver."""
    from kipy import KiCad
    vistos, vivos = [], []
    for s in sorted(glob.glob("/tmp/kicad/api-*.sock")):
        vistos.append(s)
        try:
            k = KiCad(socket_path="ipc://" + s)
            b = k.get_board()
            vivos.append((s, k, b))
        except Exception:
            continue
    if not vivos:
        msg = ("RECUSO: nenhum editor de PLACA a responder.\n"
               "        sockets vistos: %s\n"
               "        Abre a placa com:\n"
               '          open -a "/Applications/KiCad/PCB Editor.app" --args <ficheiro.kicad_pcb>\n'
               "        (e confirma api.enable_server=true em kicad_common.json)"
               % (", ".join(v.split("/")[-1] for v in vistos) or "nenhum"))
        if exigir:
            sys.exit(msg)
        return None, None
    if len(vivos) > 1:
        print("  ⚠️ %d editores de placa abertos; uso o primeiro (%s). Fecha os "
              "outros se nao for o que queres."
              % (len(vivos), vivos[0][0].split("/")[-1]), file=sys.stderr)
    return vivos[0][1], vivos[0][2]


if __name__ == "__main__":
    k, b = placa()
    print("liga v%s — KiCad %s" % (VERSAO, k.get_version().full_version))
    print("  %d footprints · %d pistas · %d zonas"
          % (len(b.get_footprints()), len(b.get_tracks()), len(b.get_zones())))
