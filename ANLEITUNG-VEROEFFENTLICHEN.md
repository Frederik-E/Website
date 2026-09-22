# Veröffentlichen

Wie du deine Änderungen ins Netz bringst. Für den allerersten Aufbau gibt es eine eigene Anleitung: [ANLEITUNG-START.md](ANLEITUNG-START.md).

---

## Der ganze Vorgang

1. Terminal im Projektordner öffnen.
2. `npm run deploy` eingeben.
3. Warten. Meist unter zwei Minuten.
4. Die Seite im Browser neu laden.

Das ist alles. Es gibt kein Kontingent, du kannst beliebig oft veröffentlichen.

Siehst du deine Änderung nicht, lade mit `Strg + Shift + R` neu (auf dem Mac `Cmd + Shift + R`). Der Browser hält alte Dateien manchmal fest.

---

## Was dabei passiert

`npm run deploy` macht nacheinander:

1. **Prüfen.** Es sucht nach fehlenden Texten, kaputten Links, Problemen bei der Barrierefreiheit und Anfragen an fremde Server.
2. **Bauen.** Aus deinen Dateien in `inhalt/` entstehen fertige HTML-Seiten, verkleinerte Bilder und das Lebenslauf-PDF.
3. **Hochladen.** Nur wenn Schritt 1 fehlerfrei war.

Findet die Prüfung etwas, bricht der Befehl ab und lädt nichts hoch. Deine Seite im Netz bleibt dann unverändert. Es kann also nichts kaputtgehen.

---

## Vorher ansehen, ohne hochzuladen

1. `npm run dev` eingeben.
2. <http://localhost:4321> im Browser öffnen.
3. Ändern und speichern. Der Browser aktualisiert sich von selbst.
4. Zum Beenden `Strg + C` drücken.

In dieser Vorschau siehst du auch alles, was als Entwurf markiert ist. Auf der veröffentlichten Seite fehlt das.

Willst du den ganzen Ablauf testen, ohne etwas hochzuladen:

```
npm run deploy -- --dry-run
```

Das prüft und baut alles und hört vor dem Hochladen auf.

---

## Nur prüfen

```
npm run check
```

Dauert etwa eine Minute. Am Ende steht ein Bericht in drei Teilen.

---

## Fehlermeldungen lesen

### Fehlende Inhalte

Stellen, an denen noch dein Text fehlt. Beispiel:

```
inhalt/ueber-mich.md
  Zeile 1: [TODO: 60 bis 120 Woerter ueber dich ...
```

Öffne die genannte Datei in der genannten Zeile und ersetze den Platzhalter. Die Seite wird nicht veröffentlicht, solange irgendwo `[TODO` steht.

### Technische Fehler

Betreffen den Code oder die Barrierefreiheit. Beispiel:

```
/projects/mein-projekt/
  axe: image-alt (critical) Images must have alternate text
```

Meistens fehlt eine Bildbeschreibung. Trage `titelbild_alt` in den Kopfdaten des Projekts nach.

Ein anderer häufiger Fall:

```
Fremde Anfrage
  fonts.googleapis.com wurde geladen
```

Dann lädt die Seite etwas von einem fremden Server. Das darf nicht sein, weil die Seite ohne Zustimmung keine Daten an Dritte geben soll. Meist kommt es von einem eingebetteten Video oder einer Schriftart in einem eigenen Text.

### Hinweise

Empfehlungen, keine Fehler. Sie halten die Veröffentlichung nicht auf. Beispiel: ein Bild über 5 MB, eine HEIC-Datei, eine übrig gebliebene Beispielseite.

### Fehler beim Bauen

Bricht schon das Bauen ab, nennt die Meldung Datei und Feld:

```
Fehler in inhalt/lebenslauf.yaml:
  - abschnitte.0.eintraege.0.von: Feld "von" braucht das Format JJJJ-MM, zum Beispiel 2025-10.
```

Meist ist ein Doppelpunkt, ein Anführungszeichen oder eine Einrückung verrutscht. YAML ist streng mit Leerzeichen: nur Leerzeichen, keine Tabulatoren.

---

## Wenn das Hochladen scheitert

**„Not logged in" oder eine Frage nach Anmeldung**

```
npx wrangler login
```

Der Browser öffnet sich, auf „Allow" klicken.

**„wrangler: command not found"**

```
npm install -D wrangler
```

**Der Befehl hängt sehr lange**

Mit `Strg + C` abbrechen und noch einmal starten. Es wird nichts halb hochgeladen: entweder die ganze neue Fassung ist oben oder die alte bleibt stehen.

**Eine Datei ist zu groß**

Cloudflare nimmt höchstens 25 MB pro Datei. Betrifft praktisch nur Videos. `npm run video` warnt ab 10 MB und bricht ab 25 MB ab.

---

## Eine Änderung rückgängig machen

Es gibt keinen Zurück-Knopf im Netz. Du änderst die Dateien zurück und veröffentlichst neu.

Wenn du mit Git arbeitest (siehe [ANLEITUNG-START.md](ANLEITUNG-START.md), letzter Schritt):

1. `git status` zeigt, was du geändert hast.
2. `git checkout -- inhalt/` setzt alle Änderungen in `inhalt/` zurück.
3. `npm run deploy` veröffentlicht den alten Stand wieder.

Ohne Git: die Datei aus deiner Sicherungskopie zurückholen und neu veröffentlichen.

---

## Alle Befehle

| Befehl | Wirkung |
|---|---|
| `npm run dev` | Lokale Vorschau, mit Entwürfen |
| `npm run check` | Alles prüfen, nichts hochladen |
| `npm run deploy` | Prüfen und veröffentlichen |
| `npm run deploy -- --dry-run` | Alles außer dem Hochladen |
| `npm run new -- "Titel"` | Neues Projekt anlegen |
| `npm run new-seite -- "Titel"` | Neue Seite anlegen |
| `npm run video -- <datei> <ordner>` | Video verkleinern und einbauen |
| `npm run cv` | Lebenslauf-PDF neu erzeugen |
