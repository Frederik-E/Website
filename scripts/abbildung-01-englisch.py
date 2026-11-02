"""Erzeugt die englische Fassung von Abb. 01 im Projekt cone-corridor:

    python3 scripts/abbildung-01-englisch.py

Liest abb/01_aufgabe.svg und schreibt abb/01_aufgabe.en.svg. Die Beschriftung
steckt dort als Glyphen (Matplotlib, Liberation Sans 8,5 pt). Das Skript setzt
die englischen Texte mit Matplotlib in derselben Schrift und tauscht sie ein.
Position, Farbe und Ausrichtung bleiben, die Maßangabe bleibt zentriert.
Braucht Python mit matplotlib und die Schrift Liberation Sans.

Nach einer Änderung an 01_aufgabe.svg erneut ausführen. Ändert sich dort eine
Beschriftung, hier in TEXTE den deutschen und englischen Text anpassen.
"""
import io, re, sys
import matplotlib
matplotlib.use('svg')
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties
from matplotlib.textpath import TextToPath

ORDNER = 'inhalt/projekte/cone-corridor/abb'
quelle = sys.argv[1] if len(sys.argv) > 1 else f'{ORDNER}/01_aufgabe.svg'
ziel = sys.argv[2] if len(sys.argv) > 2 else f'{ORDNER}/01_aufgabe.en.svg'
TEXTE = {  # Gruppe: (deutsch, englisch, zentriert)
    'text_2': ('Reihe B', 'Row B', False),
    'text_3': ('Reihe A', 'Row A', False),
    'text_4': ('Trigger-Pylone: steht näher als 0,2 m an der Spur',
               'Trigger cone: less than 0.2 m from the lane', False),
    'text_5': ('0,40 × 0,30 m', '0.40 × 0.30 m', True),
}
fp = FontProperties(family='Liberation Sans', size=8.5)
t2p = TextToPath()
breite = lambda s: t2p.get_text_width_height_descent(s, fp, ismath=False)[0]

# Englische Texte mit Matplotlib als Glyphen setzen
plt.rcParams['svg.fonttype'] = 'path'
fig = plt.figure(figsize=(4, 2))
for gid, (_, en, _) in TEXTE.items():
    fig.text(0.1, 0.5, en, fontproperties=fp, gid='neu_' + gid)
puffer = io.StringIO()
fig.savefig(puffer, format='svg')
neu = puffer.getvalue()
glyphen = {gid: pfad for pfad, gid in re.findall(r'(<path id="(LiberationSans-[0-9a-f]+)"[^>]*/>)', neu)}
uses = {}
for gid in TEXTE:
    m = re.search(r'<g id="neu_%s">(.*?)</g>\s*</g>' % gid, neu, re.S)
    uses[gid] = [
        '<use xlink:href="#%s"%s/>' % (ref, ' transform="translate(%s 0)"' % x if x else '')
        for ref, x in re.findall(r'<use xlink:href="#([^"]+)"(?: x="([^"]*)")?/>', m.group(1))
    ]
    assert len(uses[gid]) == len(TEXTE[gid][1]), gid

s = open(quelle, encoding='utf-8').read()
# Alle alten Glyphen-Definitionen aus den Textgruppen entfernen
s, anzahl = re.subn(r'\s*<defs>\s*(?:<path id="LiberationSans-[^>]*/>\s*)+</defs>', '', s)

for gid, (de, en, zentriert) in TEXTE.items():
    muster = re.compile(r'(<g id="%s">\s*)<!-- %s -->(\s*<g style="[^"]*" transform="translate\()([-\d.]+)( [-\d.]+\) scale\(0\.085 -0\.085\)">)(.*?)(\s*</g>\s*</g>)' % (gid, re.escape(de)), re.S)
    m = muster.search(s)
    assert m, gid
    x = float(m.group(3))
    if zentriert:
        x += (breite(de) - breite(en)) / 2
    einzug = '\n     '
    s = s[:m.start()] + m.group(1) + '<!-- %s -->' % en + m.group(2) + ('%g' % x) + m.group(4) + einzug + einzug.join(uses[gid]) + m.group(6) + s[m.end():]

# Alle gebrauchten Glyphen in Gruppe text_1 sammeln
gebraucht = sorted({h for u in uses.values() for h in re.findall(r'#(LiberationSans-[0-9a-f]+)', ' '.join(u))})
defs = '\n     <defs>\n      ' + '\n      '.join(glyphen[g] for g in gebraucht) + '\n     </defs>'
s = s.replace('<!-- Beschriftung entfernt; die Glyphen hier nutzen andere Texte -->',
              '<!-- Glyphs for the labels below -->', 1)
m = re.search(r'(<g id="text_1">\s*<!--[^>]*-->\s*<g [^>]*>)', s)
s = s[:m.end()] + defs + s[m.end():]

# Übrige Kommentare
s = s.replace('<!-- Fahrzeug als Auto in Draufsicht, Außenmaß 0,40 × 0,30 m -->', '<!-- Vehicle as a car seen from above, outer dimensions 0.40 × 0.30 m -->')
s = s.replace('<!-- Trigger-Pylone: steht näher als 0,2 m an der Spur -->', '<!-- Trigger cone: stands closer than 0.2 m to the lane -->')
open(ziel, 'w', encoding='utf-8').write(s)
print(f'{ziel}: {len(TEXTE)} Beschriftungen auf Englisch')
