#!/usr/bin/env python3
"""cotuca_noise v0.1.0 — caça-ruído: espectro do piso de saída.

Diferente do cotuca_measure.py, que acha UMA frequência dominante:
este faz média de Welch sobre muitos quadros pra estabilizar o piso e
lista os PICOS ESTREITOS que se destacam dele. Ruído tonal (apito) tem
pico; ruído largo (crackle, hiss) não tem — a diferença entre os dois
é o que o relatório mostra.

Uso:
    ./cotuca_noise.py --device 3 --seconds 8 --label "entrada em curto"
    ./cotuca_noise.py --device 3 --compare a.npy b.npy
"""

VERSION = "0.1.0"

import argparse
import sys

import numpy as np
import sounddevice as sd

# Suspeitos conhecidos: se um pico cair perto, o nome sai no relatorio.
SUSPEITOS = [
    (1000.0, "frame USB (1 kHz) ou harmonico"),
    (2000.0, "2o harmonico do frame USB"),
    (3000.0, "3o harmonico do frame USB"),
    (60.0,   "rede eletrica 60 Hz"),
    (120.0,  "2o harmonico da rede"),
    (180.0,  "3o harmonico da rede"),
    (50.0,   "rede eletrica 50 Hz"),
    (1.0,    "heartbeat do LED (1 Hz)"),
    (8000.0, "clock de regulador chaveado?"),
]


def nomeia(f):
    for alvo, nome in SUSPEITOS:
        # 3% de tolerancia, ou 2 Hz no grave
        tol = max(alvo * 0.03, 2.0)
        if abs(f - alvo) <= tol:
            return nome
    return ""


def analisa(x, sr, label, topn=8):
    x = x - x.mean()
    n = len(x)
    rms = float(np.sqrt(np.mean(x**2)))
    peak = float(np.max(np.abs(x)))

    def db(v):
        return 20 * np.log10(max(v, 1e-12))

    print(f"\n=== {label} ===")
    print(f"  RMS {db(rms):7.1f} dBFS   pico {db(peak):7.1f} dBFS"
          + ("   <<< CLIPANDO" if peak > 0.99 else ""))
    if peak > 0.99:
        print("  (baixe o ganho da interface — o espectro esta contaminado)")

    # Welch: quadros de 8192 com 50% de sobreposicao, media de potencia.
    # Media estabiliza o piso largo e mantem os picos estreitos.
    N = 8192
    w = np.hanning(N)
    step = N // 2
    frames = [x[i:i+N] * w for i in range(0, n - N, step)]
    if not frames:
        print("  captura curta demais")
        return None
    P = np.mean([np.abs(np.fft.rfft(f))**2 for f in frames], axis=0)
    freqs = np.fft.rfftfreq(N, 1 / sr)
    amp = np.sqrt(P) * 2 / np.sum(w)

    # piso local = mediana numa janela larga; pico = quanto se destaca dele
    k = 101
    pad = np.pad(amp, k // 2, mode="edge")
    floor = np.array([np.median(pad[i:i+k]) for i in range(len(amp))])
    excess_db = 20 * np.log10(np.maximum(amp, 1e-14) / np.maximum(floor, 1e-14))

    print(f"\n  {'freq':>9}  {'nivel':>9}  {'acima do piso':>13}   suspeito")
    lo = np.searchsorted(freqs, 15.0)
    cand = []
    i = lo
    while i < len(amp) - 1:
        if excess_db[i] > 6 and amp[i] >= amp[i-1] and amp[i] >= amp[i+1]:
            cand.append((excess_db[i], freqs[i], amp[i]))
            i += 3
        else:
            i += 1
    cand.sort(reverse=True)
    if not cand:
        print("       (nenhum pico tonal — ruido e LARGO, nao tonal)")
    for ex, f, a in cand[:topn]:
        print(f"  {f:8.1f} Hz  {db(a):8.1f}  {ex:12.1f} dB   {nomeia(f)}")

    print(f"\n  {'banda':<18} {'energia':>8}")
    tot = np.sum(P)
    for a_, b_ in [(15,60),(60,200),(200,800),(800,2500),(2500,8000),(8000,20000)]:
        m = (freqs >= a_) & (freqs < b_)
        print(f"  {a_:5d}-{b_:<6d} Hz  {np.sum(P[m])/tot*100:6.1f}%")
    return {"rms": rms, "peak": peak, "freqs": freqs, "amp": amp}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--device", type=int, default=3)
    ap.add_argument("--seconds", type=float, default=8.0)
    ap.add_argument("--rate", type=int, default=48000)
    ap.add_argument("--channel", type=int, default=1, help="0=ch1, 1=ch2")
    ap.add_argument("--label", default="captura")
    ap.add_argument("--save", default=None)
    args = ap.parse_args()

    print(f"cotuca_noise v{VERSION}")
    info = sd.query_devices(args.device)
    ch = min(int(info["max_input_channels"]), 2)
    if ch < 1:
        sys.exit("device sem entrada")
    rec = sd.rec(int(args.seconds * args.rate), samplerate=args.rate,
                 channels=ch, device=args.device, dtype="float64", blocking=True)
    r = analisa(rec[:, min(args.channel, ch - 1)], args.rate, args.label)
    if args.save and r:
        np.save(args.save, np.vstack([r["freqs"], r["amp"]]))
        print(f"\n  salvo em {args.save}")


if __name__ == "__main__":
    main()
