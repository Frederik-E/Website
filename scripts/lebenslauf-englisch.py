#!/usr/bin/env python3
"""
Übersetzt den eigenen Lebenslauf (aus Canva) ins Englische:
inhalt/lebenslauf-pdf/<name>.pdf  ->  inhalt/lebenslauf-pdf/<name>.en.pdf

Nur der Text wird ausgetauscht. Hintergrund, Linien, Aufzählungspunkte und
Foto bleiben unverändert. Jede ersetzte Zeile steht mit Schrift, Größe, Farbe,
Sperrung und Grundlinie der deutschen Zeile an derselben Stelle.

Die eingebetteten Schriften enthalten nur die Zeichen des deutschen Texts.
Für den englischen Text werden deshalb die vollständigen Schriften genommen:
Now Bold und Aileron Regular aus scripts/schriften/, Lato aus dem System.
Ihre Zeichenbreiten stimmen mit den eingebetteten genau überein.

Ändert sich der deutsche Lebenslauf, hier ZEILEN, PROFIL und UNVERAENDERT
anpassen. Das Skript bricht ab, wenn eine Zeile im PDF in keiner der Listen
steht oder eine Zeile der Listen im PDF fehlt.

Braucht PyMuPDF und fontTools:  pip install pymupdf fonttools
  python3 scripts/lebenslauf-englisch.py
"""
import re
import sys
from pathlib import Path

try:
    import pymupdf
    from fontTools.ttLib import TTFont
except ImportError:
    sys.exit('PyMuPDF oder fontTools fehlt. Installieren mit: pip install pymupdf fonttools')
import io

WURZEL = Path(__file__).resolve().parent.parent
ORDNER = WURZEL / 'inhalt' / 'lebenslauf-pdf'
SCHRIFTEN = WURZEL / 'scripts' / 'schriften'
LATO = Path('/usr/share/fonts/truetype/lato')

SCHRIFTDATEIEN = {
    'Now-Bold': SCHRIFTEN / 'Now-Bold.otf',
    'Aileron-Regular': SCHRIFTEN / 'Aileron-Regular.otf',
    'Lato-Regular': LATO / 'Lato-Regular.ttf',
    'Lato-Bold': LATO / 'Lato-Bold.ttf',
}

# Deutsche Zeile -> englische Zeile. Eng am Original, britisches Englisch.
ZEILEN = {
    # Linke Spalte
    'KONTAKT': 'CONTACT',
    'BILDUNG': 'EDUCATION',
    'Seit 2025': 'Since 2025',
    'KARLSRUHE INSTITUT FÜR TECHNOLOGY': 'KARLSRUHE INSTITUTE OF TECHNOLOGY',
    'TECHNISCHE UNIVERSITÄT HAMBURG': 'HAMBURG UNIVERSITY OF TECHNOLOGY',
    'Allgemeine Ingenieurwissenschaften': 'General Engineering Science',
    '- Maschinenbau (2,6)': '- Mechanical Engineering (2.6)',
    'FÄHIGKEITEN': 'SKILLS',
    'Python (inkl. AI - Frameworks)': 'Python (incl. AI frameworks)',
    'CAD (Inventor und Creo)': 'CAD (Inventor and Creo)',
    'Fertigung (Drehen, CNC-Fräsen,': 'Manufacturing (turning, CNC milling,',
    'Löten, allg. Metallbearbeitung)': 'soldering, general metalworking)',
    'SPRACHEN': 'LANGUAGES',
    'Deutsch (Muttersprache)': 'German (native language)',
    'Englisch (Fließend)': 'English (fluent)',
    'SONSTIGES': 'OTHER',
    'Führerschein Klasse B': 'Driving licence class B',
    # Rechte Spalte
    'PROFIL': 'PROFILE',
    'BERUFSERFAHRUNGEN': 'WORK EXPERIENCE',
    'Praktikant und Werkstudent - Qualitätsmanagement': 'Intern and working student - Quality management',
    'Konstruktion und Programmierung automatisierter Teststände': 'Design and programming of automated test benches',
    'Implementierung mithilfe von C++, Arduino, Python': 'Implementation using C++, Arduino, Python',
    'Okt - Dez 2023': 'Oct - Dec 2023',
    'Praktikant': 'Intern',
    'Grundlagen der Metall - und Kunststoffbearbeitung': 'Fundamentals of metal and plastics processing',
    'Vorrichtungsbau für die Produktion': 'Fixture construction for production',
    'Studentische Hilfskraft': 'Student assistant',
    'Vermittlung physikalischer Grundlagen der Flugphysik an Schülern':
        'Teaching pupils the physical fundamentals of flight physics',
    'Schüler für Technik und Luftfahrt begeistert': 'Sparked pupils’ interest in technology and aviation',
    'Schulpraktikant': 'School intern',
    'Einblicke in das Berufsbild des Bauzeichners': 'Insights into the profession of a construction draughtsman',
    'Grundlagen des technischen Zeichnens, AutoCAD, Sketchup': 'Fundamentals of technical drawing, AutoCAD, Sketchup',
    'Einblicke in das Berufsbild des Elektronikers': 'Insights into the profession of an electronics technician',
    'Grundlagen der Elektronik (Löten, PCB-Design)': 'Fundamentals of electronics (soldering, PCB design)',
    'ANHANG': 'ATTACHMENTS',
    'Notenauszug Master-Studium': 'Transcript of records, Master’s programme',
}

# Zeilen, die rechtsbündig stehen (Zeitangaben): die rechte Kante bleibt.
RECHTSBUENDIG = {'Okt - Dez 2023'}

# Der Profiltext läuft über mehrere Zeilen und wird neu umbrochen.
PROFIL_DE = [
    'Mechatronik Student, mit fundierten Kenntnissen in den Bereichen',
    'Maschinenbau, maschinelles Lernen und Python Software-Entwicklung.',
    'Durch Praktika und Werkstudententätigkeiten in der Entwicklung',
    'automatisierter Teststände bringe ich Erfahrungen im',
    'ingenieurswissenschaftlichen Umfeld mit. Auf der Suche nach einer HiWi',
    'Stelle, um technische Lösungen praxisnah mitzuentwickeln, mein Wissen',
    'einzubringen und meine Kenntnisse weiter auszubauen.',
]
PROFIL_EN = (
    'Mechatronics student with solid knowledge in mechanical engineering, machine learning '
    'and Python software development. Through internships and working student jobs in the '
    'development of automated test benches, I bring experience from an engineering environment. '
    'Looking for a HiWi position (student assistant) to help develop practical technical '
    'solutions, contribute my knowledge and further expand my skills.'
)
# Titel des englischen PDFs (Browser-Tab, PDF-Betrachter).
TITEL = 'CV Frederik Eichlohn'

# Rechte Grenze des Profiltexts: so weit wie die längste deutsche Zeile.
PROFIL_BREITE = 322

# Zeilen, die auf Englisch gleich bleiben.
UNVERAENDERT = {
    'FREDERIK', 'EICHLOHN',
    'STUDENT/MECHATRONICS AND', 'INFORMATION TECHNOLOGY/', 'AUTONOMOUS SYSTEMS AND AI',
    'frederik.eichlohn@gmail.com', 'Zähringerstraße 4', '76131 Karlsruhe',
    'Master of Science', 'Mechatronics and Information', 'Technology - autonomous systems', 'and AI',
    '2020 - 2024', 'Bachelor of Science',
    'MS Office', 'Linux, Git/Github, ROS2, CI/CD', 'Matlab/Simulink',
    'Löwenstein Medical Technology GmbH', 'JAN 2024 - SEPT 2024',
    'Möllering Gummi - und Kunststofftechnik GmbH',
    'DLR School Lab TUHH', 'Sept. 2021 - Aug 2023',
    'ECE GmbH', 'Sept. 2018',
    'Hamburger Hochbahn AG', 'Sept. 2016',
}


def schrift_laden(pfad):
    """
    Lädt eine Schrift so, dass jede Glyphe eines ASCII-Zeichens nur noch zu
    diesem Zeichen gehört. Das Leerzeichen teilt sich die Glyphe sonst mit dem
    geschützten Leerzeichen (U+00A0), der Bindestrich mit U+2010; beim Kopieren
    oder automatischen Auslesen des PDFs kämen dann diese Zeichen heraus.
    """
    schrift = TTFont(pfad)
    for tabelle in schrift['cmap'].tables:
        ascii_glyphen = {glyphe for zeichen, glyphe in tabelle.cmap.items() if zeichen < 0x80}
        for zeichen in [z for z, g in tabelle.cmap.items() if z >= 0x80 and g in ascii_glyphen]:
            del tabelle.cmap[zeichen]
    puffer = io.BytesIO()
    schrift.save(puffer)
    return pymupdf.Font(fontbuffer=puffer.getvalue())


def deutsches_pdf():
    pdfs = sorted(p for p in ORDNER.glob('*.pdf') if not p.name.lower().endswith('.en.pdf'))
    if len(pdfs) != 1:
        sys.exit(f'In {ORDNER} muss genau ein deutsches PDF liegen, gefunden: {[p.name for p in pdfs]}')
    return pdfs[0]


def zeilen_lesen(seite):
    """Alle Textzeilen der Seite mit Schrift, Größe, Farbe und Zeichenlagen."""
    zeilen = []
    for block in seite.get_text('rawdict')['blocks']:
        if block['type'] != 0:
            continue
        for zeile in block['lines']:
            for span in zeile['spans']:
                text = ''.join(z['c'] for z in span['chars'])
                if text.strip():
                    zeilen.append({**span, 'text': text, 'schluessel': text.strip()})
    return zeilen


def sperrung(span, schrift):
    """Abstand, den der Satz zusätzlich zwischen zwei Zeichen lässt (in pt)."""
    zeichen = [z for z in span['chars']]
    if len(zeichen) < 2:
        return 0.0
    vorschub = sum(schrift.text_length(z['c'], span['size']) for z in zeichen[:-1])
    return (zeichen[-1]['origin'][0] - zeichen[0]['origin'][0] - vorschub) / (len(zeichen) - 1)


def breite(text, schrift, groesse, sperr):
    return sum(schrift.text_length(c, groesse) for c in text) + sperr * (len(text) - 1)


def umbrechen(text, schrift, groesse, grenze):
    zeilen, aktuell = [], ''
    for wort in text.split():
        probe = f'{aktuell} {wort}'.strip()
        if aktuell and schrift.text_length(probe, groesse) > grenze:
            zeilen.append(aktuell)
            aktuell = wort
        else:
            aktuell = probe
    if aktuell:
        zeilen.append(aktuell)
    return zeilen


def main():
    quelle = deutsches_pdf()
    ziel = quelle.with_name(quelle.stem + '.en.pdf')

    for name, pfad in SCHRIFTDATEIEN.items():
        if not pfad.exists():
            sys.exit(f'Schrift {name} fehlt: {pfad}')
    schriften = {name: schrift_laden(pfad) for name, pfad in SCHRIFTDATEIEN.items()}

    doc = pymupdf.open(quelle)
    if doc.page_count != 1:
        sys.exit(f'Erwartet wird ein einseitiges PDF, {quelle.name} hat {doc.page_count} Seiten.')
    seite = doc[0]
    zeilen = zeilen_lesen(seite)

    # Jede Zeile muss bekannt sein, jede bekannte Zeile muss vorkommen.
    bekannt = set(ZEILEN) | set(PROFIL_DE) | UNVERAENDERT
    unbekannt = [z['schluessel'] for z in zeilen if z['schluessel'] not in bekannt]
    gefunden = {z['schluessel'] for z in zeilen}
    fehlend = sorted((set(ZEILEN) | set(PROFIL_DE) | UNVERAENDERT) - gefunden)
    if unbekannt or fehlend:
        if unbekannt:
            print('Zeilen ohne Übersetzung (in ZEILEN, PROFIL_DE oder UNVERAENDERT eintragen):')
            for text in unbekannt:
                print(f'  {text!r}')
        if fehlend:
            print('Zeilen aus dem Skript, die im PDF nicht mehr vorkommen:')
            for text in fehlend:
                print(f'  {text!r}')
        sys.exit(1)

    # Was neu gesetzt wird: (Schrift, Größe, Farbe, x, y, Sperrung, Text)
    neu = []
    for z in zeilen:
        if z['schluessel'] not in ZEILEN:
            continue
        if z['font'] not in schriften:
            sys.exit(f'Für die Schrift {z["font"]} ({z["schluessel"]!r}) liegt keine Datei vor.')
        schrift = schriften[z['font']]
        sperr = sperrung(z, schrift)
        text = ZEILEN[z['schluessel']]
        x, y = z['chars'][0]['origin']
        if z['schluessel'] in RECHTSBUENDIG:
            letztes = [c for c in z['chars'] if c['c'].strip()][-1]
            rechts = letztes['origin'][0] + schrift.text_length(letztes['c'], z['size'])
            x = rechts - breite(text, schrift, z['size'], sperr)
        neu.append((z['font'], z['size'], z['color'], x, y, sperr, text))

    profil = [z for z in zeilen if z['schluessel'] in PROFIL_DE]
    profil.sort(key=lambda z: z['origin'][1])
    erste = profil[0]
    schrift = schriften[erste['font']]
    umbrochen = umbrechen(PROFIL_EN, schrift, erste['size'], PROFIL_BREITE)
    if len(umbrochen) > len(profil):
        sys.exit(f'Der englische Profiltext braucht {len(umbrochen)} Zeilen, Platz ist für {len(profil)}.')
    for zeile, text in zip(profil, umbrochen):
        x, y = zeile['chars'][0]['origin']
        neu.append((zeile['font'], zeile['size'], zeile['color'], x, y, sperrung(zeile, schrift), text))

    # Deutschen Text entfernen. Nur Text: Flächen, Linien und Bilder bleiben.
    # Das Rechteck ist ein schmaler Streifen um die Zeilenmitte, damit die
    # Nachbarzeilen sicher unberührt bleiben.
    for z in zeilen:
        if z['schluessel'] in ZEILEN or z['schluessel'] in PROFIL_DE:
            x0, y0, x1, y1 = z['bbox']
            mitte = (y0 + y1) / 2
            seite.add_redact_annot(pymupdf.Rect(x0, mitte - 1.5, x1, mitte + 1.5), fill=False)
    seite.apply_redactions(
        images=pymupdf.PDF_REDACT_IMAGE_NONE,
        graphics=pymupdf.PDF_REDACT_LINE_ART_NONE,
        text=pymupdf.PDF_REDACT_TEXT_REMOVE,
    )

    # Englischen Text Zeichen für Zeichen setzen, mit der Sperrung des Originals.
    schreiber = {}
    for font, groesse, farbe, x, y, sperr, text in neu:
        schrift = schriften[font]
        tw = schreiber.setdefault(farbe, pymupdf.TextWriter(seite.rect))
        for zeichen in text:
            tw.append((x, y), zeichen, font=schrift, fontsize=groesse)
            x += schrift.text_length(zeichen, groesse) + sperr
    for farbe, tw in schreiber.items():
        rgb = tuple(((farbe >> verschiebung) & 0xFF) / 255 for verschiebung in (16, 8, 0))
        tw.write_text(seite, color=rgb)

    # Titel, den der Browser-Tab beim Öffnen des PDFs zeigt. Er steht doppelt:
    # im Dokumentinfo und in den XMP-Metadaten.
    doc.set_metadata({**doc.metadata, 'title': TITEL})
    xmp = doc.get_xml_metadata()
    if '<dc:title>' in xmp:
        vor, rest = xmp.split('<dc:title>', 1)
        titel_xmp, nach = rest.split('</dc:title>', 1)
        titel_xmp = re.sub(r'(<rdf:li [^>]*>)[^<]*(</rdf:li>)', rf'\g<1>{TITEL}\g<2>', titel_xmp)
        doc.set_xml_metadata(f'{vor}<dc:title>{titel_xmp}</dc:title>{nach}')

    doc.subset_fonts()
    doc.save(ziel, garbage=3, deflate=True)
    print(f'{ziel.relative_to(WURZEL)} geschrieben ({ziel.stat().st_size // 1024} KB).')


if __name__ == '__main__':
    main()
