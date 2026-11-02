#!/usr/bin/env python3
"""svg_colisoes v0.9.0 — verificador de sobreposicao para SVGs entregues.

⚠️ ESTE E O PASSO 1 DA SKILL `design-review`, e vem ANTES do olho.
   Zero colisoes e o portao, e ele sabe dizer NAO — e sabe dizer
   "nao consegui olhar", que e outra coisa. Ver
   .claude/skills/design-review/SKILL.md.


Calcula a caixa (bbox) de cada texto e de cada forma e reporta interseccoes:
  texto x texto   (sempre defeito)
  texto x forma   (defeito, salvo se o texto esta DENTRO de um rect grande
                   que e claramente um contentor — ver --contentor)
CODIGOS DE SAIDA, e sao TRES perguntas diferentes:

    0   verificado, ZERO colisoes    -> passa
    1   verificado, HA colisoes      -> defeito no DESENHO
    2   erro de uso
    3   NAO VERIFICAVEL              -> defeito na FERRAMENTA ou no ficheiro.
                                       Nem aprovacao nem reprovacao.

⚠️ O 3 existe porque, ate 2026-09-05, um ficheiro que REBENTAVA saia com 1 — o
mesmo codigo de quem tinha colisoes de verdade. Uma falha da ferramenta
vestia-se de achado, e quem lia a lista via "defeito" onde estava "nao
consegui olhar".

⚠️ E ZERO TEXTOS NAO E ZERO COLISOES. Um SVG onde nao se acha `<text>` nenhum
nao foi aprovado: nao foi olhado. Acontece com os renders do `kicad-cli`, que
desenham as letras como PATHS — a ferramenta ve 12 860 segmentos e nenhuma
palavra, e diria "0 colisoes" com toda a confianca. Zero so vale como
resultado quando havia o que contar.

Uso: python3 svg_colisoes.py ficheiro.svg [--contentor 0.6] [--largura 0.55]
  --largura   largura media de caractere em em (Helvetica ~0.55)
  --contentor rects com area > esta fraccao do canvas sao contentores e
              nao colidem com texto por dentro deles
"""
import re, sys, math, xml.etree.ElementTree as ET

VERSAO = "0.9.0"

W_CHAR = 0.55

def bbox_text(el, fs, anc=None):
    """A caixa de um <text>. Usa a largura MEDIDA quando o ficheiro a traz.

    O KiCad escreve `textLength` em cada <text> — a largura exacta que ele
    proprio calculou com a fonte de tracos — e depois desenha os glifos como
    paths ao lado. Havendo esse numero, nao se estima: mede-se. A estimativa
    `n × corpo × 0,6` erra por 18 mm numa nota de 73 caracteres (medido a
    2026-09-05 na folha do painel), o suficiente para dizer "limpo" sobre um texto
    que atravessa um potenciometro."""
    x = float(el.get("x", 0)); y = float(el.get("y", 0))
    s = "".join(el.itertext())
    tl = el.get("textLength")
    w = float(tl) if tl else len(s) * fs * W_CHAR
    # ⚠️ O ANCHOR TAMBEM VEM DO CSS, e ate 2026-09-22 so se lia o atributo.
    # Um `<text class="rot">` com `text-anchor:middle` na folha de estilo era
    # modelado como se comecasse no ponto — a caixa saia deslocada de meia
    # largura, e a ferramenta reportava colisoes que nao existiam E calava
    # outras que existiam. A mesma familia do `font-size`, que ja se lia.
    if anc is None:
        anc = el.get("text-anchor") or "start"
    if anc == "middle": x0 = x - w / 2
    elif anc == "end":  x0 = x - w
    else:               x0 = x
    # baseline em y; ascendente ~0.75 em, descendente ~0.25 em
    return (x0, y - fs * 0.75, x0 + w, y + fs * 0.25, s)

def bbox_path(d):
    nums = [float(v) for v in re.findall(r"-?\d+\.?\d*(?:e-?\d+)?", d)]
    if len(nums) < 4: return None
    xs, ys = nums[0::2], nums[1::2]
    return (min(xs), min(ys), max(xs), max(ys))

def num(v, base=None, omissao=0.0):
    """Le um comprimento SVG. Entende `100%`, `12px`, `4mm`, `10pt` e cru.

    ⚠️ EXISTE PORQUE `float(el.get("width"))` REBENTAVA EM `100%`. Tres SVGs
    de docs/ morriam aqui — e saiam com o MESMO codigo 1 dos que tinham
    colisoes de verdade. Uma falha da ferramenta vestida de achado.

    `base` e a dimensao de referencia para percentagens (largura ou altura do
    canvas). Sem base, uma percentagem nao e resoluvel e devolve None — que o
    chamador trata, em vez de rebentar.
    """
    if v is None: return omissao
    v = str(v).strip()
    if not v: return omissao
    if v.endswith('%'):
        try: f = float(v[:-1]) / 100.0
        except ValueError: return None
        return None if base is None else f * base
    m = re.match(r'^(-?[\d.]+(?:e-?\d+)?)\s*(px|pt|mm|cm|in|em|ex)?$', v)
    if not m: return None
    x = float(m.group(1)); u = m.group(2)
    return x * {'mm': 3.7795, 'cm': 37.795, 'in': 96.0,
                'pt': 1.3333, 'em': 16.0, 'ex': 8.0}.get(u, 1.0)


def inter(a, b, pad=0.0):
    return not (a[2] + pad < b[0] or b[2] + pad < a[0] or a[3] + pad < b[1] or b[3] + pad < a[1])

def quanto(a, b):
    """QUAO fundo entram uma na outra, no eixo mais raso — em unidades da folha.

    ⚠️ NASCEU DE UM CASO REAL (2026-09-06). A ferramenta dizia "ha colisao" e nao
    dizia QUANTO, e eu reportei ao dB como defeito serio um par `V-`/`4` que
    se sobrepunha 42 MICROMETROS — dentro do erro do proprio modelo, que
    supoe ascendente de 0,75 do corpo e a fonte de tracos do KiCad nao tem
    exactamente essa metrica.

    Um veredito binario obriga quem le a ir calcular a mao para saber se
    mexe no desenho. 42 um le-se como ruido do modelo; 2 mm salta a vista.
    Medir nao baixa a regua do portao — ele continua a acusar. Dá contexto
    a quem decide."""
    ox = min(a[2], b[2]) - max(a[0], b[0])
    oy = min(a[3], b[3]) - max(a[1], b[1])
    d = min(ox, oy)                       # o eixo mais raso e o que decide
    return f"{d*1000:.0f} um" if abs(d) < 0.2 else f"{d:.2f}"

def main():
    if len(sys.argv) < 2: print(__doc__); sys.exit(2)
    fn = sys.argv[1]
    contentor = 0.6; global W_CHAR
    # --esquematico: NUM ESQUEMATICO, TEXTO A TOCAR LINHA E A CONVENCAO.
    # Um rotulo de rede PRENDE-SE ao fio — e o que um rotulo e; um numero de
    # pino ASSENTA na perna. Medido no preamp: dos 74 restantes, 67 eram
    # `AGND` (13x), `VBIAS`, `V+PRE`, `PWR_FLAG` e os pinos 1..8. Acusa-los
    # ensina a ignorar o portao, que e o anti-padrao §6 da skill.
    # Com o modo: 1 texto×texto + 6 texto×borda — numero que alguem olha.
    esquematico = False
    for i, a in enumerate(sys.argv):
        if a == "--contentor": contentor = float(sys.argv[i+1])
        if a == "--largura": W_CHAR = float(sys.argv[i+1])
        if a == "--esquematico": esquematico = True
    root = ET.parse(fn).getroot()
    ns = {"s": "http://www.w3.org/2000/svg"}
    # canvas: o viewBox manda quando width/height vem em percentagem
    vb = (root.get("viewBox") or "").replace(",", " ").split()
    cw = num(root.get("width"), None, None)
    ch = num(root.get("height"), None, None)
    if (cw is None or ch is None) and len(vb) == 4:
        cw, ch = float(vb[2]), float(vb[3])
    if cw is None or ch is None:
        cw, ch = 1000.0, 1000.0

    # font-size por classe (do <style>) + inline
    ambiguas = set()   # classes com regras em desacordo: nao se usam
    css = {}      # classe -> tamanho
    css_anc = {}  # classe -> text-anchor
    # ⚠️ SEM DEPENDER DO ESPACO DE NOMES, e foi isso que falhou a 2026-09-22.
    # Os SVGs do manual nao declaravam `xmlns`; o ElementTree parseia-os sem
    # espaco de nomes, esta procura com `{...svg}style` nao achava nada, e a
    # ferramenta caia no omisso de 12 px e `text-anchor: start` — modelando
    # cada rotulo CENTRADO como se comecasse no ponto. Meia largura de desvio
    # em todos os textos, em silencio. A busca por nome local sobrevive as
    # duas formas, e o gerador tambem foi consertado: os dois lados.
    for st in [e for e in root.iter()
               if (e.tag.split("}")[-1] if isinstance(e.tag, str) else "") == "style"]:
        for m in re.finditer(r"([\w.#\s,>-]+)\{([^}]*)\}", st.text or ""):
            corpo = m.group(2)
            fm = re.search(r"font-size\s*:\s*([\d.]+)", corpo)
            am = re.search(r"text-anchor\s*:\s*([\w-]+)", corpo)
            # um seletor pode nomear varias classes ("svg .rot.peq"): guarda
            # em TODAS, porque nao se resolve especificidade aqui — e o que
            # interessa e nao ficar com o omisso "start" quando ha regra.
            # (!) SO SELETORES DE UMA CLASSE SO. Esta ferramenta NAO resolve
            # CSS: nao ha contexto nem especificidade. A 2026-09-22 ela aceitou
            # `svg.dpdt .rot.peq{font-size:12px}` como se falasse de `.rot`,
            # e mediu o manual inteiro com 12 px onde eram 15. Regra composta
            # nao se le — e se duas regras discordarem sobre a MESMA classe,
            # a classe e marcada AMBIGUA e nao se usa. Adivinhar em silencio
            # e o que se esta a consertar; adivinhar melhor nao serve.
            sel = m.group(1).strip()
            cs = re.findall(r"\.([\w-]+)", sel)
            if len(cs) != 1 or not re.fullmatch(r"\.[\w-]+", sel):
                continue
            c = cs[0]
            if fm:
                v = float(fm.group(1))
                if c in css and css[c] != v: ambiguas.add(c)
                css[c] = v
            if am:
                if c in css_anc and css_anc[c] != am.group(1): ambiguas.add(c)
                css_anc[c] = am.group(1)
    assumidos = []   # textos cujo tamanho nao estava declarado em lado nenhum

    def anchor(el):
        m = re.search(r"text-anchor\s*:\s*([\w-]+)", el.get("style", "") or "")
        if m: return m.group(1)
        if el.get("text-anchor"): return el.get("text-anchor")
        # a ULTIMA classe que tem regra vence, que e a aproximacao honesta
        # a cascata sem implementar especificidade
        achado = None
        for c in (el.get("class") or "").split():
            if c in css_anc and c not in ambiguas: achado = css_anc[c]
        return achado or "start"

    def fsize(el):
        m = re.search(r"font-size\s*:\s*([\d.]+)", el.get("style", "") or "")
        if m: return float(m.group(1))
        if el.get("font-size"): return float(el.get("font-size"))
        for c in (el.get("class") or "").split():
            if c in css and c not in ambiguas: return css[c]
        assumidos.append("".join(el.itertext())[:20])
        return 12.0

    # ⚠️ OS GLIFOS NAO SAO DESENHO. O kicad-cli escreve cada texto DUAS vezes:
    # um <text opacity=0> (que diz o que la esta e que largura tem) e um
    # <g class="stroked-text"> com as letras desenhadas como paths. No preamp
    # sao 18 483 paths de letra para 333 de desenho a serio — 98%.
    #
    # Sem esta excepcao cada rotulo colidia com o seu PROPRIO desenho: 256
    # acusacoes num esquema limpo. Portao que acusa 256 vezes e portao que se
    # desliga na primeira semana, e ai deixa de haver portao.
    glifos = set()
    for g in root.iter():
        if g.tag.split("}")[-1] == "g" and "stroked-text" in (g.get("class") or ""):
            for e2 in g.iter():
                if e2 is not g: glifos.add(id(e2))

    texts, shapes, segs, vistos = [], [], [], set()
    for el in root.iter():
        if id(el) in glifos: continue          # e a letra, nao o desenho
        tag = el.tag.split("}")[-1]
        # ⚠️ O <text> do KiCad e INVISIVEL de proposito: `opacity="0"`, com os
        # glifos desenhados como paths num <g class="stroked-text"> ao lado. Ele
        # esta la para dizer o texto e a largura. Descartar por opacidade — que
        # era a regra para sombreado decorativo — apagava TODOS os textos de
        # todo esquematico exportado, e a ferramenta respondia "NAO VERIFICAVEL"
        # a ficheiros com 336 textos medidos la dentro. O portao existia e
        # estava cego.
        if tag != "text":
            try:
                if float(el.get("opacity", 1)) < 0.5: continue   # decorativo
            except ValueError: pass
        if tag == "text":
            # ⚠️ O MESMO TEXTO, DUAS VEZES, NA MESMA COORDENADA nao sao dois
            # textos sobrepostos: e um texto desenhado duas vezes. O kicad-cli
            # emite cada um em duplicado (`C21` em 202.9557,138.6350 aparecia
            # 2x), e cada rotulo acusava colisao com o seu proprio gemeo.
            # Conteudo + coordenada + corpo iguais e o mesmo texto; textos
            # diferentes nao partilham os tres.
            chave = ((el.text or "").strip(), el.get("x"), el.get("y"), el.get("font-size"))
            if chave in vistos: continue
            vistos.add(chave)
            texts.append(bbox_text(el, fsize(el), anchor(el)))
        elif tag == "rect":
            x, y = num(el.get("x"), cw, 0.0), num(el.get("y"), ch, 0.0)
            w, h = num(el.get("width"), cw, 0.0), num(el.get("height"), ch, 0.0)
            if None in (x, y, w, h): continue                  # nao resolvivel
            if w * h > contentor * cw * ch: continue           # fundo da pagina
            kind = "contentor" if w * h > 0.02 * cw * ch else "rect"
            shapes.append((x, y, x + w, y + h, kind))
        elif tag == "circle":
            cx, cy, r = float(el.get("cx")), float(el.get("cy")), float(el.get("r"))
            shapes.append((cx - r, cy - r, cx + r, cy + r, "circle"))
        elif tag == "line":
            x1, y1, x2, y2 = (float(el.get(k)) for k in ("x1", "y1", "x2", "y2"))
            segs.append(((x1, y1), (x2, y2)))
        elif tag in ("path", "polyline", "polygon"):
            d = el.get("d") or el.get("points") or ""
            b = bbox_path(d)
            if not b: continue
            area = (b[2]-b[0]) * (b[3]-b[1])
            fechado = tag == "polygon" or d.strip().lower().endswith("z")
            if fechado and area > 0.02 * cw * ch: shapes.append((*b, "contentor"))
            else: segs.extend(path_segs(d, fechado))
        elif tag == "line":
            pass

    probs = []
    for i, a in enumerate(texts):
        for b in texts[i+1:]:
            if inter(a[:4], b[:4]):
                probs.append(("texto×texto", a[4][:40], b[4][:40], quanto(a[:4], b[:4])))
    for a in texts:
        for s in shapes:
            if s[4] == "contentor":
                # texto totalmente dentro de um contentor e legitimo; so colide se cruzar a borda
                inside = a[0] >= s[0] and a[2] <= s[2] and a[1] >= s[1] and a[3] <= s[3]
                outside = not inter(a[:4], s[:4])
                if inside or outside: continue
                probs.append(("texto×borda", a[4][:40], f"{s[4]} {int(s[0])},{int(s[1])}", quanto(a[:4], s[:4])))
            else:
                # rect/circle: rotulo DENTRO da forma e legitimo (etiqueta); so acusa se cruzar a borda
                inside = a[0] >= s[0] and a[2] <= s[2] and a[1] >= s[1] and a[3] <= s[3]
                if inter(a[:4], s[:4]) and not inside:
                    probs.append(("texto×borda", a[4][:40], f"{s[4]} {int(s[0])},{int(s[1])}", quanto(a[:4], s[:4])))

    for a in texts:
        for (p1, p2) in segs:
            if seg_rect(p1, p2, a[:4]):
                if esquematico: continue          # rotulo no fio e convencao
                probs.append(("texto×linha", a[4][:40], f"seg {int(p1[0])},{int(p1[1])}-{int(p2[0])},{int(p2[1])}"))
                break
    # ⚠️ ZERO TEXTOS E AUSENCIA DE DADO, NAO APROVACAO. Sem `<text>` nao ha
    # o que colidir, e "0 colisoes" seria uma frase verdadeira sobre nada.
    if not texts:
        print(f"{fn}: NAO VERIFICAVEL — nenhum <text> encontrado "
              f"({len(shapes)} formas, {len(segs)} segmentos).")
        print("   As letras podem estar como PATHS (renders do kicad-cli fazem isso).")
        print("   Zero colisoes aqui nao significa nada. Ver a skill design-review §1.")
        sys.exit(3)
    print(f"{fn}: {len(texts)} textos, {len(shapes)} formas, {len(segs)} segmentos, {len(probs)} colisoes")
    # ⚠️ ASSUMIR EM SILENCIO E O MODO DE FALHA CARO. A 2026-09-22 esta ferramenta
    # mediu 78 figuras com 12 px e `start` porque nao achou o `<style>`, e
    # ninguem soube. Agora ela diz de quantos textos teve de adivinhar.
    if assumidos:
        print("   (!) %d texto(s) SEM tamanho declarado — assumi 12 px: %s"
              % (len(assumidos), ", ".join(sorted(set(assumidos))[:6])))
    for p in probs: print("  !!", *p, flush=True)
    sys.exit(1 if probs else 0)

def path_segs(d, fechado):
    """converte um `d` de path (ou `points` de polyline) em segmentos.

    ⚠️ 2026-09-20: a versao anterior so entendia M/L/Z. Tudo o resto — e sobretudo
    o **H** e o **V**, que levam UM numero — era lido como par de coordenadas,
    e um caminho ortogonal saia como diagonais que nao existem. Nao e so falso
    positivo: e o verificador a medir um desenho que nao e o que esta la.
    Apanhado no desenho da chave DPDT do manual, cujos fios sao todos H/V.

    Curvas (C/S/Q/T/A) aproximam-se pelo ponto final — melhor do que tratar os
    pontos de controlo como vertices, que era o que acontecia.
    """
    ARGS = {"M": 2, "L": 2, "H": 1, "V": 1, "C": 6, "S": 4, "Q": 4, "T": 2, "A": 7, "Z": 0}
    toks = re.findall(r"[MmLlHhVvCcSsQqTtAaZz]|-?\d*\.?\d+(?:[eE]-?\d+)?", d)
    # `points` de polyline/polygon nao tem comandos: sao pares soltos
    if not any(tk[0].isalpha() for tk in toks):
        nums = [float(v) for v in toks]
        pts = list(zip(nums[0::2], nums[1::2]))
        out = list(zip(pts, pts[1:]))
        if fechado and len(pts) > 2:
            out.append((pts[-1], pts[0]))
        return out
    out, pts = [], []
    cur = (0.0, 0.0)
    ini = None
    i, cmd = 0, None
    while i < len(toks):
        tk = toks[i]
        if tk[0].isalpha():
            cmd = tk
            i += 1
            if cmd in "Zz":
                if ini and cur != ini:
                    out.append((cur, ini))
                cur = ini or cur
                continue
        if cmd is None:
            i += 1
            continue
        n = ARGS[cmd.upper()]
        vals = [float(v) for v in toks[i:i + n]]
        if len(vals) < n:
            break
        i += n
        rel = cmd.islower()
        c = cmd.upper()
        if c == "H":
            novo_pt = (cur[0] + vals[0] if rel else vals[0], cur[1])
        elif c == "V":
            novo_pt = (cur[0], cur[1] + vals[0] if rel else vals[0])
        else:
            dx, dy = vals[-2], vals[-1]
            novo_pt = (cur[0] + dx, cur[1] + dy) if rel else (dx, dy)
        if c == "M":
            ini = novo_pt
            if len(pts) > 1:
                out.extend(zip(pts, pts[1:]))
            pts = [novo_pt]
        else:
            out.append((cur, novo_pt))
            pts.append(novo_pt)
        cur = novo_pt
        # um comando repetido sem letra continua o mesmo comando (M vira L)
        if c == "M":
            cmd = "l" if rel else "L"
    if fechado and ini and cur != ini:
        out.append((cur, ini))
    return out


def seg_rect(p1, p2, r):
    """segmento p1-p2 toca o retangulo r=(x0,y0,x1,y1)?"""
    (x1, y1), (x2, y2) = p1, p2
    x0, y0, X1, Y1 = r
    def inside(x, y): return x0 <= x <= X1 and y0 <= y <= Y1
    if inside(x1, y1) or inside(x2, y2): return True
    # Liang-Barsky
    dx, dy = x2 - x1, y2 - y1
    t0, t1 = 0.0, 1.0
    for p, q in ((-dx, x1 - x0), (dx, X1 - x1), (-dy, y1 - y0), (dy, Y1 - y1)):
        if p == 0:
            if q < 0: return False
        else:
            t = q / p
            if p < 0: t0 = max(t0, t)
            else: t1 = min(t1, t)
            if t0 > t1: return False
    return True

def thin_miss(t, s):
    """bbox de linha diagonal/eixo pode conter o texto sem a linha o tocar.
    Para linhas axiais (largura ou altura ~0) exige toque real; para as
    outras, assume colisao (conservador)."""
    w, h = s[2] - s[0], s[3] - s[1]
    if h < 1.5:   # horizontal
        return not (t[1] <= s[1] <= t[3]) or not (t[2] >= s[0] and t[0] <= s[2])
    if w < 1.5:   # vertical
        return not (t[0] <= s[0] <= t[2]) or not (t[3] >= s[1] and t[1] <= s[3])
    return False

if __name__ == "__main__":
    # ⚠️ Excepcao NAO e colisao. Antes de 2026-09-05 um crash subia sem tratamento
    # e o shell via rc=1, indistinguivel de "achei defeito".
    try:
        main()
    except SystemExit:
        raise
    except Exception as e:
        fn = sys.argv[1] if len(sys.argv) > 1 else "?"
        print(f"{fn}: NAO VERIFICAVEL — {type(e).__name__}: {e}")
        print("   Isto e defeito da FERRAMENTA, nao do desenho.")
        sys.exit(3)
