# Anleitung

Drei Anleitungen, je nachdem was du vorhast.

| | |
|---|---|
| **[Von der Domain bis zur veröffentlichten Website](ANLEITUNG-START.md)** | Der ganze Weg beim ersten Mal, in der richtigen Reihenfolge. Domain kaufen, Cloudflare einrichten, Inhalte schreiben, veröffentlichen. Einmal durcharbeiten. |
| **[Inhalte schreiben](ANLEITUNG-INHALTE.md)** | Projekte, Seiten, Bilder, Videos, Lebenslauf. Das brauchst du ständig. |
| **[Veröffentlichen](ANLEITUNG-VEROEFFENTLICHEN.md)** | Änderungen ins Netz bringen und Fehlermeldungen lesen. Ein Befehl. |

---

## Die drei Befehle, die du am häufigsten brauchst

```
npm run dev       Seite lokal ansehen, unter http://localhost:4321
npm run check     Alles prüfen, nichts hochladen
npm run deploy    Prüfen und veröffentlichen
```

`npm run dev` beendest du mit `Strg + C`.

---

## Wo liegt was

Alles, was du pflegst, liegt in **`inhalt/`**. Dort brauchst du keinen Code anzufassen.

```
inhalt/
  einstellungen.yaml     Name, Sprache, Links, Domain
  ueber-mich.md          Der Text auf /about/
  lebenslauf.yaml        Der Lebenslauf auf /cv/
  impressum.md
  datenschutz.md
  projekte/              Ein Ordner pro Projekt
  seiten/                Eine Datei pro eigener Seite
```

Der Rest des Projekts ist Code und Werkzeug:

| Ordner | Inhalt |
|---|---|
| `src/` | Der Code der Website |
| `scripts/` | Die Skripte hinter den `npm run`-Befehlen |
| `vorlagen/` | Vorlagen für neue Projekte und Seiten |
| `dist/` | Die gebaute Seite, entsteht automatisch |
| `.screenshots/` | Bilder aus `npm run shots`, nur zur Kontrolle |

`PLAN.md` und `CLAUDE.md` richten sich an jemanden, der am Code arbeitet. Für die Pflege der Inhalte brauchst du sie nicht.
