#!/usr/bin/env python3
"""nivel_tempo v0.2.0 — RMS em janelas de 50 ms, pra ver o notch passar."""
import sys, numpy as np, sounddevice as sd
dev  = int(sys.argv[1]) if len(sys.argv) > 1 else 2
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0
sr = 48000
x = sd.rec(int(secs*sr), samplerate=sr, channels=1, device=dev, dtype='float32')
sd.wait(); x = x[:,0]
w = int(0.05*sr); n = len(x)//w
r = [20*np.log10(max(np.sqrt(np.mean(x[i*w:(i+1)*w]**2)),1e-9)) for i in range(n)]
r = np.array(r)
print(f"nivel_tempo v0.1.0 — {secs:.0f}s, janelas de 50 ms")
print(f"  max {r.max():6.1f} dBFS   min {r.min():6.1f} dBFS   EXCURSAO {r.max()-r.min():5.1f} dB")
esc = "".join("█▇▆▅▄▃▂▁"[min(7,int((r.max()-v)/max(r.max()-r.min(),1e-9)*7))] for v in r)
for i in range(0, len(esc), 80): print("  "+esc[i:i+80])
