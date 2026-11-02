# Anleitung

Vier Anleitungen, je nachdem was du vorhast.

| | |
|---|---|
| **[Von der Domain bis zur veröffentlichten Website](ANLEITUNG-START.md)** | Der ganze Weg beim ersten Mal, in der richtigen Reihenfolge. Domain kaufen, Inhalte schreiben. Die Schritte zu Cloudflare sind durch die GitHub-Anleitung ersetzt. Einmal durcharbeiten. |
| **[Veröffentlichen mit GitHub Pages](ANLEITUNG-GITHUB.md)** | Einmalig einrichten: Repository, Domain, Pages. Danach genügt `git push`. |
| **[Inhalte schreiben](ANLEITUNG-INHALTE.md)** | Projekte, Seiten, Bilder, Videos, Lebenslauf. Das brauchst du ständig. |
| **[Veröffentlichen](ANLEITUNG-VEROEFFENTLICHEN.md)** | Änderungen ins Netz bringen und Fehlermeldungen lesen. Ein Befehl. |

---

## Die drei Befehle, die du am häufigsten brauchst

```
npm run dev       Seite lokal ansehen, unter http://localhost:4321
npm run check     Alles prüfen, nichts hochladen
npm run deploy    Prüfen und anzeigen, was noch hochgeladen werden muss
```

`npm run dev` beendest du mit `Strg + C`.

---

## Wo liegt was

Alles, was du pflegst, liegt in **`inhalt/`**. Dort brauchst du keinen Code anzufassen.

```
inhalt/
  einstellungen.yaml     Name, Links, Domain, Untertitel und Einleitung in beiden Sprachen
  lebenslauf.yaml        Profil und Lebenslauf: auf /cv/, Kurzform auf der Startseite
  lebenslauf.en.yaml     Dasselbe auf Englisch
  foto/                  Dein Foto, ein Bild
  impressum.md           (und impressum.en.md)
  datenschutz.md         (und datenschutz.en.md)
  projekte/              Ein Ordner pro Projekt, darin index.md und index.en.md
  seiten/                Eine Datei pro eigener Seite, dazu <name>.en.md
```

Die Seite gibt es auf Deutsch unter `/` und auf Englisch unter `/en/`. Die englischen Texte stehen in den Dateien mit `.en` im Namen. Wie das zusammenhängt, steht in [Inhalte schreiben](ANLEITUNG-INHALTE.md#die-englische-fassung).

Der Rest des Projekts ist Code und Werkzeug:

| Ordner | Inhalt |
|---|---|
| `src/` | Der Code der Website |
| `scripts/` | Die Skripte hinter den `npm run`-Befehlen |
| `vorlagen/` | Vorlagen für neue Projekte und Seiten |
| `dist/` | Die gebaute Seite, entsteht automatisch |
| `.screenshots/` | Bilder aus `npm run shots`, nur zur Kontrolle |

`PLAN.md` und `CLAUDE.md` richten sich an jemanden, der am Code arbeitet. Für die Pflege der Inhalte brauchst du sie nicht.
