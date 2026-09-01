#!/usr/bin/env python3
"""medir_pente v0.3.0 — mede o ESPACAMENTO de um pente no espectro.

Nasceu em 01/09 para um defeito concreto: o dB reportou um pente de
1000 Hz exactos, de 1 kHz a 23 kHz, +17 a +35 dB, IMUNE as tres
topologias de alimentacao. 48000/48 = 1000 — cheira a taxa de bloco.

O teste que decide e MANIPULACAO, nao correlacao: se o bloco passar de
48 para 96 e o espacamento cair para 500 Hz, esta provado. Este
programa mede o espacamento; quem muda o bloco e a gravacao.

Nao mede "o pico" — mede a DISTANCIA entre picos. Sao perguntas
diferentes, e o `cotuca_measure` responde a outra.

Uso:
    ./medir_pente.py --device 3 --seconds 4
    ./medir_pente.py --device 3 --esperado 1000
"""
import argparse, sys
import numpy as np
import sounddevice as sd

VERSAO = "0.3.0"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--device", type=int, required=True)
    ap.add_argument("--seconds", type=float, default=4.0)
    ap.add_argument("--sr", type=int, default=48000)
    ap.add_argument("--lo", type=float, default=1000.0)
    ap.add_argument("--hi", type=float, default=23000.0)
    ap.add_argument("--esperado", type=float, default=None)
    a = ap.parse_args()
    print("medir_pente v%s" % VERSAO)

    n = int(a.seconds * a.sr)
    x = sd.rec(n, samplerate=a.sr, channels=1, device=a.device, dtype="float64")
    sd.wait()
    x = x[:, 0]
    # PRE-VOO: prova que o canal fala, ANTES de dizer o que ele diz.
    # Sem isto, "sem pente claro" e indistinguivel de "cabo no chao" —
    # e as duas frases saem com a mesma cara.
    pico = float(np.max(np.abs(x)))
    rms  = float(np.sqrt(np.mean(x * x)))
    ddb  = lambda v: 20 * np.log10(v + 1e-20)
    print("  CANAL: pico %.1f dBFS · rms %.1f dBFS" % (ddb(pico), ddb(rms)))
    if pico < 1e-5:
        print("  *** ENTRADA MUDA — o cabo chega? o instrumento toca? ***")
        return 1
    if ddb(rms) < -80:
        print("  ⚠️  rms abaixo de -80 dBFS: isto e chao de conversor,"
              " nao sinal. Qualquer veredito daqui e sobre o SILENCIO.")

    # MEDIA DE WELCH, e nao uma FFT unica.
    #
    # A v0.1.0 fazia uma FFT de 4 s: resolucao de 0,25 Hz, e o RUIDO
    # tem um maximo local a cada poucos bins. Resultado medido: 6044
    # "dentes" com 2,8 Hz de espacamento — o instrumento contou o
    # proprio ruido e respondeu com cara de dado.
    #
    # Media de segmentos resolve isso pela raiz: um dente de pente esta
    # SEMPRE na mesma frequencia e sobrevive a media; um maximo de ruido
    # muda de sitio a cada segmento e achata-se. E 4096 pontos dao
    # 11,7 Hz de resolucao, que chega de sobra para 1000 Hz de
    # espacamento e nao convida o ruido a fazer-se de dente.
    seg, hop = 4096, 2048
    w = np.hanning(seg)
    acc, nseg = None, 0
    for i in range(0, len(x) - seg, hop):
        X = np.fft.rfft(x[i:i + seg] * w)
        p2 = np.abs(X) ** 2
        acc = p2 if acc is None else acc + p2
        nseg += 1
    if nseg < 4:
        print("captura curta demais para media — usa --seconds maior."); return 1
    mag = 10 * np.log10(acc / nseg + 1e-20)
    f = np.fft.rfftfreq(seg, 1.0 / a.sr)
    print("  media de %d segmentos de %d pontos (%.1f Hz por bin)"
          % (nseg, seg, a.sr / seg))

    m = (f >= a.lo) & (f <= a.hi)
    fb, mb = f[m], mag[m]

    # CHAO LOCAL por mediana movel: um pente sobe acima do que o
    # rodeia, e comparar com a media global daria o resultado errado
    # numa banda inclinada.
    # CHAO LOCAL numa janela de ~600 Hz: larga o suficiente para
    # passar por cima de um dente sem o seguir, estreita o suficiente
    # para acompanhar a inclinacao da banda.
    k = max(3, int(600.0 / (a.sr / seg)) | 1)
    chao = np.convolve(mb, np.ones(k) / k, mode="same")
    saliencia = mb - chao

    # Picos: acima de 6 dB do chao local E maiores que os vizinhos.
    cand = np.where((saliencia[1:-1] > 6.0)
                    & (mb[1:-1] > mb[:-2]) & (mb[1:-1] > mb[2:]))[0] + 1
    if len(cand) < 4:
        print("menos de 4 dentes acima de 6 dB do chao — sem pente claro.")
        print("  dentes achados: %d" % len(cand)); return 0

    picos = fb[cand]
    difs = np.diff(picos)
    # Mediana, e nao media: um dente em falta duplicaria uma distancia
    # e a media mentiria; a mediana aguenta.
    esp = float(np.median(difs))
    alt = float(np.median(saliencia[cand]))
    print("  dentes: %d entre %.0f e %.0f Hz" % (len(picos), picos[0], picos[-1]))
    print("  ESPACAMENTO (mediana): %.1f Hz" % esp)
    print("  altura sobre o chao (mediana): %+.1f dB" % alt)
    print("  primeiros: %s" % ", ".join("%.0f" % p for p in picos[:8]))
    for bs in (48, 96, 32, 64, 128):
        if abs(esp - a.sr / bs) < esp * 0.06:
            print("  >>> BATE COM BLOCO DE %d (%.1f Hz = %d/%d)"
                  % (bs, a.sr / bs, a.sr, bs))
    if a.esperado:
        d = abs(esp - a.esperado) / a.esperado * 100
        print("  vs esperado %.0f Hz: erro %.1f%%  %s"
              % (a.esperado, d, "BATE" if d < 6 else "NAO BATE"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
