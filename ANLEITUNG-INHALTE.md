# Inhalte schreiben

Alles, was du an der Seite pflegst, liegt im Ordner `inhalt/`. Du brauchst dafür keinen Code anzufassen.

Nach jeder Änderung speichern. Läuft `npm run dev`, siehst du das Ergebnis sofort im Browser.

---

## Wo liegt was

```
inhalt/
  einstellungen.yaml     Name, Sprache, Links, Domain
  ueber-mich.md          Der Text auf /about/
  lebenslauf.yaml        Der Lebenslauf auf /cv/
  lebenslauf.pdf         Optional: dein eigenes PDF
  impressum.md
  datenschutz.md
  projekte/
    mein-projekt/
      index.md           Kopfdaten und Text
      titelbild.jpg
      demo.mp4
  seiten/
    meine-seite.md       Wird zu /meine-seite/
vorlagen/                Vorlagen, von den Skripten benutzt
```

---

## Die Startseite

Die Startseite ist wie ein technisches Zeichnungsblatt aufgebaut und besteht aus drei Teilen, die du alle über `inhalt/` steuerst.

**Oben das Startbild.** Deine Einleitung groß links, rechts das bewegte Zeichen als „Abb. 01“: eine Punktmatrix, über die farbige Ringe laufen, jeder von einem zufälligen Punkt aus. Alle 40 Sekunden zeichnen die Ringe deine Initialen, einen Buchstaben nach dem anderen. Welche Buchstaben das sind, steht in `inhalt/einstellungen.yaml` bei `initialen`; leer lassen blendet sie aus. Darunter eine Leiste mit der Anzahl deiner Projekte, dem Datum des neuesten und allen Themen aus den Kopfdaten.

- Die **Einleitung** steht in `inhalt/einstellungen.yaml` bei `einleitung`. Ein bis zwei Sätze, woran du arbeitest, nicht wer du bist. Leer lassen (`einleitung: ""`) blendet sie aus.
- Die Leiste füllt sich von selbst. Du musst dort nichts eintragen.

**Darunter die ausgewählten Arbeiten.** Jedes Projekt mit `startseite: true` erscheint als Datenblatt: links Nummer, Titel, Datum, Rolle und Themen, rechts der Satz aus `kurz` und das Titelbild.

- Die Anordnung bestimmst du mit `reihenfolge`. Ist kein Projekt markiert, stehen dort die drei neuesten.
- Drei bis vier Projekte sind ein gutes Maß. Mit Titelbild wirken sie am stärksten.
- Die Nummer (P-01, P-02 und so weiter) vergibt die Seite selbst, gezählt ab dem ältesten Projekt. Ein neues Projekt verschiebt die Nummern der alten nicht, außer du trägst eines mit älterem Datum nach.
- Unter jedem Bild steht eine Maßlinie mit seinen echten Pixelmaßen.

**Ganz unten die Übersicht.** Über mich, Lebenslauf und jede Seite aus `inhalt/seiten/`. Legst du eine neue Seite an, erscheint sie hier automatisch mit ihrer `beschreibung`.

---

## Ein Projekt anlegen

1. `npm run new -- "Titel des Projekts"` ausführen.
2. Die angelegte Datei `inhalt/projekte/<ordner>/index.md` öffnen.
3. Die Kopfdaten oben ausfüllen und alle `[TODO: ...]` ersetzen.
4. Darunter die vier Abschnitte schreiben.
5. Wenn es fertig ist, `entwurf: true` auf `entwurf: false` ändern.

### Die Kopfdaten

| Feld | Pflicht | Bedeutung |
|---|---|---|
| `titel` | ja | Name des Projekts |
| `kurz` | ja | Ein bis zwei Sätze für die Übersicht, höchstens 200 Zeichen |
| `datum` | ja | Format `JJJJ-MM`, bestimmt die Reihenfolge |
| `bis` | nein | Format `JJJJ-MM`, Ende eines Zeitraums. Dann steht auf der Seite „Mai 2026 – Juli 2026“ |
| `rolle` | nein | „Allein" oder „Perception im Viererteam" |
| `tags` | nein | `[ROS 2, SLAM, Python]` |
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

Auf der Unterseite stehen die Zwischenüberschriften links neben ihrem Text und werden automatisch durchnummeriert (01, 02, ...). Schreib die Nummern also nicht selbst dazu.

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

Ein paar Adressen sind vom Code belegt: `about`, `cv`, `projects`, `impressum`, `datenschutz`. Das Skript sagt dir das, falls du eine davon erwischst.

---

## Der Text über dich

1. `inhalt/ueber-mich.md` öffnen.
2. Den Platzhalter durch deinen Text ersetzen. 60 bis 120 Wörter, in der Ich-Form.
3. Speichern.

Der Text steht auf `/about/`, erreichbar über die Kopfzeile. Die Startseite zeigt nur Arbeiten.

Ein Aufbau, der funktioniert: erster Satz Studium und Schwerpunkt, zweiter bis vierter Satz ein konkretes Projekt mit deiner Rolle und dem Ergebnis, letzter Satz was du als nächstes suchst.

---

## Der Lebenslauf

1. `inhalt/lebenslauf.yaml` öffnen.
2. Abschnitte und Einträge ändern. Die Reihenfolge in der Datei ist die Reihenfolge auf der Seite.
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

**Eigenes PDF verwenden:** Leg dein PDF als `inhalt/lebenslauf.pdf` ab und führe `npm run cv` aus. Der Download liefert ab dann dein PDF. Löschst du die Datei wieder, wird das PDF wieder aus der Seite erzeugt.

---

## Sprache umstellen

1. `inhalt/einstellungen.yaml` öffnen.
2. `sprache: en` auf `sprache: de` ändern oder umgekehrt.
3. Speichern.

Das ändert alle Beschriftungen und die Datumsangaben. Deine eigenen Texte übersetzt es nicht, die schreibst du in der Sprache, die du willst. Die Adressen der Seiten bleiben gleich.

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
[Auf eine eigene Seite](/about/)

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
- Videos ohne Ton aufnehmen oder den Ton ignorieren. Das Skript entfernt ihn, weil die Videos stumm laufen.
- Zeig lieber ein kurzes Video vom laufenden System als fünf Standbilder davon.
