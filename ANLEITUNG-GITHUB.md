# Veröffentlichen mit GitHub Pages

Einmal einrichten, danach reicht `git push`. GitHub prüft und baut die Seite selbst und stellt sie online. Das Hosting kostet nichts. Nur die Domain kostet Geld.

Der Ablauf steht in `.github/workflows/veroeffentlichen.yml`. Er läuft bei jedem Push auf den Zweig `main`: Abhängigkeiten installieren, `npm run check` ausführen (baut die Seite, prüft sie und erzeugt das Lebenslauf-PDF), dann `dist/` veröffentlichen. Findet die Prüfung etwas, bleibt die alte Fassung online.

**Wichtig:** Die Seite muss unter einer eigenen Domain oder unter `<benutzername>.github.io` liegen, nicht unter `<benutzername>.github.io/<repository>`. Alle Links im Code beginnen mit `/` und würden in einem Unterpfad ins Leere zeigen.

---

## Einmalig einrichten

### 1. Vorher: Inhalte fertig machen

`npm run check` muss durchlaufen, sonst veröffentlicht GitHub nichts. Dazu gehören ein echter Name, deine Domain in `inhalt/einstellungen.yaml` (ohne Schrägstrich am Ende) und keine `[TODO`-Reste. Lokal prüfst du das mit `npm run check`.

Der Datenschutztext in `inhalt/datenschutz.md` muss zum Hosting passen. Beschreibt er noch Cloudflare, ersetze es durch GitHub Pages (Server-Logs bei GitHub, Microsoft als Betreiber).

### 2. Zweig in `main` umbenennen

Das Projekt heißt lokal `master`, der Workflow wartet auf `main`:

```
git branch -m master main
```

### 3. Alles speichern

```
git add -A
git commit -m "Seite für GitHub Pages vorbereiten"
```

### 4. Repository bei GitHub anlegen

1. <https://github.com/new> öffnen.
2. Namen wählen, zum Beispiel `website`.
3. **Public** wählen (GitHub Pages ist bei privaten Repositories nur in bezahlten Tarifen frei).
4. Keine README, keine `.gitignore`, keine Lizenz anhaken. Das Repository soll leer sein.
5. „Create repository" klicken.

### 5. Hochladen

GitHub zeigt dir danach die Adresse an. Dann:

```
git remote add origin https://github.com/<benutzername>/website.git
git push -u origin main
```

Der erste Lauf scheitert, solange Pages noch nicht eingeschaltet ist. Das ist normal.

### 6. Pages einschalten

1. Im Repository auf „Settings", dann links auf „Pages".
2. Bei „Build and deployment" unter „Source" **GitHub Actions** wählen.
3. Unter „Actions" den fehlgeschlagenen Lauf öffnen und „Re-run all jobs" klicken (oder einen neuen Push machen).

Nach etwa drei Minuten steht unter „Settings", „Pages" die Adresse der Seite.

### 7. Eigene Domain verbinden

1. Bei deinem Domain-Anbieter im DNS-Bereich Einträge anlegen:
   - Für `www.deine-domain.de`: ein **CNAME**-Eintrag, Name `www`, Ziel `<benutzername>.github.io`.
   - Für `deine-domain.de` ohne `www`: vier **A**-Einträge mit Name `@` auf `185.199.108.153`, `185.199.109.153`, `185.199.110.153`, `185.199.111.153`. Aktuelle Werte stehen in der GitHub-Doku unter „Managing a custom domain for your GitHub Pages site".
2. Im Repository unter „Settings", „Pages", „Custom domain" deine Domain eintragen und speichern.
3. Warten, bis GitHub die Domain geprüft hat (Minuten bis zu einem Tag).
4. „Enforce HTTPS" ankreuzen, sobald es sich anklicken lässt.
5. `domain` in `inhalt/einstellungen.yaml` auf genau diese Adresse setzen, mit `https://`.

Ohne eigene Domain trägst du bei `domain` `https://<benutzername>.github.io` ein. Dann muss das Repository `<benutzername>.github.io` heißen.

---

## Danach jedes Mal

```
npm run deploy
```

prüft und sagt dir, was noch fehlt. Dann:

```
git add -A
git commit -m "Was du geändert hast"
git push
```

Fortschritt und Fehler siehst du im Repository unter „Actions". Ein grüner Haken heißt: online.

---

## Wenn etwas nicht klappt

| Problem | Lösung |
|---|---|
| Lauf ist rot beim Schritt „npm run check" | Dort steht derselbe Bericht wie lokal. Behebe die Punkte und pushe neu. |
| „Get Pages site failed" oder „Not Found" | Pages ist noch nicht auf „GitHub Actions" gestellt (Schritt 6). |
| Seite ohne Formatierung | `domain` stimmt nicht oder die Seite liegt in einem Unterpfad (siehe oben). |
| Quelltexte unter `/code/` werden heruntergeladen statt angezeigt | GitHub ignoriert `public/_headers`. Dann die Dateien als `.txt` anbieten, z. B. `cone_corridor.cpp.txt`, und die Links anpassen. |
| Alte Fassung sichtbar | Mit `Strg + Shift + R` neu laden. |
| Änderung rückgängig machen | `git revert HEAD`, dann `git push`. |
