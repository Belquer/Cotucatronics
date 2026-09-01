import importlib.util, sys
def check(mod):
    spec=importlib.util.spec_from_file_location("g",mod); g=importlib.util.module_from_spec(spec); spec.loader.exec_module(g)
    par={}
    def find(a):
        par.setdefault(a,a)
        while par[a]!=a: par[a]=par[par[a]]; a=par[a]
        return a
    def uni(a,b): par[find(a)]=find(b)
    segs=[]
    for pts in g.W:
        for a,b in zip(pts,pts[1:]):
            if a!=b: segs.append((a,b)); uni(a,b)
    pinpts={}
    for ref,typ,val,x,y in g.PLACE:
        for num in sorted(g.PINPOS[typ]): pinpts[(ref,num)]=g.P(ref,num)
    # ponto sobre segmento (so onde ha juncao declarada, como no KiCad)
    def on(p,a,b):
        (px,py),(ax,ay),(bx,by)=p,a,b
        if abs((bx-ax)*(py-ay)-(by-ay)*(px-ax))>1e-6: return False
        return min(ax,bx)-1e-6<=px<=max(ax,bx)+1e-6 and min(ay,by)-1e-6<=py<=max(ay,by)+1e-6
    cand=set(g.JUNC)|set(pinpts.values())|{p for s in segs for p in s}
    for p in cand:
        p=(round(p[0],3),round(p[1],3))
        for a,b in segs:
            if on(p,a,b): uni(p,a)
    name={}
    for t,x,y,a in g.GLBL: name.setdefault(find((round(x,3),round(y,3))),set()).add(t)
    nets={}; solto=[]
    for (ref,num),p in pinpts.items():
        if p not in par: solto.append(f"{ref}.{num}@{p}"); continue
        r=find(p); nm="/".join(sorted(name.get(r,set()))) or f"n{sorted(par).index(r)}"
        nets.setdefault(nm,[]).append(f"{ref}.{num}")
    print(f"\n=== {mod} ===")
    print(f"  pinos soltos: {len(solto)}" + (("  " + ", ".join(solto)) if solto else ""))
    orfaos=[t for t,x,y,a in g.GLBL if (round(x,3),round(y,3)) not in par]
    print(f"  rotulos sem fio: {len(orfaos)}" + (("  "+", ".join(orfaos)) if orfaos else ""))
    curtos=[s for r,s in name.items() if len(s)>1]
    print(f"  redes com dois nomes: {len(curtos)}" + (("  "+str(curtos)) if curtos else ""))
    print(f"  redes: {len(nets)}")
    for nm in sorted(nets, key=lambda k:(k.startswith('n'), k)):
        n=len(nets[nm]); mark="   <-- so um pino NESTA folha" if n<2 else ""
        print(f"    {nm:12s} ({n:2d}) {', '.join(nets[nm])}{mark}")
for m in sys.argv[1:]: check(m)
