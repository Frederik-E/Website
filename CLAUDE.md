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
- `.etikett`: Geist Mono, Versalien, gesperrt, in Muted. `.raster-grund`: Millimeterraster.

**Bausteine:** `Seitenkopf.astro` (Etiketten, große Überschrift), `Startbild.astro` (Einleitung und Zeichen als „Abb. 01“ mit Mittellinien und Messkreis), `ProjectEntry.astro` (Datenblatt auf der Startseite), `Abbildung.astro` (Sucherecken, Maßlinie), `Stueckliste.astro` (Projektübersicht). Nummern der Navigation aus `src/lib/navigation.ts`, Projektnummern `P-01` ab dem ältesten über `projektNummer`.

**Kopf und Fuß:** Kopfzeile klebt oben, halb durchsichtig mit Weichzeichner; links das Zeichen (auf allen Seiten, auch der Startseite), rechts nummerierte Navigation. Fußzeile dunkel in Iron Grey mit Farbschimmer: große E-Mail-Adresse, darunter das Schriftfeld (Name, Seite, Pfad, Stand, Rechtliches). Der Name steht nur dort, im Browser-Tab, im JSON-LD und im Lebenslauf-PDF. Kein Portraitfoto, kein Suchstatus, keine Schaltflächen.

**Nur echte Angaben in der Zeichnung:** Schriftfeld, Datenleiste und Maßlinien zeigen nur, was aus den Daten folgt (Pfad, Baumonat `STAND`, Anzahl der Projekte, Themen, Pixelmaße). Nichts erfinden, auch keine Koordinaten oder Versionen.

**Links:** Chestnut mit Unterstreichung in Dusty Rose nur im Fließtext. Sonst `.ruhig` (Navigation, Titel, Listen) oder `.dezent` (einzelne Verweise). Ganze Einträge sind über einen aufgespannten Titel-Link klickbar.

**Farben:** Palette unverändert, abgeleitete Töne nur per `color-mix` aus Iron Grey und Mist. `--color-konstruktion` nie für Text. Cool Sky: Zeichen, Marker, Messkreis, Sucherecken beim Zeigen, Schimmer. Der Marker (`--color-mark-soft`) steht am aktiven Navigationspunkt, bei `<mark>` und `hr`; `.trenner` gibt es nicht mehr.

**Bewegung:** Linien zeichnen sich beim Scrollen, `.erscheinen` blendet ein (`animation-timeline: view()`, kein JavaScript). Überschriften steigen auf, Messkreis dreht, Schimmer schwebt, Bilder zoomen beim Zeigen. Bei `prefers-reduced-motion` steht alles still.

**Freigegeben (nicht mehr verboten):** Monospace- und Versalien-Beschriftungen, Nummern (01, P-01), Farbverläufe als Schimmer, Millimeterraster, dunkle Fläche, Einblenden beim Scrollen, Effekte beim Zeigen, Cool Sky außerhalb des Markers, das Zeichen in der Kopfzeile der Startseite.

**Weiter verboten:** Karten mit Schatten, Glaseffekt auf Inhaltsflächen, pulsierende Punkte, Status-Badges, Pfeile an Linktexten, Emojis, Hervorhebung einzelner Wörter in Überschriften, Mittelpunkte zwischen Metadaten, Cursor-Effekte, Partikel, Tipp-Animationen, Parallax, „Hi, I'm …“, Kontaktformulare, Cookie-Banner.

**Das Zeichen** sind **zwei organische Formen uebereinander**: eine in Cool Sky, darueber leicht versetzt und gedreht eine deckende in Dusty Rose. Beide quillen und wandern langsam. Kein GIF, kein JavaScript, kein SMIL.

**Warum Dusty Rose:** die einzige Palettenfarbe ohne Textrolle, und warm gegen das kalte Cool Sky. Eine hellere Fassung derselben Farbe sah aus wie ein Schatten und wurde verworfen.

**Kein Mischton im Ueberlapp:** eine Form deckt immer die andere, im Ueberlapp steht nur Rose oder nur Blau. Ein Ueberdruck mit `mix-blend-mode: multiply` (Indigo im Ueberlapp) war umgesetzt und wurde auf Wunsch des Betreibers entfernt. Nicht wieder einfuehren, auch nicht im Favicon.

**Wechsel der Reihenfolge:** In einem Takt von zwei Blob-Runden (40 s) gehen die Formen zweimal schraeg auseinander und kommen mit getauschter Reihenfolge zurueck. Umgeschaltet wird bei 25 % und 75 % des Takts, dort beruehren sie sich nicht. SVG kennt kein `z-index`, deshalb liegt eine zweite blaue Form ganz oben (`.vorn`), die per `step-end` sichtbar wird. Aendert sich Blob, Versatz oder Weg, **nachmessen**: beide Formen einzeln rendern und die Pixel zaehlen, die in beiden liegen. Bei 10 s und 30 s muessen es 0 sein, sonst springt die Farbe im Ueberlapp sichtbar. Ohne Bewegung (reduced motion, Druck) steht Rose vorn.

**Keine Schatten am Zeichen:** Schlagschatten und innerer Schatten wurden ausprobiert und vom Betreiber verworfen.

**Beide Formen benutzen dieselben Keyframes**, die zweite nur mit negativer `animation-delay`. Dadurch stehen sie immer an verschiedenen Stellen ihrer Bewegung, ohne dass zweite Formdaten noetig sind.

**Erzeugt, nicht von Hand geschrieben:** `npm run zeichen` schreibt `src/styles/zeichen.css` und `src/lib/zeichen.ts` aus `scripts/zeichen-erzeugen.mjs`. Beide Dateien nie von Hand aendern. Im Generator steht `MIT_PIKTOGRAMMEN`: damit formt sich der Blob zwischendurch zu Roboter, Chip und Zahnrad. Ausprobiert und verworfen, steht auf `false`.

**Regeln fuer die Formen:**
- Alle Zustaende brauchen **dieselbe Befehlsfolge** (`SEGMENTE` kubische Segmente, geschlossen). Nur dann kann der Browser ueberblenden.
- Der Blob kommt aus der **exakten Ableitung** des Radius nach dem Winkel. Geschaetzte Tangenten, etwa ueber Catmull-Rom, erzeugen sichtbare Knicke.
- Winkelfrequenzen klein halten (1 bis 3), sonst wird die Kontur kantig.
- Mehrere Wellen mit verschiedenen, teils gegenlaeufigen Zeitfrequenzen. Eine einzelne Welle liest sich als Rotation und wirkt mechanisch.

**Nahtlose Schleife, die Regeln dafuer:**
- Der letzte Keyframe (100 %) muss exakt der erste (0 %) sein.
- Der Blob dreht genau **eine** Runde pro Schleife, alle Frequenzen sind ganze Zahlen.
- **Pruefen ueber `getComputedStyle(el).d`** bei `currentTime = 0` und `currentTime = Dauer`, nicht ueber einen Pixelvergleich der Screenshots. Der Pixelvergleich hat hier falsch negativ gemeldet, obwohl die Pfade identisch waren.

**Groesse im Blick behalten:** die Formdaten sind die groesste CSS-Datei. Stellschrauben: `SEGMENTE`, Zahl der Zwischenschritte, Rundung der Zahlen (eine Nachkommastelle reicht).

## Schreibregeln

Ich-Form, kurze Sätze, konkrete Angaben. Keine Floskeln (passionate, innovativ, nahtlos, cutting-edge, leverage, journey, …). Keine Gedankenstriche als Stilmittel, keine rhetorischen Fragen, keine Emojis. Oberflächentexte sagen genau, was passiert. Nichts über die Person erfinden: fehlender Text wird `[TODO: …]`.

## Inhalte

Alles Pflegbare liegt in `inhalt/`, Vorlagen in `vorlagen/`, Code in `src/` und `scripts/`.

- `inhalt/einstellungen.yaml` wird per `?raw` importiert und mit Zod geprüft (`src/lib/einstellungen-schema.ts`). Im Alltag sind `[TODO`-Werte erlaubt, `npm run check` verlangt gültige Werte.
- Projekte: `inhalt/projekte/<ordner>/index.md`, Collection mit `glob()`, URL `/projects/<ordner>/`. Ohne Text unter den Kopfdaten entsteht keine Detailseite.
- Freie Seiten: `inhalt/seiten/<name>.md`, Collection `seiten`, URL `/<name>/` über die Catch-all-Route `src/pages/[...seite].astro`. Mit `navigation: true` erscheinen sie in der Kopfzeile. `src/lib/seiten.ts` wirft einen Fehler, wenn der Dateiname eine vom Code belegte Adresse trifft (about, cv, projects, impressum, datenschutz, ...).
- Die Kopfzeile baut ihre Navigation selbst und markiert den aktuellen Punkt über `Astro.url.pathname`. Es gibt kein `aktiv`-Prop mehr.
- Bilder über `astro:assets` und den `image()`-Helfer im Schema, Videos per `import.meta.glob` mit `?url`.
- Oberflächentexte in `src/i18n.ts` für `en` und `de`, Datum über `Intl.DateTimeFormat`.

## Befehle

`npm run dev` · `npm run new -- "Titel"` · `npm run new-seite -- "Titel"` · `npm run zeichen` · `npm run video -- <datei> <ordner>` · `npm run shots` · `npm run cv` · `npm run check` · `npm run deploy`

## Fallstricke

- **`astro dev` und `astro preview` laufen ab Astro 7 als gemeinsamer Hintergrund-Dienst** mit Sperrdatei (`astro dev stop`, `astro preview stop|status|logs`). Die Skripte benutzen sie gar nicht, sondern bauen immer frisch und starten ihren eigenen kleinen Server (`scripts/lib/statik-server.mjs`), der dist/ so ausliefert wie Cloudflare.
- **Nie einen laufenden Dev-Server fuer Screenshots verwenden.** Sein HMR laesst veraltetes CSS und veraltete Skripte in der Seite stehen. Das hat zweimal zu falschen Schluessen gefuehrt: einmal schien ein `<script>` zu fehlen, einmal schien der Kopf falsch zu sitzen. Beides war nur veralteter Zustand.
- **Bei Layout-Zweifeln messen, nicht aus dem Screenshot ablesen.** Ein kurzes Playwright-Skript mit `getBoundingClientRect` und `scrollWidth` klaert in Sekunden, ob wirklich etwas ueberlaeuft.
- **`backdrop-filter` nur ohne Präfix schreiben.** Steht `-webkit-backdrop-filter` daneben, behält der CSS-Minifier nur die Präfix-Form, und Chromium 151 kennt sie nicht mehr. Der Weichzeichner der Kopfzeile fiel dadurch unbemerkt aus.
- **Ganzseiten-Screenshots zeigen Bilder weit unter dem Sichtbereich manchmal grau**, weil Chromium sie nicht rechtzeitig dekodiert. Vor einem Fehlerbericht hinscrollen und den Sichtbereich fotografieren.
- **`.feld` hat schon eine Animation** (Linie zeichnen). `.erscheinen` auf demselben Element ersetzt sie. Zum Einblenden ein inneres Element nehmen.
- **Etiketten und kleiner Text nie in `--color-konstruktion`**: axe meldet zu wenig Kontrast. Muted verwenden.

## Abweichungen vom Plan (bewusst)

- **Astro 7.3.2** statt 6: aktuelle stabile Version. Markdown läuft über Satteri statt remark/rehype, der Compiler ist strenger bei unsauberem HTML, `compressHTML` steht auf `'jsx'` (Leerraum zwischen Inline-Elementen wird entfernt, deshalb Linkreihen als Flex-Container mit `gap`).
- **Geist statt IBM Plex:** mit dem Redesign vom 2026-09-22. Kursiv steht als eigener Fonts-Eintrag (`--font-sans-kursiv`), sonst lädt `<Font preload />` alle Schnitte vor. Vorgeladen werden Geist 400 und 500 und Geist Mono 400, zusammen 36 KB.
- **Fußzeile dunkel:** `PLAN.md` sah ursprünglich einen dunklen Footer vor, nach der Rückmeldung „minimalistischer“ war er eine Weile hell. Mit dem Konzept „Technische Zeichnung“ ist er wieder dunkel, mit Schriftfeld.
- **Zod aus `astro/zod`, nicht aus `astro:content`:** das `z` aus `astro:content` ist in Astro 7 als veraltet markiert.
- **Keine Syntaxfaerbung fuer Code** (`markdown.syntaxHighlight: false`): Shiki bringt sonst eine dunkle Flaeche und fremde Farben in die Seite.
- **Playwright ist auf `~1.62.1` fixiert**, weil der passende Chromium bereits lokal liegt. Ein Wechsel auf 1.63 lädt rund 180 MB nach.
