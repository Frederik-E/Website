# Inhalte schreiben

Alles, was du an der Seite pflegst, liegt im Ordner `inhalt/`. Du brauchst dafür keinen Code anzufassen.

Nach jeder Änderung speichern. Läuft `npm run dev`, siehst du das Ergebnis sofort im Browser.

---

## Wo liegt was

```
inhalt/
  einstellungen.yaml     Name, Links, Domain, Untertitel und Einleitung in beiden Sprachen
  lebenslauf.yaml        Profil und Lebenslauf: auf /cv/, Kurzform auf der Startseite
  lebenslauf.en.yaml     Dasselbe auf Englisch, für /en/
  foto/                  Dein Foto, ein Bild
  lebenslauf-pdf/        Optional: dein eigenes PDF (englisch mit .en.pdf am Ende)
  impressum.md           (und impressum.en.md)
  datenschutz.md         (und datenschutz.en.md)
  projekte/
    mein-projekt/
      index.md           Kopfdaten und Text
      index.en.md        Die englischen Texte
      titelbild.jpg
      demo.mp4
  seiten/
    meine-seite.md       Wird zu /meine-seite/
    meine-seite.en.md    Wird zu /en/meine-seite/
vorlagen/                Vorlagen, von den Skripten benutzt
```

Die Seite gibt es zweimal: auf Deutsch unter `/` und auf Englisch unter `/en/`. Zu fast jeder Datei gehört deshalb eine englische Fassung mit `.en` im Namen. Mehr dazu unter [Die englische Fassung](#die-englische-fassung).

---

## Die Startseite

Die Startseite ist wie ein technisches Zeichnungsblatt aufgebaut. Oben stehen dein Foto und die große Zeile, darunter dein Lebenslauf in Kurzform, danach die Projekte. Alles steuerst du über `inhalt/`.

**Oben das Startbild.** Deine Einleitung groß links, rechts das bewegte Zeichen: eine Punktmatrix, über die farbige Ringe laufen, jeder von einem zufälligen Punkt aus. Alle 40 Sekunden zeichnen die Ringe deine Initialen, einen Buchstaben nach dem anderen. Welche Buchstaben das sind, steht in `inhalt/einstellungen.yaml` bei `initialen`; leer lassen blendet sie aus.

- Die **Einleitung** steht in `inhalt/einstellungen.yaml` bei `einleitung`. Eine kurze Zeile, woran du arbeitest, nicht wer du bist. Leer lassen (`einleitung: ""`) blendet sie aus.

Links neben der Einleitung steht klein dein Foto aus `inhalt/foto/`. Dein Profiltext steht nicht auf der Startseite, nur im Lebenslauf.

**Darunter der Lebenslauf in Kurzform.** Jeder Abschnitt aus `inhalt/lebenslauf.yaml` mit `startseite: true`, mit Titel, Organisation und Zeitraum. Die Stichpunkte stehen nur auf `/cv/`. Oben rechts zwei Verweise: der ganze Lebenslauf und das PDF.

**Danach die ausgewählten Arbeiten.** Oben eine Leiste mit der Anzahl deiner Projekte, dem Datum des neuesten und allen Themen aus den Kopfdaten. Sie füllt sich von selbst. Jedes Projekt mit `startseite: true` erscheint als Datenblatt: links Titel, Datum, Rolle und Themen, rechts der Satz aus `kurz` und das Titelbild.

- Ein Projekt kann `stichpunkte` haben, zum Beispiel die nötigen Skills. Sie stehen rechts als Liste über dem Titelbild, jede Zeile mit einer kleinen Kreuzmarke. Sind es mehr als drei kurze Punkte (höchstens 24 Zeichen), stehen sie in Spalten. Mit Stichpunkten darfst du `kurz` weglassen.

- Die Anordnung bestimmst du mit `reihenfolge`. Ist kein Projekt markiert, stehen dort die drei neuesten.
- Drei bis vier Projekte sind ein gutes Maß. Mit Titelbild wirken sie am stärksten.

**Ganz unten deine eigenen Seiten.** Jede Seite aus `inhalt/seiten/` mit ihrer `beschreibung`. Gibt es keine, fällt der Teil weg.

---

## Ein Projekt anlegen

1. `npm run new -- "Titel des Projekts"` ausführen.
2. Die angelegte Datei `inhalt/projekte/<ordner>/index.md` öffnen.
3. Die Kopfdaten oben ausfüllen und alle `[TODO: ...]` ersetzen.
4. Darunter die vier Abschnitte schreiben.
5. Wenn es fertig ist, `entwurf: true` auf `entwurf: false` ändern.

Das Skript legt daneben `index.en.md` an. Dort trägst du die englischen Texte ein, siehe [Die englische Fassung](#die-englische-fassung).

### Die Kopfdaten

| Feld | Pflicht | Bedeutung |
|---|---|---|
| `titel` | ja | Name des Projekts |
| `kurz` | ja, außer es gibt `stichpunkte` | Ein bis zwei Sätze für die Übersicht, höchstens 200 Zeichen |
| `datum` | ja | Format `JJJJ-MM`, bestimmt die Reihenfolge |
| `bis` | nein | Format `JJJJ-MM`, Ende eines Zeitraums. Dann steht auf der Seite „Mai 2026 – Juli 2026“ |
| `rolle` | nein | „Allein" oder „Perception im Viererteam" |
| `tags` | nein | `[ROS 2, SLAM, Python]` |
| `stichpunkte` | nein | Liste, steht im Eintrag über dem Titelbild. Jede Zeile mit `- ` beginnen |
| `titelbild` | nur mit Video | `./titelbild.jpg` |
| `titelbild_alt` | nur mit Bild | Was auf dem Bild zu sehen ist |
| `video` | nein | `demo.mp4` |
| `code` | nein | Link zum Repository |
| `demo` | nein | Link zu Demo, Video oder Paper |
| `startseite` | nein | `true` zeigt das Projekt auf der Startseite |
| `reihenfolge` | nein | Zahl, kleinere stehen weiter oben |
| `entwurf` | nein | `true` blendet es öffentlich aus |

### Was in ein Projekt gehört

Die Vorlage gibt vier Abschnitte vor. Halte dich daran, sie beantworten genau das, was jemand wissen will:

**Problem.** Was war die Aufgabe, und warum war sie nicht trivial? Ein bis drei Sätze. Am besten mit dem konkreten Punkt, an dem die erste Lösung gescheitert ist.

**Ansatz.** Was hast du gebaut, mit welchen Werkzeugen, und warum so? War es Teamarbeit, schreib dazu, was **dein** Teil war.

**Ergebnis.** Was kam heraus? Mit Zahl, wenn es eine gibt: Genauigkeit, Laufzeit, Fehlerrate, Anzahl der Durchläufe. „Funktioniert gut" sagt nichts, „4 cm mittlerer Querfehler bei 2 m/s" sagt alles.

**Was ich gelernt habe.** Ein bis drei Sätze. Was würdest du beim nächsten Mal anders machen?

Auf der Unterseite stehen die Zwischenüberschriften links neben ihrem Text.

Schreib in der Ich-Form und in kurzen Sätzen. Keine Floskeln wie „innovativ", „leidenschaftlich" oder „nahtlos".

---

## Ein Projekt ohne eigene Unterseite

Für kleine Sachen, die nur einen Satz und einen Link brauchen.

1. `npm run new -- "Titel des Projekts"` ausführen.
2. Die Datei öffnen und **alles unterhalb der zweiten `---`-Zeile löschen**.
3. `titel`, `kurz` und `datum` ausfüllen.
4. Bei `# code: https://github.com/...` das `#` und das Leerzeichen davor entfernen und deinen Link eintragen.
5. `entwurf: false` setzen.

Der Eintrag erscheint dann in der Übersicht und verlinkt direkt auf das Repository. Es entsteht keine eigene Unterseite.

---

## Ein Bild zu einem Projekt

1. Das Bild in den Projektordner legen, zum Beispiel `inhalt/projekte/mein-projekt/titelbild.jpg`.
2. In `index.md` bei `# titelbild: ./titelbild.jpg` das `#` entfernen.
3. Auch bei `# titelbild_alt:` das `#` entfernen und beschreiben, was zu sehen ist.
4. Speichern.

Das Titelbild erscheint in der Übersicht als kleine Vorschau, auf der Startseite und auf der Unterseite groß. Es behält sein eigenes Seitenverhältnis, wird also nicht beschnitten. Hat deine Zeichnung einen breiten einfarbigen Rand, schneide ihn mit `npm run bild -- inhalt/projekte/mein-projekt/titelbild.png` weg. Das ersetzt die Datei, hebe sie vorher auf, wenn du sie noch brauchst. Um die Größe musst du dich nicht kümmern, die Seite erzeugt passende Fassungen selbst.

Weitere Bilder mitten im Text: Datei daneben legen und einbinden mit
`![Was zu sehen ist](./aufbau.png)`

Die Bildbeschreibung ist keine Formsache. Sie wird vorgelesen, wenn jemand die Seite nicht sehen kann, und sie steht da, wenn ein Bild nicht lädt.

---

## Ein Video zu einem Projekt

Ein Video pro Projekt. Du brauchst dafür einmalig das Programm ffmpeg.

1. `npm run video -- pfad/zu/deinem/video.mp4 mein-projekt` ausführen.
2. Das Skript verkleinert das Video und legt es als `demo.mp4` im Projektordner ab.
3. Fehlt ein Titelbild, wird der erste Frame genommen.
4. In `index.md` das Feld `titelbild_alt` ausfüllen.
5. Speichern und nachsehen.

ffmpeg installieren, falls es fehlt:

- Windows: `winget install Gyan.FFmpeg`, danach das Terminal neu öffnen
- macOS: `brew install ffmpeg`
- Linux: `sudo apt install ffmpeg`

Das Video läuft nur auf der Unterseite des Projekts, stumm und in Schleife, mit Steuerung zum Anhalten. In der Übersicht steht das Titelbild, damit die Seite schnell lädt.

Zehn Sekunden Video reichen fast immer. Über 25 MB nimmt Cloudflare die Datei nicht an, das Skript bricht dann ab.

---

## Reihenfolge, Startseite, verstecken, löschen

**Auf der Startseite zeigen:** `startseite: true` in den Kopfdaten. Ist bei keinem Projekt `startseite: true` gesetzt, erscheinen die drei neuesten.

**Reihenfolge ändern:** `reihenfolge: 1`, `reihenfolge: 2` und so weiter. Kleinere Zahlen stehen weiter oben. Ohne `reihenfolge` sortiert das Datum, neueste zuerst.

**Verstecken:** `entwurf: true`. Das Projekt bleibt in der lokalen Vorschau sichtbar und trägt dort den Hinweis „Entwurf". Auf der veröffentlichten Seite fehlt es.

**Löschen:** den ganzen Ordner löschen.

---

## Eine neue Seite anlegen

Für alles, was kein Projekt ist: Notizen, Lehre, Vorträge, ein Verzeichnis, was auch immer.

1. `npm run new-seite -- "Titel der Seite"` ausführen.
2. Die angelegte Datei `inhalt/seiten/<name>.md` öffnen.
3. Die `[TODO: ...]` ersetzen und den Text schreiben.
4. Soll die Seite in die Kopfzeile, `navigation: true` setzen.
5. Wenn sie fertig ist, `entwurf: false` setzen.

Die Adresse ergibt sich aus dem Dateinamen: `inhalt/seiten/vortraege.md` wird zu `/vortraege/`.

| Feld | Bedeutung |
|---|---|
| `titel` | Überschrift der Seite, zugleich der Text in der Kopfzeile |
| `beschreibung` | Ein Satz, der bei Google unter dem Titel steht |
| `navigation` | `true` stellt die Seite in die Kopfzeile |
| `reihenfolge` | Zahl, kleinere stehen in der Kopfzeile weiter links |
| `noindex` | `true` hält die Seite aus Suchmaschinen heraus |
| `entwurf` | `true` zeigt sie nur in der lokalen Vorschau |

Bilder für eine Seite legst du nach `inhalt/seiten/` und bindest sie mit `![Was zu sehen ist](./mein-bild.jpg)` ein.

Das Skript legt daneben `<name>.en.md` an. Dort stehen Titel, Beschreibung und Text auf Englisch; die Seite erscheint dann unter `/en/<name>/`.

Ein paar Adressen sind vom Code belegt: `cv`, `projects`, `impressum`, `datenschutz`, `en`. Das Skript sagt dir das, falls du eine davon erwischst.

---

## Dein Foto

1. Ein Bild in `inhalt/foto/` legen, als JPG, PNG oder WebP. Der Dateiname ist egal.
2. Speichern, fertig.

Das Foto steht klein auf der Startseite links neben der großen Zeile und im Lebenslauf links neben der Überschrift. Im PDF steht es oben rechts neben deinem Namen. Gezeigt wird ein Ausschnitt im Hochformat 4:5, mittig, das Gesicht im oberen Drittel. 800 Pixel Breite reichen. HEIC-Fotos vom iPhone vorher als JPG exportieren.

Leg nur ein Bild in den Ordner. Liegen dort mehrere, nimmt die Seite das erste nach dem Alphabet. Unterordner beachtet sie nicht.

Dein jetziges Foto ist `portrait.jpg`: das Original mit aufgehelltem, kühlem Hintergrund, damit es zum Weiß der Seite passt. Das Original liegt in `inhalt/foto/original/`. Bei einem neuen Foto vor einfarbigem Hintergrund erzeugt `python3 scripts/foto-aufhellen.py inhalt/foto/original/<datei> inhalt/foto/portrait.jpg` die aufgehellte Fassung (braucht Python mit numpy, scipy und opencv). Solange kein Foto da ist, steht an beiden Stellen ein Platzhalter, und die Seite wird nicht veröffentlicht.

---

## Profil und Lebenslauf

Beides steht in `inhalt/lebenslauf.yaml`. Eine eigene Seite „Über mich“ gibt es nicht mehr.

**Das Profil** steht ganz oben in der Datei bei `profil`: 60 bis 120 Wörter in der Ich-Form. Es erscheint im Lebenslauf als erster Abschnitt, nicht auf der Startseite. Eine Leerzeile beginnt einen neuen Absatz.

```yaml
profil: |
  Erster Absatz.

  Zweiter Absatz.
```

Ein Aufbau, der funktioniert: erster Satz Studium und Schwerpunkt, zweiter bis vierter Satz ein konkretes Projekt mit deiner Rolle und dem Ergebnis, letzter Satz was du als nächstes suchst.

**Der Lebenslauf:**

1. `inhalt/lebenslauf.yaml` öffnen.
2. Abschnitte und Einträge ändern. Die Reihenfolge in der Datei ist die Reihenfolge auf der Seite. `startseite: true` zeigt einen Abschnitt zusätzlich in Kurzform auf der Startseite.
3. Zeiten im Format `JJJJ-MM`, zum Beispiel `von: 2025-10`. `bis:` leer lassen bedeutet „heute".
4. Speichern.
5. `npm run cv` ausführen, damit das PDF neu entsteht.

Es gibt zwei Arten von Einträgen:

```yaml
# Station
- titel: M.Sc. Robotics
  organisation: TU Muenchen
  ort: Muenchen
  von: 2025-10
  bis:
  punkte:
    - Schwerpunkt Perception

# Liste
- bereich: Robotics
  liste: [ROS 2, Gazebo, Nav2]
```

Keine Adresse, keine Telefonnummer, kein Geburtsdatum. Das gehört nicht auf eine öffentliche Seite.

**Der englische Lebenslauf** steht in `inhalt/lebenslauf.en.yaml`, mit demselben Aufbau: dieselben Abschnitte und Einträge in derselben Reihenfolge, mit denselben Zeitangaben. Änderst du etwas in `lebenslauf.yaml`, ändere es dort auch. `npm run check` meldet, wenn die beiden Dateien auseinanderlaufen.

`npm run cv` erzeugt beide PDFs: `/cv.pdf` aus `/cv/` und `/en/cv.pdf` aus `/en/cv/`.

**Eigenes PDF verwenden:** Leg dein PDF in den Ordner `inhalt/lebenslauf-pdf/`, der Name ist egal (z. B. `Lebenslauf.pdf`). Die englische Fassung bekommt `.en.pdf` am Ende (z. B. `Lebenslauf.en.pdf`). Je Sprache nur ein PDF. Dann `npm run cv` ausführen. Auf der deutschen Seite lädt man ab dann dein deutsches PDF herunter, auf der englischen dein englisches. Fehlt eine Sprache, wird das PDF dafür weiter aus der Seite erzeugt; `npm run check` weist darauf hin. Löschst du die Dateien wieder, wird wieder aus der Seite erzeugt.

**Englische Fassung deines PDFs:** `python3 scripts/lebenslauf-englisch.py` übersetzt dein deutsches PDF und legt es als `<name>.en.pdf` daneben. Nur der Text ändert sich, das Design bleibt. Die Übersetzungen stehen als Tabelle oben im Skript. Änderst du deinen deutschen Lebenslauf, müssen sie angepasst werden; das Skript sagt dir, welche Zeilen neu sind. Am einfachsten bittest du Claude darum.

---

## Die englische Fassung

Die Seite gibt es auf Deutsch unter `/` und auf Englisch unter `/en/`, sonst sind die Adressen gleich: `/cv/` und `/en/cv/`, `/projects/` und `/en/projects/`. Rechts in der Kopfzeile wechselt ein Link („English“ bzw. „Deutsch“) zur selben Seite in der anderen Sprache. Die Beschriftungen und Datumsangaben übersetzt die Seite selbst. Deine Texte stehen in eigenen Dateien mit `.en` im Namen:

| Deutsch | Englisch | Was dort steht |
|---|---|---|
| `einstellungen.yaml`: `untertitel`, `einleitung` | `untertitel_en`, `einleitung_en` in derselben Datei | Untertitel und große Zeile |
| `lebenslauf.yaml` | `lebenslauf.en.yaml` | Profil und Lebenslauf, gleicher Aufbau |
| `impressum.md`, `datenschutz.md` | `impressum.en.md`, `datenschutz.en.md` | der ganze Text |
| `projekte/<ordner>/index.md` | `projekte/<ordner>/index.en.md` | nur die Texte, siehe unten |
| `seiten/<name>.md` | `seiten/<name>.en.md` | Titel, Beschreibung und Text |

**Ein Projekt übersetzen:**

1. `inhalt/projekte/<ordner>/index.en.md` öffnen. `npm run new` legt die Datei an; fehlt sie, kopiere `vorlagen/projekt.en.md` dorthin.
2. Oben `titel`, `kurz`, `rolle`, `tags`, `stichpunkte` und `titelbild_alt` auf Englisch eintragen, so viele davon, wie `index.md` hat.
3. Darunter den Text auf Englisch, mit denselben Bildern wie in `index.md`.
4. Steht im Titelbild Text, eine englische Fassung daneben legen (z. B. `titelbild.en.png`) und als `titelbild` eintragen. Sonst gilt das Bild aus `index.md`.
5. Speichern und unter `/en/projects/<ordner>/` nachsehen.

In `index.en.md` stehen nur Texte. Datum, Links, Video, Reihenfolge, Startseite und Entwurf trägst du allein in `index.md` ein, sie gelten für beide Sprachen. Schreibst du so ein Feld in `index.en.md`, bricht die Vorschau mit einer Meldung ab, die das erklärt.

Bilder mit Text im Projekttext bekommen ebenfalls eine englische Fassung mit `.en` im Namen, zum Beispiel `abb/01_aufgabe.en.svg`. Die gezeichneten Titelbilder erzeugen ihre Skripte in beiden Sprachen (`node scripts/titelbild-korridor.mjs` usw.). Für Abb. 01 von `cone-corridor` gibt es `python3 scripts/abbildung-01-englisch.py`.

**Fehlt eine Übersetzung,** steht auf der englischen Seite der deutsche Text. Die Seite funktioniert dann trotzdem. `npm run check` listet unter „Hinweise“ alles auf, was noch nicht übersetzt ist. Das hält die Veröffentlichung nicht auf.

---

## Markdown, das Nötigste

Das ist die Schreibweise für alle `.md`-Dateien.

```markdown
## Überschrift zweiter Ebene
### Überschrift dritter Ebene

Ein Absatz. Eine Leerzeile trennt Absätze.

**fett** und *kursiv*

- Aufzählung
- zweiter Punkt

1. Nummeriert
2. zweiter Punkt

[Linktext](https://beispiel.de)
[Auf eine eigene Seite](/cv/)

![Was zu sehen ist](./bild.jpg)

`Code im Text`

Einmal pro Seite: <mark>hervorgehobener Satz</mark>
```

Der Marker `<mark>` wirkt wie ein Leuchtstift. Höchstens einmal pro Seite, und nie in einer Überschrift.

---

## Hinweise zu Fotos und Videos

- Handyfotos als **JPG** speichern, nicht als HEIC. Am iPhone unter „Einstellungen", „Kamera", „Formate" auf „Maximale Kompatibilität" stellen. `npm run check` warnt, wenn eine HEIC-Datei in `inhalt/` liegt.
- **Keine GIFs.** Sie sind groß und ruckeln. Nimm ein kurzes MP4 und lass es durch `npm run video` laufen.
- Bilder über 5 MB machen das Bauen langsam. Ein Foto mit 2000 Pixeln Breite reicht.
- Fotos ohne Text brauchen keine englische Fassung. Beide Sprachen zeigen dieselbe Datei.
- Videos ohne Ton aufnehmen oder den Ton ignorieren. Das Skript entfernt ihn, weil die Videos stumm laufen.
- Zeig lieber ein kurzes Video vom laufenden System als fünf Standbilder davon.
