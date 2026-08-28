#!/usr/bin/env python3
"""pitch_spectrum v0.2.0 — os N maiores picos, nao so o dominante.

O medidor de pico dominante mente quando o sinal tem bandas laterais
fortes: um granular com ratio != 1 gera AM na taxa do grao, e a lateral
pode superar a portadora. Ver o pico dominante sozinho fez a quinta
parecer BAIXA quando ela esta no lugar.
"""
import sys, numpy as np, sounddevice as sd

dev  = int(sys.argv[1]) if len(sys.argv) > 1 else 2
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 4.0
sr   = 48000
x = sd.rec(int(secs*sr), samplerate=sr, channels=1, device=dev, dtype='float32')
sd.wait()
x = x[:,0] * np.hanning(len(x))
X = np.abs(np.fft.rfft(x))
f = np.fft.rfftfreq(len(x), 1/sr)
# picos locais acima de -50 dB do maior
mx = X.max()
idx = [i for i in range(2, len(X)-2)
       if X[i] > X[i-1] and X[i] > X[i+1] and X[i] > mx*10**(-50/20)]
idx.sort(key=lambda i: -X[i])
print(f"pitch_spectrum v0.2.0 — {secs:.0f} s, bin {sr/len(x):.2f} Hz")
for i in idx[:6]:
    print(f"  {f[i]:9.2f} Hz   {20*np.log10(X[i]/mx):6.1f} dB rel")
