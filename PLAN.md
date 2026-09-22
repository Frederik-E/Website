# Implementierungsplan: Persönliche Portfolio-Website

Dieser Plan ist die Quelle der Wahrheit für dieses Projekt. `CLAUDE.md` fasst ihn kurz zusammen.

## Arbeitsweise

- Der Plan wird Phase für Phase umgesetzt. Nach jeder Phase wird angehalten, das Ergebnis gezeigt und auf Freigabe gewartet. Nach der Freigabe folgt ein Git-Commit.
- Für alles rund um Astro gilt die aktuelle Doku der installierten Version. Widerspricht dieser Plan der Doku von Astro oder Cloudflare, gilt die Doku; Abweichungen werden genannt.
- Visuelle Prüfung: Nach jeder Änderung am Aussehen Screenshots erzeugen (`npm run shots`), ansehen und gegen die Design-Regeln abgleichen, bevor eine Phase als fertig gemeldet wird.
- Schritte, die der Betreiber der Seite selbst erledigen muss (Logins, Accounts, eigene Texte), werden deutlich als **Aufgabe für mich** markiert und kurz erklärt.
- Die Abschnitte „Design“, „Schreibregeln“ und „Inhalte“ gelten in jeder Phase.

## Ziel

Eine schnelle, statische Portfolio-Website für einen angehenden Ingenieur im Bereich autonome Systeme und KI. Sie soll wirken wie die Seite eines Menschen, der seine Arbeit ernst nimmt, nicht wie ein generiertes Template. Die Projekte mit Bildern und Videos sind der Kern.

Pflege-Anforderungen, nach Wichtigkeit:

1. Projekte mit wenig Aufwand hinzufügen (im einfachsten Fall: Titel, ein Satz, ein Link)
2. Informationen, Bilder und Videos einfach ablegen, ohne Code anzufassen
3. Einen kurzen Text über mich schreiben und ändern
4. Änderungen schnell veröffentlichen, beliebig oft

## Technische Entscheidungen

| Bereich | Entscheidung | Grund |
|---|---|---|
| Framework | Astro 6 oder neuer, TypeScript strict | Statische Seiten, Inhalte als Markdown, kein unnötiges JavaScript |
| Styling | Reines CSS mit Custom Properties | Volle Kontrolle, keine Utility-Klassen-Optik |
| Schrift | Geist und Geist Mono über Astros eingebaute Fonts-API mit Fontsource-Provider | Schriftdateien werden beim Build geladen und von der eigenen Domain ausgeliefert, mit Preload und angepassten Fallback-Metriken |
| Inhalte | Ordner `inhalt/` mit Markdown und YAML, geprüft mit Zod 4 aus `astro/zod` (z. B. `z.url()` statt des veralteten `z.string().url()`) | Alles, was gepflegt wird, liegt an einem Ort; Fehler werden beim Build klar gemeldet |
| Bilder | `astro:assets` | Automatische Optimierung und responsive Größen |
| Hosting | Cloudflare Workers mit statischen Assets, Upload per `wrangler deploy`, ohne Adapter | Anfragen an statische Dateien sind kostenlos und unbegrenzt, und Deploys verbrauchen kein Kontingent |
| Werkzeuge | Playwright (Chromium) mit `@axe-core/playwright` | Screenshots, Barrierefreiheits-Test, Prüfung auf fremde Anfragen, PDF des Lebenslaufs |
| Skripte | Node-Skripte in `scripts/*.mjs` | Laufen unter Windows, macOS und Linux gleich |

URLs einheitlich mit Schrägstrich am Ende (`trailingSlash: "always"`), Seiten-URL (`site`) aus den Einstellungen.

---

## Design

**Freigegeben am 2026-09-22.** Der erste Entwurf „Arbeitsmappe“ (heller Grund, sehr zurückhaltend, eine schmale Spalte) sah laut Betreiber aus „wie nach 20 Minuten selbst gecodet“. Ein zweiter, noch reduzierterer Entwurf war ihm zu schlicht: er wollte ausdrücklich mehr grafische Elemente. Freigegeben ist deshalb das Konzept „Technische Zeichnung“. Der Betreiber hat es ausdrücklich als gelungen bezeichnet; es ist der Maßstab für spätere Änderungen.

### Konzept „Technische Zeichnung“

Die Seite ist ein Zeichnungsblatt: Randlinien, Felder mit Kreuzmarken, Millimeterraster, Maßlinien, Sucherecken, ein Schriftfeld und Beschriftungen in Monospace. Das passt zu einem Ingenieur für autonome Systeme. Das weiche, bewegte Zeichen steht als Kontrast im strengen Raster. Die Projekte mit Bildern und Videos bleiben der Kern.

### Farben

Die Palette bleibt. Alle weiteren Töne sind mit `color-mix` aus Iron Grey und Lavender Mist abgeleitet, es gibt keinen neuen Farbton.

| Token | Wert | Rolle |
|---|---|---|
| `--color-bg` | `#FCF7FF` Lavender Mist | Seitenhintergrund, Schrift auf Dunkel |
| `--color-text` | `#394648` Iron Grey | Text, Überschriften, Grund der dunklen Fußzeile |
| `--color-text-muted` | `#60696D` | Sekundärtext und alle Etiketten |
| `--color-link` | `#8C271E` Chestnut | Links im Fließtext, Fokus-Ring |
| `--color-underline` | `#B98389` Dusty Rose | Unterstreichung der Links im Fließtext |
| `--color-mark` | `#65AFFF` Cool Sky | Zeichen, Marker, Messkreis, Sucherecken beim Zeigen, Farbschimmer |
| `--color-mark-soft` | `#B8D7FF` | Textmarker, Textauswahl |
| `--color-line` | `#B98389` Dusty Rose | zweite Form des Zeichens, Farbschimmer |
| `--color-haarlinie` | 16 % Iron Grey auf Mist | Randlinien, Feldlinien, Zellen |
| `--color-flaeche` | 6 % Iron Grey auf Mist | Fläche beim Zeigen, Grund hinter ladenden Bildern |
| `--color-konstruktion` | 38 % Iron Grey auf Mist | Kreuzmarken, Mittellinien, Sucherecken, Maßlinien. **Nie für Text** |
| `--color-raster` | 7 % Iron Grey auf Mist | Millimeterraster |
| `--color-auf-dunkel-leise` | 70 % Mist auf Iron Grey | Etiketten auf der dunklen Fläche |
| `--color-linie-dunkel` | 18 % Mist auf Iron Grey | Linien auf der dunklen Fläche |

Gemessene Kontraste (WCAG):

| Kombination | Kontrast | Folge |
|---|---|---|
| Iron Grey auf Mist | 9,3 : 1 | für allen Text |
| Muted auf Mist | 5,3 : 1 | für Sekundärtext und Etiketten ab 12 px |
| Chestnut auf Mist | 8,2 : 1 | für Links und Fokus-Ring |
| Iron Grey auf Marker-Soft | 6,6 : 1 | markierter Text bleibt lesbar |
| Mist auf Iron Grey | 9,3 : 1 | Text auf der dunklen Fläche |
| Mist-leise auf Iron Grey | 5,4 : 1 | Etiketten auf der dunklen Fläche |
| Cool Sky auf Iron Grey | 4,3 : 1 | nur als Linie oder Schimmer, nie für kleinen Text |
| Cool Sky auf Mist | 2,2 : 1 | nie für Text oder Fokus-Ring |
| Konstruktion auf Mist | unter 3 : 1 | nie für Text; axe meldet es sonst |
| Chestnut gegen Iron Grey | 1,1 : 1 | Links im Fließtext unterscheiden sich nur durch die Unterstreichung, die daher dort immer sichtbar ist |

`<meta name="color-scheme" content="light">` bleibt gesetzt. Kein Dark Mode.

### Das Zeichen und der Marker

**Das Zeichen**, klein in der Kopfzeile jeder Seite und groß als „Abb. 01“ im Startbild: **zwei organische Formen übereinander**. Eine in Cool Sky, darüber leicht versetzt und gedreht eine deckende in Dusty Rose. Beide quillen und wandern langsam und schieben sich dadurch dynamisch übereinander.

**Zur Farbwahl:** Dusty Rose ist die einzige Palettenfarbe ohne Textrolle und bildet warm gegen kalt einen echten Kontrast. Eine hellere Fassung von Cool Sky wurde verworfen, sie sah aus wie ein Schatten.

**Zum Überlapp:** Eine Form deckt immer die andere. Im Überlapp steht also Rose oder Blau, nie eine Mischfarbe. Ein Überdruck mit `mix-blend-mode: multiply` (tiefes Indigo im Überlapp) wurde ausprobiert und verworfen: die dritte Farbe gehört nicht zur Palette.

**Zur Reihenfolge:** Zweimal in 40 Sekunden gehen die Formen schräg auseinander, bis sie sich nicht mehr berühren, und kommen mit getauschter Reihenfolge zurück: erst Blau vorn, dann wieder Rose vorn. Getauscht wird nur, wenn sie getrennt sind, sonst würde die Farbe im Überlapp springen.

**Verworfen:** Schatten am Zeichen, sowohl ein leichter Schlagschatten als auch ein innerer Schatten (Zeichen eine Ebene unter der Seite). Beides sah nicht stimmig aus.

**Technisch:** `<path>`-Elemente mit denselben `@keyframes`, die rosa Form mit negativer `animation-delay`, damit beide an verschiedenen Stellen ihrer Bewegung stehen. Für den Wechsel der Reihenfolge liegt eine zweite blaue Form ganz oben und wird ein- und ausgeschaltet, weil SVG kein `z-index` kennt. Kein GIF, kein JavaScript, kein SMIL. `npm run zeichen` erzeugt `src/styles/zeichen.css` und `src/lib/zeichen.ts` aus `scripts/zeichen-erzeugen.mjs`. Bei `prefers-reduced-motion: reduce` und im Druck stehen die Formen still.

**Verworfen:** Der Blob formte sich zwischendurch zu Piktogrammen (Roboter, Antenne, Chip, Zahnrad). Der Code bleibt im Generator hinter `MIT_PIKTOGRAMMEN`, steht aber auf `false`.

**Der Marker** (Textmarker-Strich in `--color-mark-soft`) tritt an drei Stellen auf: aktiver Navigationspunkt zusätzlich zu `aria-current`, `<mark>` in eigenen Texten (höchstens einmal pro Seite, nie in Überschriften) und `hr` im Fließtext. Der frühere Gliederungsstrich `.trenner` ist durch die Felder ersetzt.

### Links und Fokus

- **Im Fließtext:** Chestnut, Unterstreichung immer sichtbar in Dusty Rose, 1px, `text-underline-offset: 0.25em`. Beim Zeigen wird sie Chestnut.
- **Ruhig** (`.ruhig`): Navigation, Titel und Listen, in Textfarbe. Ihre Stellung zeigt, dass sie Links sind; die Unterstreichung erscheint erst beim Zeigen.
- **Dezent** (`.dezent`): einzeln stehende Verweise wie „Code auf GitHub“, in Textfarbe mit feiner Unterstreichung.
- **Ganze Einträge** sind klickbar: der Titel-Link spannt sich über den Eintrag. Weitere Links darin liegen darüber.
- Fokus: `outline: 2px solid var(--color-link); outline-offset: 4px;`
- Es gibt keine Schaltflächen. „Lebenslauf herunterladen (PDF)“ ist ein Textlink auf `/cv/`, der `/cv.pdf` mit dem Dateinamen `Vorname-Nachname-CV.pdf` lädt.

### Typografie

- **Geist** (400, 500, dazu 400 kursiv als eigener Eintrag) für alles. **Geist Mono** (400) für Etiketten, Nummern, Datumsangaben in Listen und Code. Subset `latin`.
- Überschriften im Gewicht 500 und eng gesetzt: je größer, desto enger (bis `-0.045em` im Startbild). `strong` ist 500.
- **Etiketten** (`.etikett`): Geist Mono, 12 px, Versalien, gesperrt um `0.08em`, in Muted.
- Fließtext `1.0625rem` (17 px), Zeilenhöhe 1.65, Textbreite höchstens `64ch`.
- Größen stehen in `tokens.css` (`--text-klein` bis `--text-display`), mit `clamp()` fließend.

### Aufbau

- **Blatt** (`.blatt`): höchstens `76rem` breit. Zwei senkrechte Randlinien laufen durch Kopfzeile, Inhalt und Fußzeile.
- **Felder** (`.feld`): jeder Abschnitt hat oben eine Haarlinie von Rand zu Rand mit einer Kreuzmarke an beiden Enden. `.knapp` für schmale Zeilen, `.ohne-linie` für das erste Feld unter der Kopfzeile.
- **Raster:** links ein Drittel für Nummer und Einordnung, rechts zwei Drittel für den Inhalt. `.geteilt` zeichnet ab `60rem` die senkrechte Linie in der Mitte der Lücke. `.zweispaltig` und `.gegliedert` (Zwischenüberschrift links neben ihrem Absatz, automatisch nummeriert) folgen demselben Raster.
- **Zellen** (`.zellen`): wie im Schriftfeld. Die erste Spalte ist genau so breit, dass ihre Linie mit der Teilungslinie fluchtet (`--erste-spalte`).
- **Kopfzeile:** bleibt oben stehen, halb durchsichtig mit Weichzeichner. Links das Zeichen (Link zur Startseite), rechts die Navigation mit Nummern: 01 Projekte, 02 Über mich, 03 Lebenslauf, danach freie Seiten mit `navigation: true`. Die Nummern kommen aus `src/lib/navigation.ts` und erscheinen auch im Seitenkopf.
- **Seitenkopf** (`Seitenkopf.astro`): Etiketten links (Nummer und Seite) und rechts (Stand), große Überschrift, optional ein Einleitungssatz, Millimeterraster im Hintergrund.
- **Fußzeile:** dunkel in Iron Grey mit zwei weichen Farbschimmern. Oben „Kontakt“ mit der E-Mail-Adresse sehr groß, dazu GitHub und LinkedIn. Darunter das Schriftfeld mit Name, Seite, Pfad, Stand und Rechtlichem.
- **Der Name** steht im Schriftfeld, im Browser-Tab, im JSON-LD und im Lebenslauf-PDF. In der Kopfzeile nicht.
- **Kein Portraitfoto, kein Suchstatus.**
- Mobil gilt derselbe Aufbau, einspaltig. Die Nummern in der Navigation entfallen unter `34rem`.

### Nur echte Angaben in der Zeichnung

Schriftfeld, Datenleiste, Etiketten und Maßlinien zeigen nur, was aus den Daten folgt: Pfad der Seite, Baumonat (`STAND` in `src/lib/text.ts`), Anzahl der Projekte, Datum des neuesten, Themen aus den Kopfdaten, Pixelmaße der Bilddatei. Keine erfundenen Koordinaten, Versionsnummern oder Kennzahlen über die Person.

### Seiten im Einzelnen

- **Startseite:** Startbild mit Einleitung groß links und dem Zeichen als „Abb. 01“ rechts (Mittellinien, gestrichelter Messkreis in Cool Sky, Millimeterraster, Farbschimmer). Darunter die Datenleiste. Dann „01 Ausgewählte Arbeiten“ als Datenblätter (`ProjectEntry`): links Projektnummer, Titel und Angaben, die beim Scrollen stehen bleiben; rechts Kurztext und Titelbild. Zuletzt „02 Weitere Inhalte“.
- **Projektnummer:** `P-01` ab dem ältesten Projekt (`projektNummer`), damit neue Projekte die alten Nummern nicht verschieben.
- **Abbildung** (`Abbildung.astro`): Sucherecken an allen vier Ecken, darunter eine Maßlinie mit Endstrichen und den Pixelmaßen, bei laufendem Video „Video“.
- **Projektübersicht:** Stückliste (`Stueckliste.astro`) mit den Spalten Nr., Vorschau, Titel, Themen, Datum. Ohne Titelbild steht der technische Platzhalter, ein Rechteck mit Diagonalkreuz.
- **Projektseite:** Seitenkopf, Angaben als Zellen (Datum, Rolle, Themen, Links), Abbildung in voller Breite, gegliederter Text, am Ende nächstes Projekt und alle Projekte.
- **Lebenslauf:** nummerierte Abschnitte im Raster, Zeitraum als Etikett rechts. Das PDF übernimmt den Stil ohne Randlinien und Kreuzmarken.
- Nie Karten mit Schatten, nie identische Kacheln.

### Bewegung

- Linien zeichnen sich, wenn ihr Feld ins Bild kommt; Inhalte mit `.erscheinen` blenden ein. Beides per `animation-timeline: view()`, ohne JavaScript. Browser ohne Scroll-Animationen zeigen alles sofort.
- Überschriften steigen beim Laden auf, der Messkreis dreht sich in 80 s, der Farbschimmer schwebt.
- Beim Zeigen: Bilder rücken leicht näher, Sucherecken ziehen sich in Cool Sky zusammen, Zeilen bekommen die Fläche als Grund, in der Navigation wächst eine Linie unter dem Wort.
- Bei `prefers-reduced-motion: reduce` steht alles still. Im Druck gibt es keine Bewegung.

**Offener Punkt zur Barrierefreiheit:** Das Zeichen, der Messkreis und der Schimmer laufen in einer Endlosschleife, also länger als fünf Sekunden. WCAG 2.2.2 verlangt dafür streng genommen eine Möglichkeit, sie anzuhalten. Umgesetzt ist `prefers-reduced-motion`. Das ist bewusst so entschieden, weil die Bewegung zum Merkmal der Seite gehört.

### Verbotsliste

Mit dem Konzept „Technische Zeichnung“ sind erlaubt: Etiketten in Monospace und Versalien, Nummern (01, P-01), Farbverläufe als weicher Schimmer, das Millimeterraster, eine dunkle Fläche, Einblenden beim Scrollen, Effekte beim Zeigen und die durchscheinende Kopfzeile.

Weiterhin nicht verwenden:

- Rahmen oder Kästen um Textblöcke; Karten mit Schatten; identische Kacheln; Glaseffekt auf Inhaltsflächen
- Leuchtende oder pulsierende Punkte, Status-Badges wie „Available for work“
- Neon-Akzente, harte Verläufe als Fläche
- Hervorhebung eines einzelnen Wortes in einer Überschrift durch Farbe, Kursiv oder Fett
- Metadaten mit Mittelpunkten verbunden („2025 · ROS 2 · Python“)
- Pfeile an Linktexten, Emojis, Icon-Reihen als Dekoration
- Cursor-Effekte, Partikel, Tipp-Animationen, Parallax
- Hero mit großen Kennzahlen, „Hi, I'm …“
- Kontaktformulare, Chat-Widgets, Cookie-Banner (die Seite setzt keine Cookies)

### Videos

- `muted loop playsinline preload="none"`, Posterbild ist das optimierte Titelbild, `aria-label` aus `titelbild_alt`.
- Ohne JavaScript: Video mit `controls`, kein Autoplay.
- Mit JavaScript (ein kleines, von Astro gebündeltes `<script>`, kein Framework): Das Video startet, sobald es sichtbar wird (IntersectionObserver), und pausiert, wenn es den Bildschirm verlässt. Bei `prefers-reduced-motion: reduce` kein Autoplay, nur Steuerelemente.
- Im Druck erscheint statt des Videos das Posterbild.

---

## Schreibregeln

Gelten für alle Texte im Projekt (Platzhalter, Oberflächentexte, Anleitung):

- Ich-Form, kurze Sätze, konkrete Angaben (Zahlen, Werkzeuge, eigene Rolle).
- Keine Floskeln. Verboten sind u. a.: passionate, cutting-edge, innovative, seamless, leverage, unlock, empower, journey, delve, robust (als Füllwort), „leidenschaftlich“, „innovativ“, „nahtlos“, „modernste“.
- Keine Gedankenstriche als Stilmittel, keine rhetorischen Fragen, keine Emojis.
- Oberflächentexte sagen genau, was passiert, z. B. „Code auf GitHub“, „Lebenslauf herunterladen (PDF)“, „Alle Projekte“ (auf Englisch entsprechend).
- Keine Inhalte über die Person erfinden. Wo Text fehlt, steht ein Platzhalter der Form `[TODO: …]`. Die Veröffentlichung bricht ab, solange ein Platzhalter auf einer veröffentlichten Seite steht.

---

## Inhalte

Alles Pflegbare liegt im Ordner `inhalt/`. Code liegt in `src/` und `scripts/`, Vorlagen in `vorlagen/`.

```
inhalt/
  einstellungen.yaml        Sprache, Name, Untertitel, Einleitung, Links, Domain
  ueber-mich.md             Kurzer Text über mich (60–120 Wörter)
  seiten/                   Frei angelegte Seiten, eine Datei pro Seite
  lebenslauf.yaml           Stationen und Kenntnisse
  lebenslauf.pdf            Optional: eigenes PDF; fehlt es, wird eines erzeugt
  impressum.md
  datenschutz.md
  projekte/
    fahrerloses-modellauto/
      index.md              Kopfdaten und Beschreibung
      titelbild.jpg
      demo.mp4
      aufbau.png            weitere Bilder für den Text
vorlagen/
  projekt.de.md
  projekt.en.md
```

### einstellungen.yaml

- Einlesen per Vite-Import mit `?raw` (der Dev-Server zeigt Änderungen sofort), parsen mit dem Paket `yaml`, prüfen mit Zod. `astro.config` liest `domain` für `site`.
- Im Dev-Betrieb sind `[TODO`-Werte erlaubt, damit die Seite sofort läuft. `npm run check` verlangt gültige Werte (E-Mail, URLs) und meldet `example.com` als Fehler.

### Kopfdaten eines Projekts (`index.md`)

| Feld | Pflicht | Bedeutung |
|---|---|---|
| `titel` | ja | Projektname |
| `kurz` | ja | Ein bis zwei Sätze für die Übersicht, höchstens 200 Zeichen |
| `datum` | ja | Format `JJJJ-MM`; bestimmt die Reihenfolge |
| `rolle` | nein | z. B. „Allein“ oder „Perception im Viererteam“ |
| `tags` | nein | Liste, z. B. `[ROS 2, SLAM, Python]` |
| `titelbild` | wenn `video` gesetzt | Relativer Pfad, z. B. `./titelbild.jpg`; dient auch als Posterbild |
| `titelbild_alt` | wenn `titelbild` gesetzt | Bildbeschreibung |
| `video` | nein | Dateiname im Projektordner, z. B. `demo.mp4` |
| `code` | nein | Link zum Repository |
| `demo` | nein | Link zu einer Live-Demo, einem Video oder einem Paper |
| `startseite` | nein | `true` zeigt das Projekt auf der Startseite |
| `reihenfolge` | nein | Zahl; kleinere Werte stehen auf der Startseite weiter oben |
| `entwurf` | nein | `true` blendet das Projekt auf der veröffentlichten Seite aus |

Regeln:

- Collection mit dem `glob()`-Loader, Basis `./inhalt/projekte`, Muster `*/index.md`. Die URL ist `/projects/<ordnername>/`.
- Bilder über den `image()`-Helfer im Schema. Videos per `import.meta.glob` mit `?url` auflösen.
- Ist der Text unter den Kopfdaten leer, wird **keine Detailseite** erzeugt. Der Eintrag verlinkt dann direkt auf `code` bzw. `demo` oder ist ein reiner Texteintrag.
- Entwürfe erscheinen in `npm run dev` mit dem schlichten Texthinweis „Entwurf“ und fehlen im Build.
- Startseite: Projekte mit `startseite: true`, sortiert nach `reihenfolge`, dann `datum`. Gibt es keine, die drei neuesten.
- Fehlerhafte Kopfdaten brechen den Build mit einer verständlichen deutschen Meldung ab, die Datei und Feld nennt.

### Vorlage für Projekte

Kopfdaten mit einem YAML-Kommentar pro Feld. Darunter diese Gliederung, jeweils mit einem `[TODO: …]`-Hinweis (englische Fassung: Problem, Approach, Result, What I learned):

```markdown
## Problem
## Ansatz
## Ergebnis
## Was ich gelernt habe
```

Wer keine Detailseite möchte, löscht alles unter den Kopfdaten. Vergessene Platzhalter fängt `npm run check` ab.

### lebenslauf.yaml

Frei benennbare Abschnitte in fester Reihenfolge, zwei Eintragsarten (Stationen und Listen).

- `startseite: true` an einem Abschnitt hat seit dem Umbau keine Wirkung mehr, weil die Startseite nur Arbeiten zeigt. Das Feld bleibt für eine spätere Kurzfassung erhalten.
- Keine Adresse, keine Telefonnummer, kein Geburtsdatum.

### Lebenslauf als PDF

- Liegt `inhalt/lebenslauf.pdf` vor, wird es vor dem Build nach `public/cv.pdf` kopiert.
- Sonst erzeugt `npm run cv` das PDF aus der gebauten Seite `/cv/` mit Playwright (A4, Druck-Stylesheet, ohne Navigation und Footer, Domain und E-Mail in der Fußzeile) und schreibt es nach `public/cv.pdf` und `dist/cv.pdf`.
- `public/cv.pdf` steht in `.gitignore`.

### Rechtliches

`impressum.md` und `datenschutz.md` enthalten Platzhalter. Die Datenschutz-Vorlage nennt als Stichpunkte: Hosting bei Cloudflare (Server-Logs), gegebenenfalls E-Mail-Weiterleitung über Cloudflare, Kontakt per E-Mail, keine Cookies, kein Tracking, Schriften und Videos selbst gehostet.

---

## Seiten

URLs sind unabhängig von der Sprache gleich.

| Route | Inhalt |
|---|---|
| `/` | Startbild mit Einleitung und dem Zeichen als Abbildung, Datenleiste, ausgewählte Arbeiten als Datenblätter, dann „Weitere Inhalte“ mit Über mich, Lebenslauf und allen Seiten aus `inhalt/seiten/` |
| `/projects/` | Alle veröffentlichten Projekte als Stückliste, neueste zuerst, keine Filter |
| `/projects/<ordner>/` | Medium, Titel, Metadaten als Definitionsliste, Text, Links zu Code und Demo, Link zum nächsten Projekt mit Detailseite |
| `/about/` | Kurzer Text über mich aus `inhalt/ueber-mich.md` |
| `/<name>/` | Frei angelegte Seite aus `inhalt/seiten/<name>.md` |
| `/cv/` | Vollständiger Lebenslauf und PDF-Link; mit Druck-Stylesheet |
| `/impressum/` | Aus `inhalt/impressum.md`; `noindex`; Titel in beiden Sprachen „Impressum“ |
| `/datenschutz/` | Aus `inhalt/datenschutz.md`; `noindex`; Titel „Datenschutz“ bzw. „Privacy“ |
| `/404` | Kurzer Hinweis und Link zur Startseite |

- Oberflächentexte für `en` und `de` in `src/i18n.ts`; Datumsangaben mit `Intl.DateTimeFormat` in der gewählten Sprache.
- `@astrojs/sitemap` ohne Impressum und Datenschutz, `robots.txt` mit Verweis auf die Sitemap.
- Pro Seite: Titel, Beschreibung, `canonical`-Link, Open-Graph-Tags (Titelbild des Projekts, als 1200×630 erzeugt).
- JSON-LD vom Typ `Person` mit Name, URL und `sameAs` (GitHub, LinkedIn).

---

## Befehle

| Befehl | Wirkung |
|---|---|
| `npm run dev` | Lokale Vorschau, inklusive Entwürfe |
| `npm run new -- "Titel"` | Legt `inhalt/projekte/<ordner>/index.md` aus der Vorlage an (`datum` = aktueller Monat, `entwurf: true`) |
| `npm run new-seite -- "Titel"` | Legt `inhalt/seiten/<name>.md` aus der Vorlage an, erreichbar unter `/<name>/` |
| `npm run zeichen` | Erzeugt die Formen des Zeichens neu, nach Änderungen in `scripts/zeichen-erzeugen.mjs` |
| `npm run video -- <datei> <ordner>` | Komprimiert ein Video mit ffmpeg (H.264, höchstens 1280px breit, 30 fps, ohne Ton, `+faststart`) als `demo.mp4` in den Projektordner und trägt `video` ein |
| `npm run shots` | Screenshots aller Seiten bei 390px und 1440px Breite nach `.screenshots/` |
| `npm run cv` | Baut die Seite und erzeugt das PDF, falls kein eigenes vorliegt |
| `npm run check` | `astro check`, Build, Vorschau-Server, PDF, dann auf allen Seiten: axe-Test, interne Links, Suche nach `[TODO` und `example.com`, Protokoll aller Anfragen, Prüfung der Einstellungen, Warnung bei HEIC/GIF und Bildern über 5 MB |
| `npm run deploy` | `check`, und nur wenn alles fehlerfrei ist: `wrangler deploy`. Mit `-- --dry-run` alles außer dem Hochladen |

---

## Phasen

### Phase 1: Grundgerüst
1. Astro-Projekt, TypeScript strict, Git. `.gitignore` um `.screenshots/`, `.wrangler/`, `dist/` und `public/cv.pdf` ergänzen.
2. Fonts-API mit IBM Plex Sans und IBM Plex Mono.
3. `src/styles/tokens.css` und `src/styles/global.css`.
4. `BaseLayout.astro` mit Seitenleiste, Inhaltsspalte, mobilem Kopf, Footer und Sprunglink.
5. Playwright und Chromium, `npm run shots`.
6. `CLAUDE.md`.

**Fertig, wenn:** die leere Seite auf den Screenshots bei beiden Breiten Farben, Schrift und Layout korrekt zeigt und Playwright keine Anfrage an fremde Domains protokolliert.

### Phase 2: Inhaltsmodell
1. Ordner `inhalt/` und `vorlagen/` mit Platzhaltern.
2. Schemas für Einstellungen, Projekte und Lebenslauf; `src/i18n.ts`.
3. Zwei Beispielprojekte: eines mit Text, Titelbild und Video, eines nur mit Kopfdaten und Code-Link.

**Fertig, wenn:** der Build durchläuft, das Video im Build ausgeliefert wird, die Projekt-URL dem Ordnernamen entspricht und ein absichtlich falsches Feld eine verständliche Meldung mit Datei und Feld erzeugt.

### Phase 3: Seiten und Komponenten
1. Alle Seiten aus der Tabelle.
2. Komponenten `ProjectEntry.astro`, `ProjectMedia.astro`, `CvSection.astro`.
3. Druck-Stylesheet für `/cv/`.

**Fertig, wenn:** alle Seiten erreichbar sind, das Layout-Ziel bei 1440×900 erfüllt ist und jede Seite anhand der Screenshots gegen die Verbotsliste geprüft wurde.

### Phase 4: Pflege-Werkzeuge
1. Skripte `new`, `video`, `cv`, `check` und `deploy`; `deploy` zunächst mit `--dry-run` testen.
2. `wrangler` als Entwicklungsabhängigkeit und `wrangler.jsonc`: `name`, aktuelles `compatibility_date`, `assets` mit `directory: "./dist"` und `not_found_handling: "404-page"`. Kein Adapter, keine Worker-Datei.
3. Anleitungen auf Deutsch, jede Aufgabe in höchstens fünf Schritten: `ANLEITUNG.md` als Übersicht, dazu `ANLEITUNG-START.md` (Domain bis Veröffentlichung), `ANLEITUNG-INHALTE.md` (Projekte, Seiten, Bilder, Videos, Lebenslauf) und `ANLEITUNG-VEROEFFENTLICHEN.md` (Hochladen, Fehlermeldungen).

**Fertig, wenn:** sich mit `npm run new` und `npm run video` ein Projekt vollständig anlegen lässt und `npm run check` einen vergessenen Platzhalter zuverlässig meldet.

### Phase 5: Qualität
1. Bilder: responsive Größen, `loading="lazy"` außer beim ersten sichtbaren Bild, Alt-Texte Pflicht.
2. Barrierefreiheit: Überschriften-Hierarchie, Tastaturbedienung, `lang`-Attribut, axe ohne Fehler.
3. Performance: HTML, CSS, JavaScript und Schriften der Startseite zusammen unter 150 KB; einziges JavaScript ist das Video-Skript.
4. `public/_headers`: `Cache-Control: public, max-age=31536000, immutable` für `/_astro/*`; für alle Seiten `X-Content-Type-Options: nosniff`, `Referrer-Policy: strict-origin-when-cross-origin`, `X-Frame-Options: DENY` und eine `Permissions-Policy` ohne Kamera, Mikrofon und Standort. Astros eingebaute CSP aktivieren, sofern nichts dadurch kaputtgeht.
5. Abschließende Design-Prüfung aller Seiten gegen die Verbotsliste, mit kurzem Bericht.

**Fertig, wenn:** `npm run check` keine technischen Fehler mehr meldet und Lighthouse in allen Kategorien über 95 liegt.

### Phase 6: Veröffentlichung und Domain
1. **Aufgabe für mich:** Einstellungen, Über-mich-Text, Lebenslauf, Impressum und Datenschutz ausfüllen.
2. **Aufgabe für mich:** Cloudflare-Account, `npx wrangler login`, `npm run deploy`, Seite unter der workers.dev-Adresse prüfen.
3. Domain in Cloudflare hinzufügen; Nameserver umstellen; Domain und `www` unter „Custom Domains“ eintragen; `www` per Redirect Rule umleiten; `kontakt@…` über Cloudflare Email Routing weiterleiten.
4. Prüfen, dass Cloudflare nichts in die Seite einfügt (E-Mail-Verschleierung, Rocket Loader, Web Analytics, Bot Fight Mode aus) und die Live-Seite keine Cookies setzt.

---

## Später möglich (nicht jetzt umsetzen)

- Zweisprachige Seite über Astros i18n-Routing
- Dark Mode mit Iron Grey als Hintergrund
- Kurze Projektnotizen als weitere Collection
- Cookiefreie Besucherstatistik
