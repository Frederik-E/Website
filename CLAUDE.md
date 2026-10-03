# Arbeitsregeln für dieses Projekt

Vollständige Vorgaben: `PLAN.md`. Diese Datei ist die Kurzfassung.
Anleitungen für den Betreiber der Seite: `ANLEITUNG.md` (Übersicht), `ANLEITUNG-START.md`, `ANLEITUNG-INHALTE.md`, `ANLEITUNG-VEROEFFENTLICHEN.md`.

## Arbeitsweise

- Phase für Phase aus `PLAN.md`. Nach jeder Phase anhalten, Ergebnis zeigen, auf Freigabe warten. Erst nach Freigabe committen.
- Bei allem rund um Astro gilt die Doku der installierten Version (Astro 7). Widerspricht `PLAN.md` der Doku, gilt die Doku, und die Abweichung wird genannt.
- Nach jeder Änderung am Aussehen: `npm run shots`, Screenshots in `.screenshots/` ansehen, gegen Design-Regeln und Verbotsliste prüfen.
- Schritte, die der Betreiber selbst erledigen muss, als **Aufgabe für mich** markieren.
- Vor großen Downloads fragen.

## Design in Kürze

Konzept **„Technische Zeichnung“**, freigegeben am 2026-09-22 und vom Betreiber ausdrücklich als gelungen bezeichnet. Es ist der Maßstab für alle weiteren Änderungen. Zwei zurückhaltende Vorgänger („Arbeitsmappe“, danach eine noch ruhigere Fassung) sahen laut Betreiber aus „wie nach 20 Minuten selbst gecodet“. Er will sichtbare grafische Elemente, nicht Minimalismus. Vollständig in `PLAN.md`, Abschnitt „Design“.

**Blatt und Felder** (Klassen in `src/styles/global.css`):
- `.blatt`: zwei Randlinien über die ganze Seite, höchstens `76rem`. Kopfzeile, `main` und Fußzeile benutzen es.
- `.feld`: ein Abschnitt, oben Haarlinie mit Kreuzmarken an beiden Rändern. `.knapp` für schmale Zeilen, `.ohne-linie` direkt unter der Kopfzeile, `.geteilt` zeichnet ab `60rem` die Linie zwischen linker und rechter Spalte.
- Raster: links ein Drittel (Nummer, Einordnung), rechts zwei Drittel (Inhalt). `.zweispaltig`, `.gegliedert` (Zwischenüberschrift links, automatisch nummeriert).
- `.zellen`: Schriftfeld-Zellen mit eigenen Linien. `--erste-spalte` lässt die erste Linie mit der Teilungslinie fluchten.
- `.etikett`: Barlow Semi Condensed, Versalien, gesperrt, in Muted. `.raster-grund`: Millimeterraster.

**Bausteine:** `Seitenkopf.astro` (Etiketten, große Überschrift), `Startbild.astro` (Einleitung und die große Punktmatrix als „Abb. 01“), `ProjectEntry.astro` (Datenblatt auf der Startseite), `Abbildung.astro` (Sucherecken, Maßlinie), `Stueckliste.astro` (Projektübersicht). Nummern der Navigation aus `src/lib/navigation.ts`, Projektnummern `P-01` ab dem ältesten über `projektNummer`.

**Kopf und Fuß:** Kopfzeile klebt oben, halb durchsichtig mit Weichzeichner; links das Zeichen (auf allen Seiten, auch der Startseite), rechts nummerierte Navigation. Fußzeile dunkel in Iron Grey mit Farbschimmer: große E-Mail-Adresse, darunter das Schriftfeld (Name, Seite, Pfad, Stand, Rechtliches). Der Name steht nur dort, im Browser-Tab, im JSON-LD und im Lebenslauf-PDF; die Initialen erscheinen nur in der Animation des Zeichens im Startbild. Kein Portraitfoto, kein Suchstatus, keine Schaltflächen.

**Nur echte Angaben in der Zeichnung:** Schriftfeld, Datenleiste und Maßlinien zeigen nur, was aus den Daten folgt (Pfad, Baumonat `STAND`, Anzahl der Projekte, Themen, Pixelmaße). Nichts erfinden, auch keine Koordinaten oder Versionen.

**Links:** Chestnut mit Unterstreichung in Dusty Rose nur im Fließtext. Sonst `.ruhig` (Navigation, Titel, Listen) oder `.dezent` (einzelne Verweise). Ganze Einträge sind über einen aufgespannten Titel-Link klickbar.

**Farben:** Palette unverändert, abgeleitete Töne nur per `color-mix` aus Iron Grey und Mist. `--color-konstruktion` nie für Text. Cool Sky: Zeichen, Marker, Sucherecken beim Zeigen, Schimmer. Der Marker (`--color-mark-soft`) steht am aktiven Navigationspunkt, bei `<mark>` und `hr`; `.trenner` gibt es nicht mehr.

**Bewegung:** Linien zeichnen sich beim Scrollen, `.erscheinen` blendet ein (`animation-timeline: view()`, kein JavaScript). Überschriften steigen auf, Ringe laufen von zufälligen Punkten des Zeichens nach außen, Schimmer schwebt, Bilder zoomen beim Zeigen. Bei `prefers-reduced-motion` steht alles still.

**Freigegeben (nicht mehr verboten):** Monospace- und Versalien-Beschriftungen, Nummern (01, P-01), Farbverläufe als Schimmer, Millimeterraster, dunkle Fläche, Einblenden beim Scrollen, Effekte beim Zeigen, Cool Sky außerhalb des Markers, das Zeichen in der Kopfzeile der Startseite.

**Weiter verboten:** Karten mit Schatten, Glaseffekt auf Inhaltsflächen, pulsierende Punkte, Status-Badges, Pfeile an Linktexten, Emojis, Hervorhebung einzelner Wörter in Überschriften, Mittelpunkte zwischen Metadaten, Cursor-Effekte, Partikel, Tipp-Animationen, Parallax, „Hi, I'm …“, Kontaktformulare, Cookie-Banner.

**Das Zeichen: Punktmatrix** (seit 2026-09-22). Ein Raster aus Punkten. Von einem Startpunkt laufen Ringe nach außen, abwechselnd Cool Sky und Dusty Rose, alle 2,5 s einer, wie Wellen um einen Tropfen. **Jeder Ring startet an einem zufällig gewählten Punkt der Matrix**; der aktuelle Startpunkt ist in Iron Grey markiert und pulsiert kurz, wenn ein Ring losläuft. So hat es der Betreiber gewünscht, nacheinander verworfen: zwei Wellen aus zwei festen Ecken, dann eine feste Quelle in der Mitte. Groß als „Abb. 01“ im Startbild (9 × 9), klein in der Kopfzeile jeder Seite (4 × 4), als Standbild im Favicon.

**Reiner Blickfang ohne Bedeutung.** Der Betreiber will ausdrücklich kein Motiv mit Inhalt, „es soll einfach existieren“ wie früher der Blob, nur technisch statt organisch. In drei Runden verworfen: projektbezogene Animationen (Rundkurs, LiDAR-Scan, Sprungantwort, Roboterarm, Lissajous), übergeordnete mit Bedeutung (Bezugssystem, Kreis und Quadrat mit Projektionslinien, Explosionszeichnung, Zustandsschätzung, Fourier-Reihe) und ein neuronales Netz. Aus der letzten Runde fand er außer der Punktmatrix auch „Würfel“ (zwei 3D-Würfel, die umeinander kreisen) und „Schraffur“ (Moiré zweier schraffierter Formen) gut. Nicht wieder Motive mit Bedeutung vorschlagen.

**Technik:** `src/lib/zeichen.ts` zieht beim Bauen eine zufällige Folge von Startpunkten (32 Ringe groß, 12 klein; bei jedem Bau eine neue) und legt fest, welche Punkte zu welchem Buchstaben gehören. `src/components/Zeichen.astro` schreibt die Folge als Keyframes in ein `<style>` im SVG: sie schalten mit `steps(1, end)` registrierte Variablen (`@property`) um: `--qx`, `--qy` (Startpunkt), `--blau` (Farbe des Rings), `--vorher`, `--nachher` (Buchstabe vor und hinter der Front), `--daempfung`. `--t` läuft je Ring von 0 bis 1. Jeder Punkt rechnet im CSS mit `hypot()` seinen Abstand zum Startpunkt, daraus mit `clamp()` und `min()`/`max()` sein Leuchten. Kein JavaScript, kein SMIL. Browser ohne `hypot()` zeigen nur die grauen Punkte (`@supports`). Unter 120 px Größe gilt die kleine Fassung. Die Stile stehen mit `is:global` und dem Präfix `.zeichen-bild`: gekapselt hängt Astro an jeden der gut 300 Kreise ein Attribut, das allein rund 8 KB kostet und die Startseite über das Budget von 150 KB hebt.

**Raster** (nur große Fassung): Im Startbild zeichnet das Zeichen sein Millimeterraster selbst (`mitRaster`, halber Punktabstand), damit jeder Punkt genau auf einer Kreuzung liegt. Ein Hintergrundraster am Feld traf die Punkte nie, weil Feld und Abbildung verschieden verankert sind. Das Raster reicht weit über das SVG hinaus; `.innen` im Startbild beschneidet es mit `overflow: clip`, die Kreuzmarken des Felds liegen außerhalb und bleiben ganz.

**Initialen** (nur große Fassung, Wunsch des Betreibers): Alle 40 s (das erste Mal nach 7,5 s; zweimal in der 80-s-Folge) zeichnen die Ringe die Initialen aus `initialen` in `inhalt/einstellungen.yaml` (derzeit „FE“). Die Ringfront ist dabei die Übergangslinie: vor ihr gilt der alte Zustand, hinter ihr der neue. Folge bei zwei Buchstaben: ein Ring lässt F entstehen, einer hält es, einer verwandelt F in E, einer hält E, einer wischt es weg (`vorher`/`nachher` je Ring). Erster Buchstabe Cool Sky, zweiter Dusty Rose. Solange Buchstaben stehen, leuchten die Ringe gedämpft. Buchstaben kommen aus einer 5 × 7-Pixelschrift (`SCHRIFT` in `src/lib/zeichen.ts`, A bis Z). Vorher liefen an dieser Stelle Formen (Kreis, Quadrat, Raute, zwei Kreise); der Betreiber wollte stattdessen die Initialen, und zwar alle 40 s statt alle 80 s.

**Standbild** (reduzierte Bewegung, Druck): Die Animation fällt weg, es gelten die Werte im `style` des SVG: ein blauer Ring um die Mitte. Das Favicon (`src/pages/favicon.svg.ts`) ist ein eigenes Standbild der kleinen Fassung: dunkler Startpunkt, blauer Ring, rosa Ring.

**Prüfen:** Startpunkt und Leuchten per Playwright auslesen (`getComputedStyle(svg).getPropertyValue('--qx')`, Opacity der `.welle`-Kreise). Screenshots hinken dabei nach, weil sie Zeit brauchen; für Zeitpunkte die ausgelesenen Werte nehmen, nicht die Bilder.

**Kein Mischton:** Rosa liegt über Blau, wo beide zugleich leuchten. Ein Überdruck mit `mix-blend-mode: multiply` war früher umgesetzt und wurde auf Wunsch des Betreibers entfernt. Nicht wieder einführen, auch nicht im Favicon.

**Warum Dusty Rose:** die einzige Palettenfarbe ohne Textrolle, und warm gegen das kalte Cool Sky. Eine hellere Fassung derselben Farbe sah aus wie ein Schatten und wurde verworfen.

**Keine Schatten am Zeichen:** Schlagschatten und innerer Schatten wurden beim früheren Zeichen ausprobiert und vom Betreiber verworfen.

**Das frühere organische Zeichen** (zwei Blobs, die quellen und die Reihenfolge tauschen) samt Generator `scripts/zeichen-erzeugen.mjs` und `npm run zeichen` liegt im Commit `559fdb9`.

## Schreibregeln

Ich-Form, kurze Sätze, konkrete Angaben. Keine Floskeln (passionate, innovativ, nahtlos, cutting-edge, leverage, journey, …). Keine Gedankenstriche als Stilmittel, keine rhetorischen Fragen, keine Emojis. Oberflächentexte sagen genau, was passiert. Nichts über die Person erfinden: fehlender Text wird `[TODO: …]`.

## Inhalte

Alles Pflegbare liegt in `inhalt/`, Vorlagen in `vorlagen/`, Code in `src/` und `scripts/`.

- `inhalt/einstellungen.yaml` wird per `?raw` importiert und mit Zod geprüft (`src/lib/einstellungen-schema.ts`). Im Alltag sind `[TODO`-Werte erlaubt, `npm run check` verlangt gültige Werte.
- Projekte: `inhalt/projekte/<ordner>/index.md`, Collection mit `glob()`, URL `/projects/<ordner>/`. Ohne Text unter den Kopfdaten entsteht keine Detailseite.
- Freie Seiten: `inhalt/seiten/<name>.md`, Collection `seiten`, URL `/<name>/` über die Catch-all-Route `src/pages/[...seite].astro`. Mit `navigation: true` erscheinen sie in der Kopfzeile. `src/lib/seiten.ts` wirft einen Fehler, wenn der Dateiname eine vom Code belegte Adresse trifft (about, cv, projects, impressum, datenschutz, ...).
- Die Kopfzeile baut ihre Navigation selbst und markiert den aktuellen Punkt über `Astro.url.pathname`. Es gibt kein `aktiv`-Prop mehr.
- Bilder über `astro:assets` und den `image()`-Helfer im Schema, Videos per `import.meta.glob` mit `?url`.
- **Bilder behalten ihr eigenes Seitenverhältnis** (`img.medium`), nur Videos stehen im festen 16:9-Kasten. Technische Zeichnungen sind flach und wären sonst beschnitten. Gegen breite einfarbige Ränder gibt es `npm run bild -- <datei>`: es schneidet sie weg und legt 2 % der Breite als Rand wieder an. Das Bild wird dabei ersetzt.
- **Quelltexte** liegen in `public/code/<projekt>/` und werden aus dem Projekttext verlinkt. `public/_headers` und `scripts/lib/statik-server.mjs` geben ihnen `text/plain`, sonst laedt der Browser sie herunter statt sie anzuzeigen.
- Oberflächentexte in `src/i18n.ts` für `en` und `de`, Datum über `Intl.DateTimeFormat`.

## Befehle

`npm run dev` · `npm run new -- "Titel"` · `npm run new-seite -- "Titel"` · `npm run video -- <datei> <ordner>` · `npm run bild -- <datei>` · `npm run shots` · `npm run cv` · `npm run check` · `npm run deploy`

## Fallstricke

- **`astro dev` und `astro preview` laufen ab Astro 7 als gemeinsamer Hintergrund-Dienst** mit Sperrdatei (`astro dev stop`, `astro preview stop|status|logs`). Die Skripte benutzen sie gar nicht, sondern bauen immer frisch und starten ihren eigenen kleinen Server (`scripts/lib/statik-server.mjs`), der dist/ so ausliefert wie Cloudflare.
- **Nie einen laufenden Dev-Server fuer Screenshots verwenden.** Sein HMR laesst veraltetes CSS und veraltete Skripte in der Seite stehen. Das hat zweimal zu falschen Schluessen gefuehrt: einmal schien ein `<script>` zu fehlen, einmal schien der Kopf falsch zu sitzen. Beides war nur veralteter Zustand.
- **Der Dev-Server haelt Inhalte im Zwischenspeicher.** Wird ein Bild in `inhalt/` geloescht oder ersetzt, bricht er mit `ImageNotFound` ab und liefert gar nichts mehr, auch nach dem Neuladen. Dann: `npx astro dev stop`, aus `.astro/` die Dateien `content-assets.mjs`, `content-modules.mjs`, `data-store.json` und den Ordner `collections` loeschen, neu starten. `.astro/fonts` stehen lassen, sonst werden die Schriften neu geladen.
- **Bei Layout-Zweifeln messen, nicht aus dem Screenshot ablesen.** Ein kurzes Playwright-Skript mit `getBoundingClientRect` und `scrollWidth` klaert in Sekunden, ob wirklich etwas ueberlaeuft.
- **`backdrop-filter` nur ohne Präfix schreiben.** Steht `-webkit-backdrop-filter` daneben, behält der CSS-Minifier nur die Präfix-Form, und Chromium 151 kennt sie nicht mehr. Der Weichzeichner der Kopfzeile fiel dadurch unbemerkt aus.
- **Ganzseiten-Screenshots zeigen Bilder weit unter dem Sichtbereich manchmal grau**, weil Chromium sie nicht rechtzeitig dekodiert. Vor einem Fehlerbericht hinscrollen und den Sichtbereich fotografieren.
- **`.feld` hat schon eine Animation** (Linie zeichnen). `.erscheinen` auf demselben Element ersetzt sie. Zum Einblenden ein inneres Element nehmen.
- **Etiketten und kleiner Text nie in `--color-konstruktion`**: axe meldet zu wenig Kontrast. Muted verwenden.

## Abweichungen vom Plan (bewusst)

- **Astro 7.3.2** statt 6: aktuelle stabile Version. Markdown läuft über Satteri statt remark/rehype, der Compiler ist strenger bei unsauberem HTML, `compressHTML` steht auf `'jsx'` (Leerraum zwischen Inline-Elementen wird entfernt, deshalb Linkreihen als Flex-Container mit `gap`).
- **Barlow statt IBM Plex:** Barlow (an DIN angelehnt) für Text, Barlow Semi Condensed für Etiketten, IBM Plex Mono nur für Code. Geist war zwischendurch eingesetzt; der Betreiber fand dessen Ziffern komisch (schmale 1 mit langem Anstrich). Kursiv steht als eigener Fonts-Eintrag (`--font-sans-kursiv`), sonst lädt `<Font preload />` alle Schnitte vor. Barlow ist schmal gezeichnet und verträgt nur wenig Verengung, deshalb Laufweiten als Tokens (`--laufweite-*`).
- **Fußzeile dunkel:** `PLAN.md` sah ursprünglich einen dunklen Footer vor, nach der Rückmeldung „minimalistischer“ war er eine Weile hell. Mit dem Konzept „Technische Zeichnung“ ist er wieder dunkel, mit Schriftfeld.
- **Zod aus `astro/zod`, nicht aus `astro:content`:** das `z` aus `astro:content` ist in Astro 7 als veraltet markiert.
- **Keine Syntaxfaerbung fuer Code** (`markdown.syntaxHighlight: false`): Shiki bringt sonst eine dunkle Flaeche und fremde Farben in die Seite.
- **Playwright ist auf `~1.62.1` fixiert**, weil der passende Chromium bereits lokal liegt. Ein Wechsel auf 1.63 lädt rund 180 MB nach.
