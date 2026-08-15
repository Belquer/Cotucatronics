#!/usr/bin/env python3
"""cotuca_measure v0.1.0 — mede a frequencia real da saida do firmware.

Grava um trecho curto de uma entrada de audio e reporta a frequencia
dominante por FFT, com interpolacao parabolica pra precisao sub-bin.
Serve pra tirar o teste de afinacao do "achismo do ouvido".

Uso:
    ./cotuca_measure.py --scan                  # varre todas as entradas
    ./cotuca_measure.py --device 3 --seconds 2
    ./cotuca_measure.py --device 3 --expect 440
"""

VERSION = "0.1.0"

import argparse
import sys

import numpy as np
import sounddevice as sd


def dominant_freq(x, sr):
    """Frequencia do pico espectral, refinada por parabola no log-magnitude."""
    x = np.asarray(x, dtype=np.float64)
    x = x - x.mean()
    if np.max(np.abs(x)) < 1e-9:
        return 0.0, -np.inf

    # Hann reduz vazamento, que e o que mais atrapalha a leitura do pico
    w = np.hanning(len(x))
    spec = np.abs(np.fft.rfft(x * w))
    freqs = np.fft.rfftfreq(len(x), 1.0 / sr)

    # ignora DC e o rumor de rede abaixo de 30 Hz
    lo = np.searchsorted(freqs, 30.0)
    k = lo + int(np.argmax(spec[lo:]))

    # interpolacao parabolica nos 3 bins ao redor do pico
    if 0 < k < len(spec) - 1:
        a, b, c = (np.log(spec[k - 1] + 1e-20),
                   np.log(spec[k] + 1e-20),
                   np.log(spec[k + 1] + 1e-20))
        delta = 0.5 * (a - c) / (a - 2 * b + c)
    else:
        delta = 0.0

    freq = (k + delta) * sr / len(x)
    rms = float(np.sqrt(np.mean(x**2)))
    db = 20 * np.log10(rms + 1e-12)
    return float(freq), db


def measure(device, seconds, sr, expect=None):
    info = sd.query_devices(device)
    ch = min(int(info["max_input_channels"]), 2)
    if ch < 1:
        return
    rec = sd.rec(int(seconds * sr), samplerate=sr, channels=ch,
                 device=device, dtype="float64", blocking=True)

    print(f"\n[{device}] {info['name']}")
    for c in range(ch):
        f, db = dominant_freq(rec[:, c], sr)
        if db < -70:
            print(f"  ch{c+1}: silencio ({db:.0f} dBFS)")
            continue
        line = f"  ch{c+1}: {f:8.2f} Hz   {db:6.1f} dBFS"
        if expect:
            cents = 1200 * np.log2(f / expect) if f > 0 else float("nan")
            line += f"   erro {f - expect:+.2f} Hz ({cents:+.1f} cents)"
        print(line)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--device", type=int, default=None)
    ap.add_argument("--scan", action="store_true")
    ap.add_argument("--seconds", type=float, default=2.0)
    ap.add_argument("--rate", type=int, default=48000)
    ap.add_argument("--expect", type=float, default=None,
                    help="frequencia alvo em Hz, pra reportar erro em cents")
    args = ap.parse_args()

    print(f"cotuca_measure v{VERSION}")

    if args.scan:
        for i, d in enumerate(sd.query_devices()):
            if d["max_input_channels"] > 0:
                try:
                    measure(i, args.seconds, args.rate, args.expect)
                except Exception as e:
                    print(f"\n[{i}] {d['name']}\n  falhou: {e}")
        return

    if args.device is None:
        sys.exit("informe --device N ou use --scan")
    measure(args.device, args.seconds, args.rate, args.expect)


if __name__ == "__main__":
    main()
